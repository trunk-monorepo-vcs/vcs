package server

import (
    "fmt"
    "net"
    "encoding/binary"
)

type connectionFinishingStatus struct {
    err string
    status int
    addr net.Addr
}

func (e *connectionFinishingStatus) Error() string {
    return e.err + "in connection wit id: " + string(e.connectionId)
}

func initServer(port string) (int, error) {
	// Listen for incoming connection on port `port`
    ln, err := net.Listen("tcp", ":" + port)
    if err != nil {
        return -1, err
    }

    // Accept only one connection
    conn, err := ln.Accept()
    if err != nil {
        return -1, err
    }

    channel := make(chan *connectionFinishingStatus)
    // Handle the connection in a new goroutine
    go handleConnection(conn, channel)
    
    // Geting connectionFInishingStatus
    var connFinishingStatus *connectionFinishingStatus = <-channel

    // Handle connectionFInishingStatus
    switch (connFinishingStatus.status) {
    case 0:
        return 0, nil
    case -1:
        return -1, connFinishingStatus
    default:
        return 0, nil
    }
}

func handleConnection(conn net.Conn, channel chan *connectionFinishingStatus) {
    connFinishingStatus := connectionFinishingStatus{"", 0, conn.RemoteAddr()}

    // Close the connection and send status when we're done
    defer func() {
        conn.Close()
        channel <- &connFinishingStatus
    }()

    // Read incoming data
    protocolNumBytes := make([]byte, 1)

    _, err := conn.Read(protocolNumBytes)
    if err != nil {
        connFinishingStatus.err = err.Error()
        connFinishingStatus.status = -1
        return
    }

    // Handle specific request 
    switch (int8(protocolNumBytes[0])) {
    case 1:
        openDir(conn, &connFinishingStatus)
    case 2:
        readDir(conn, &connFinishingStatus)
    }

    
}

func openDir(conn net.Conn, connFinishingStatus *connectionFinishingStatus) {

}

func readDir(conn net.Conn, connFinishingStatus *connectionFinishingStatus) {

}