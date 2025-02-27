// Copyright 2016 the Go-FUSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// This program is the analogon of libfuse's hello.c, a a program that
// exposes a single file "file.txt" in the root directory.
package main

import (
	"context"
	"flag"
	"log"
	"syscall"

	"github.com/hanwen/go-fuse/v2/fs"
	"github.com/hanwen/go-fuse/v2/fuse"
)

type HelloRoot struct {
	fs.Inode
}
func (r *HelloRoot) logToFile(message string) syscall.Errno{
	node := &r.Inode
	for !node.IsRoot() {
		_, node = node.Parent()
	}
    ch := node.GetChild("log.txt")

	file, _ := ch.Operations().(*fs.MemRegularFile)
	
	file.Data  = append(file.Data, []byte(message+"\n")...)
	return 0;

}
func (r *HelloRoot) OnAdd(ctx context.Context) {
	ch := r.NewPersistentInode(
		ctx, &fs.MemRegularFile{
			Data: []byte("This is log file.\n"),
			Attr: fuse.Attr{
				Mode: 0644,
			},
		}, fs.StableAttr{Ino: 2})
	r.AddChild("log.txt", ch, false)
}

func (r *HelloRoot) Getattr(ctx context.Context, fh fs.FileHandle, out *fuse.AttrOut) syscall.Errno {
	out.Mode = 0755	
	return 0
}

func (r *HelloRoot) Create(ctx context.Context, name string, flags uint32, mode uint32, out *fuse.EntryOut) (*fs.Inode, fs.FileHandle, uint32, syscall.Errno) {
	fd := &fs.MemRegularFile{
		Data: []byte{},
		Attr: fuse.Attr{Mode: mode},
	}
	Inode := r.NewPersistentInode(ctx, fd, fs.StableAttr{Ino: 3})
	r.AddChild(name, Inode, false)

	r.logToFile(name + " create \n")
	return Inode, fd, 0, 0
}



func (r *HelloRoot) Opendir(ctx context.Context) syscall.Errno {
	return 0
}

var _ = (fs.NodeGetattrer)((*HelloRoot)(nil))	
var _ = (fs.NodeOnAdder)((*HelloRoot)(nil))
var _ = (fs.NodeCreater)((*HelloRoot)(nil))
var _ = (fs.NodeOpendirer)((*HelloRoot)(nil))

func main() {
	debug := flag.Bool("debug", false, "print debug data")
	flag.Parse()
	if len(flag.Args()) < 1 {
		log.Fatal("Usage:\n  hello MOUNTPOINT")
	}
	opts := &fs.Options{}
	opts.Debug = *debug
	server, err := fs.Mount(flag.Arg(0), &HelloRoot{}, opts)
	if err != nil {
		log.Fatalf("Mount fail: %v\n", err)
	}
	server.Wait()
}