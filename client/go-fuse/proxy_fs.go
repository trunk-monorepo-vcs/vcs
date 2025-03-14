// Copyright 2016 the Go-FUSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This program is the analogon of libfuse's hello.c, a program that
// exposes a single file "file.txt" in the root directory.

package main

import (
	"context"
	"database/sql"
	"flag"
	"log"
	"syscall"
	"os"
	"sync"
	
	"github.com/mattn/go-sqlite3"
	"github.com/hanwen/go-fuse/v2/fs"
	"github.com/hanwen/go-fuse/v2/fuse"
)

type proxyFs struct {
	fs.Inode
	db     *sql.DB
	dbPath string
}

type SQLStoredFile struct {
	name String
	data []byte
	mode int
}

type File struct {
	fs.Inode
	Content []byte
	mu      sync.Mutex 
	db   *sql.DB
}


type FileHandle struct {
	file *File
}

type SQLiteFile struct {
	fs.Inode
	db   *sql.DB
	name string
}
func (r *proxyFs) logToFile(message string) {
	ch := r.GetChild("log.txt")
	if ch == nil {
		return
	}

	file, ok := ch.Operations().(*fs.MemRegularFile)
	if !ok {
		return
	}

	file.Data = append(file.Data, []byte(message+"\n")...)
}

func (f *File) logToFile(message string) {
	ch := f.GetChild("log.txt")
	if ch == nil {
		return
	}

	file, ok := ch.Operations().(*fs.MemRegularFile)
	if !ok {
		return
	}

	file.Data = append(file.Data, []byte(message+"\n")...)
}

func initDB(db *sql.DB) {
	_, err := db.Exec(`
		CREATE TABLE IF NOT EXISTS files(
			name TEXT PRIMARY KEY,
			data BLOB,
			mode INTEGER
		)
	`)
	if err != nil {
		log.Println("Failed to create table:", err)
	}
}

func newSQLite(dbPath string) *proxyFs {
	db, err := sql.Open("sqlite3", dbPath)
	if err != nil {
		log.Fatal("Failed to open database:", err)
	}
	initDB(db)

	return &proxyFs{db: db, dbPath: dbPath}
}

func (r *proxyFs) OnAdd(ctx context.Context) {
	ch := r.NewPersistentInode(
		ctx, &fs.MemRegularFile{
			Data: []byte("This is log file.\n"),
			Attr: fuse.Attr{
				Mode: 0644,
			},
		}, fs.StableAttr{Ino: 2})
	r.AddChild("log.txt", ch, false)
}

func (r *proxyFs) Getattr(ctx context.Context, fh fs.FileHandle, out *fuse.AttrOut) syscall.Errno {
	out.Mode = 0755
	return 0
}

func (r *proxyFs) Create(ctx context.Context, name string, flags uint32, mode uint32, out *fuse.EntryOut) (*fs.Inode, fs.FileHandle, uint32, syscall.Errno) {
	var result sql.NullString
	err := r.db.QueryRow("SELECT * FROM files WHERE name = ? LIMIT 1", name).Scan(&result)
	if err != nil {
	  	r.logToFile(name + " failed query sql: " + err.Error())
	  	return nil, nil, 0, syscall.EIO
	}

  
	if !result.Valid {
	  	_, err = r.db.Exec("INSERT INTO files (name, data, mode) VALUES (?, ?, ?) ON CONFLICT(name) DO UPDATE SET data = ?, mode = ?",
		name, []byte{}, mode, []byte{}, mode)
		if err != nil {
			r.logToFile(name + " failed create: " + err.Error())
			return nil, nil, 0, syscall.EIO
		}
	}
  
	stable := fs.StableAttr{Ino: uint64(len(r.Children()) + 2), Mode: fuse.S_IFREG}
	Inode := r.NewPersistentInode(ctx, &SQLiteFile{db: r.db, name: name}, stable)
	r.AddChild(name, Inode, false)
	r.logToFile(name + " created")
	return Inode, nil, 0, 0
}
func (r *SQLiteFile) Readdir(ctx context.Context)(fs.DirStream, syscall.Errno){
	rows, err := r.db.Query("SELECT name FROM files")
	if (err != nil){
		log.Println("Error reading directory", err)
		return nil, syscall.EIO
	}
	defer rows.Close()
	var entries []fuse.DirEntry
	for rows.Next(){
		var name string 
		rows.Scan(&name)
		entries = append(entries, fuse.DirEntry{
			Name: name,
			Ino: uint64(len(entries) + 2),
			Mode: fuse.S_IFREG,
		})


	}
	return fs.NewListDirStream(entries), 0

}

func (r *SQLiteFile) Opendir(ctx context.Context) syscall.Errno {
	rows, err := r.db.Query("SELECT name FROM files")
	if err != nil {
		log.Println("Error reading directory:", err)
		return syscall.EIO
	}
	defer rows.Close()
	for rows.Next(){
		var name string 
		if err := rows.Scan(&name); err != nil{
			log.Println("Error scanning dirname", err)
			continue
		}

	//checking file
		if r.GetChild(name) == nil{
			stable :=fs.StableAttr{
				Ino: uint64(len(r.Children()) + 2),
				Mode: fuse.S_IFREG,
			}
			inode := r.NewPersistentInode(ctx, &SQLiteFile{db: r.db, name: name}, stable)
			r.AddChild(name, inode, false)
		}
	}

func (f *File) Open(ctx *fuse.Context, flags uint32) (fh fs.FileHandle, fuseFlags uint32, errno syscall.Errno) {
	if _, err := os.Stat(f.Path(nil)); err == nil {
		return &FileHandle{file: f}, 0, 0
	} else {
		f.mu.Lock()
		defer f.mu.Unlock() 

		var result sql.NullString
		row := f.db.QueryRow("SELECT * FROM files WHERE name = ? LIMIT 1", f.Path(nil))
		err := row.Scan(&result)
		if err != nil {
			  f.logToFile(f.Path(nil) + " failed query sql: " + err.Error())
			  return nil, 0, syscall.EIO
		}

		if !result.Valid {
			return nil, 0, syscall.EIO
		}
		
		//stable := fs.StableAttr{Ino: uint64(len(f.Children()) + 2), Mode: fuse.S_IFREG}
		//Inode := f.NewPersistentInode(ctx, &SQLiteFile{db: f.db, name: f.Path(nil)}, stable)
		var content SQLStoredFile
		err = row.Scan(&content)
		if err != nil {
			return nil, 0, syscall.EIO
		}
		f.Content = content.data

		return &FileHandle{file: f}, 0, 0
	}
}

func (fh *FileHandle) Write(ctx context.Context, data []byte, off int64) (uint32, syscall.Errno) {
	fh.file.mu.Lock()
	defer fh.file.mu.Unlock()

	
	newSize := int(off) + len(data)
	if newSize > len(fh.file.Content) {
		newContent := make([]byte, newSize)
		copy(newContent, fh.file.Content)
		fh.file.Content = newContent
	}

	copy(fh.file.Content[off:], data)

	// store data on SQLite
	_, err := fh.file.db.Exec("UPDATE files SET data = ? WHERE name = ?", fh.file.Content, fh.file.Path(nil))
	if err != nil {
		return 0, syscall.EIO
	}
	return uint32(len(data)), 0
}

func (r *proxyFs) Opendir(ctx context.Context) syscall.Errno {
	r.logToFile("Opendir called")

	return 0
}
func (r *proxyFs) Mkdir(ctx context.Context, name string, mode uint32, out *fuse.EntryOut) (*fs.Inode, syscall.Errno) {
	_, err := r.db.Exec("INSERT INTO files (name, data, mode) VALUES (?, ?, ?)", name, []byte{}, mode)
	if err != nil {
		r.logToFile("Mkdir failed: " + err.Error())
		return nil, syscall.EIO
	}

	// new inode
	stable := fs.StableAttr{Ino: uint64(len(r.Children()) + 2), Mode: fuse.S_IFDIR}
	Inode := r.NewPersistentInode(ctx, &proxyFs{db: r.db, dbPath: r.dbPath}, stable)
	r.AddChild(name, Inode, false)

	r.logToFile("Directory " + name + " created")
	return Inode, 0
}


var _ = (fs.NodeGetattrer)((*proxyFs)(nil))
var _ = (fs.NodeCreater)((*proxyFs)(nil))
var _ = (fs.NodeOpendirer)((*SQLiteFile)(nil))
var _ = (fs.NodeReaddirer)((*SQLiteFile)(nil))
var _ = (fs.NodeMkdirer)((*proxyFs)(nil))

func main() {
	debug := flag.Bool("debug", false, "print debug data")
	flag.Parse()
	if len(flag.Args()) < 1 {
		log.Fatal("Usage:\n  hello MOUNTPOINT")
	}

	dbPath := "fuse.db"
	fsRoot := newSQLite(dbPath)
	opts := &fs.Options{}
	opts.Debug = *debug
	server, err := fs.Mount(flag.Arg(0), fsRoot, opts)
	if err != nil {
		log.Fatalf("Mount fail: %v\n", err)
	}


	server.Wait() 
}
