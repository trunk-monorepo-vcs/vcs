package server

import (
	"encoding/binary"
	"fmt"
	"net"
	//"utils"
	"encoding/binary"
	"os"
	"slices"
	"utils"
)

type connectionFinishingStatus struct {
	err    string
	status int
	addr   net.Addr
}

func (e *connectionFinishingStatus) Error() string {
	return e.err + "in connection wit id: " + e.addr.String() + ".\n"
}

// func (e *connectionFinishingStatus) sendErrorMsgToClient(conn net.Conn) {
//     conn.Write(int32(len(e.err)))

// }

func initServer(port string) (int, error) {
	// Listen for incoming connection on port `port`
	ln, err := net.Listen("tcp", ":"+port)
	if err != nil {
		return 1, err
	}

	// Accept only one connection
	// For demo with one user
	conn, err := ln.Accept()
	if err != nil {
		return 1, err
	}

	channel := make(chan *connectionFinishingStatus)
	// Handle the connection in a new goroutine
	go handleConnection(conn, channel)

	// Geting connectionFInishingStatus
	var connFinishingStatus *connectionFinishingStatus = <-channel

	// Handle connectionFInishingStatus
	switch connFinishingStatus.status {
	case 0:
		return 0, nil
	case 1:
		return 1, connFinishingStatus
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
    
    for ;; {
        // Read incoming data
	    protocolNumBytes := make([]byte, 1)
        _, err := conn.Read(protocolNumBytes)
        if err != nil {
            connFinishingStatus.err = err.Error()
            conn.Write([]byte(connFinishingStatus.err))
            connFinishingStatus.status = 1
            return
        }

        // Handle specific request
        switch int8(protocolNumBytes[0]) {
        case 2:
            readDir(conn, &connFinishingStatus)
        case 3:
            getStat(conn, &connFinishingStatus)
        case 4:
            readFile(conn, &connFinishingStatus)
        }
        if connFinishingStatus.status != 0 {
            return
        }
    }
}

func readDir(conn net.Conn, connFinishingStatus *connectionFinishingStatus) {
	pathAsString := getPathFromRequest(conn, connFinishingStatus)
	if pathAsString == "" {
		return
	}
	entries, err := os.ReadDir(pathAsString)
	if err != nil {
		handlersErrorHandling(connFinishingStatus, "Can't get folder entries!", conn)
		return
	}
	// Forming response
	response := make([]byte, 1)
	response[0] = 0
	lengthBuffer := make([]byte, 4)
	for _, entry := range entries {
		binary.LittleEndian.PutUint32(lengthBuffer, uint32(len(entry.Name())))
		response = slices.Concat(response, lengthBuffer)
		response = slices.Concat(response, []byte(entry.Name()))
	}

	conn.Write(response)
}

func getStat(conn net.Conn, connFinishingStatus *connectionFinishingStatus) {
	pathAsString := getPathFromRequest(conn, connFinishingStatus)
	if pathAsString == "" {
		return
	}
	info, err := os.Stat(pathAsString)
	if err != nil {
		handlersErrorHandling(connFinishingStatus, "Can't get file stat", conn)
		return
	}
	// Forming response
	response := make([]byte, 2)
	response[0] = 0
	response[1] = 0
	if info.IsDir() {
		response[1] = 1
	}
	sizeBuffer := make([]byte, 8)
	binary.LittleEndian.PutUint64(sizeBuffer, uint64(info.Size()))
	response = slices.Concat(response, sizeBuffer)

	conn.Write(response)
}

func readFile(conn net.Conn, connFinishingStatus *connectionFinishingStatus) {
	pathAsString := getPathFromRequest(conn, connFinishingStatus)
	if pathAsString == "" {
		return
	}
	file, err := os.Open(pathAsString)
	if err != nil {
		handlersErrorHandling(connFinishingStatus, "Can't get file stat", conn)
		return
	}
	fileStat, err := file.Stat()
	if err != nil {
		handlersErrorHandling(connFinishingStatus, "Can't get file stat", conn)
		return
	}
	fileSize := fileStat.Size()
	// Read file data
	data := make([]byte, fileSize)
	_, err = file.Read(data)
	if err != nil {
		handlersErrorHandling(connFinishingStatus, "Can't read file data", conn)
		return
	}
	// Forming response
	response := make([]byte, 1)
	response[0] = 0
	sizeBuffer := make([]byte, 8)
	binary.LittleEndian.PutUint64(sizeBuffer, uint64(fileSize))
	response = slices.Concat(response, sizeBuffer)
	response = slices.Concat(response, data)

	conn.Write(response)
}

func handlersErrorHandling(connFinishingStatus *connectionFinishingStatus, errString string, conn net.Conn) {
	connFinishingStatus.err = errString
	exitCode := make([]byte, 1)
	exitCode[0] = 1
	conn.Write(slices.Concat(exitCode, []byte(errString)))
	connFinishingStatus.status = 1
}

func getPathFromRequest(conn net.Conn, connFinishingStatus *connectionFinishingStatus) string {
	pathLengthBytes := make([]byte, 4)
	_, err := conn.Read(pathLengthBytes)
	if err != nil {
		handlersErrorHandling(connFinishingStatus, err.Error(), conn)
		return ""
	}
	pathLength := utils.bytes4ToInt32(pathLengthBytes)
	if pathLength < 0 {
		handlersErrorHandling(connFinishingStatus, "Negative path length: pathLength="+pathLength, conn)
		return ""
	}
	pathAsBytes := make([]byte, pathLength)
	_, err = conn.Read(pathAsBytes)
	if err != nil {
		handlersErrorHandling(connFinishingStatus, "Can't read path from request!", conn)
		return ""
	}
	return string(pathAsBytes)
}
