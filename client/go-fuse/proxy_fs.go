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

	_ "github.com/mattn/go-sqlite3"
	"github.com/hanwen/go-fuse/v2/fs"
	"github.com/hanwen/go-fuse/v2/fuse"
)

type proxyFs struct {
	fs.Inode
	db     *sql.DB
	dbPath string
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

func initDB(db *sql.DB) {
	_, err := db.Exec(`
		CREATE TABLE IF NOT EXISTS files(
			name TEXT PRIMARY KEY,
			data BLOB,
			mode INTEGER,
			parent TEXT DEFAULT '/'
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
	_, err := r.db.Exec("INSERT INTO files (name, data, mode) VALUES (?, ?, ?) ON CONFLICT(name) DO UPDATE SET data = ?, mode = ?",
		name, []byte{}, mode, []byte{}, mode)
	if err != nil {
		r.logToFile(name + " failed create: " + err.Error())
		println(name);
		return nil, nil, 0, syscall.EIO
	}
	
	stable := fs.StableAttr{Ino: uint64(len(r.Children()) + 2), Mode: fuse.S_IFREG}
	Inode := r.NewPersistentInode(ctx, &SQLiteFile{db: r.db, name: name}, stable)
	r.AddChild(name, Inode, false)
	r.logToFile(name + " created")
	return Inode, nil, 0, 0
}
func (r *proxyFs) Readdir(ctx context.Context) (fs.DirStream, syscall.Errno) {
    rows, err := r.db.Query("SELECT name, mode FROM files WHERE parent = ?", "/mydir/")
    if err != nil {
        log.Println("Error reading directory:", err)
        return nil, syscall.EIO
    }
    defer rows.Close()

    var entries []fuse.DirEntry
    for rows.Next() {
        var name string
        var mode int
        if err := rows.Scan(&name, &mode); err != nil {
            log.Println("Error scanning row:", err)
            continue
        }
        entry := fuse.DirEntry{
            Name: name,
            Ino:  uint64(len(entries) + 2),
            Mode: uint32(mode),
        }
        entries = append(entries, entry)
    }

    return fs.NewListDirStream(entries), 0
}



func (r *SQLiteFile) Opendir(ctx context.Context) syscall.Errno {
	rows, err := r.db.Query("SELECT name FROM files WHERE parent = ?", r.name)
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
	return 0
}
func (r *proxyFs) Mkdir(ctx context.Context, name string, mode uint32, out *fuse.EntryOut) (*fs.Inode, syscall.Errno) {
	parentPath := "/" + name
	_, err := r.db.Exec("INSERT INTO files (name, data, mode, parent) VALUES (?, ?, ?, ?)", name, []byte{}, mode, parentPath)
	if err != nil {
		r.logToFile("Mkdir failed: " + err.Error())
		return nil, syscall.EIO
	}

	// new inode
	stable := fs.StableAttr{Ino: uint64(len(r.Children()) + 2), Mode: fuse.S_IFDIR}
	Inode := r.NewPersistentInode(ctx, &proxyFs{db: r.db, dbPath: name}, stable)
	r.AddChild(name, Inode, false)

	r.logToFile("Directory " + name + " created")
	return Inode, 0
}


var _ = (fs.NodeGetattrer)((*proxyFs)(nil))
var _ = (fs.NodeCreater)((*proxyFs)(nil))
var _ = (fs.NodeOpendirer)((*SQLiteFile)(nil))
var _ = (fs.NodeReaddirer)((*proxyFs)(nil))
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
