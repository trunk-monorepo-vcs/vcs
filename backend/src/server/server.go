package server

import (
	"backend/utils"
	"encoding/binary"
	"log"
	"net"
	"os"
	"slices"
	"strconv"
	"fmt"
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

func InitServer(port string) (int, error) {
	for {
		// Listen for incoming connection on port `port`
		ln, err := net.Listen("tcp", ":"+port)
		if err != nil {
			return 1, err
		}
		log.Output(1, "Server listening on port: "+port)

		// Accept only one connection
		// For demo with one user
		conn, err := ln.Accept()
		if err != nil {
			return 1, err
		}

		log.Output(1, "Install connection with client")


		channel := make(chan *connectionFinishingStatus)
		// Handle the connection
		handleConnection(conn, channel)

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
}

func handleConnection(conn net.Conn, channel chan *connectionFinishingStatus) {
	connFinishingStatus := connectionFinishingStatus{"", 0, conn.RemoteAddr()}
	log.Output(1, "Handle conn")

	defer func() {
		conn.Close()
		channel <- &connFinishingStatus
	}()
	
	log.Output(1,"waiting command")
	protocolNumBytes := make([]byte, 1)
	_, err := conn.Read(protocolNumBytes)
	if err != nil {
		connFinishingStatus.err = err.Error()
		conn.Write([]byte(connFinishingStatus.err))
		connFinishingStatus.status = 1
		return
	}
	protocolNum := int(protocolNumBytes[0])
	log.Output(1, "Get protocol number " + strconv.Itoa(protocolNum))
	// Handle specific request
	switch int8(protocolNumBytes[0]) {
	case 2:
		log.Output(1, "in readDir func")
		readDir(conn, &connFinishingStatus)
	case 3:
		log.Output(1, "in getStat func")
		getStat(conn, &connFinishingStatus)
	case 4:
		log.Output(1, "in readFile func")
		readFile(conn, &connFinishingStatus)
	}
	if connFinishingStatus.status != 0 {
		log.Output(1, "finstat")
		// Завершаем соединение, если статус не 0
		return
	}else{
		log.Output(1, "conn=0");
	}
}


func readDir(conn net.Conn, connFinishingStatus *connectionFinishingStatus) {
	defer func() {
		conn.Close()
	}()
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
		binary.BigEndian.PutUint32(lengthBuffer, uint32(len(entry.Name())))
		response = slices.Concat(response, lengthBuffer)
		response = slices.Concat(response, []byte(entry.Name()))
	}

	conn.Write(response)
}

func getStat(conn net.Conn, connFinishingStatus *connectionFinishingStatus) {
	defer func() {
		conn.Close()
	}()
	log.Output(1, "getstat start")
	pathAsString := getPathFromRequest(conn, connFinishingStatus)
	log.Output(1, "path: " + pathAsString)
	if pathAsString == "" {
		return
	}

	info, err := os.Stat(pathAsString)
	if err != nil {
		handlersErrorHandling(connFinishingStatus, "Can't get file stat", conn)
		return
	}

	// Формирование ответа
	response := make([]byte, 2)
	response[0] = 0 // Статус успеха
	response[1] = 0 // Тип файла, 0 - обычный файл, 1 - директория
	if info.IsDir() {
		response[1] = 1
	}

	// Буфер для размера файла
	sizeBuffer := make([]byte, 8)
	binary.BigEndian.PutUint64(sizeBuffer, uint64(info.Size()))
	response = slices.Concat(response, sizeBuffer)

	// Логирование ответа в шестнадцатеричном виде
	logHex("Response", response)

	// Отправка ответа
	_, err = conn.Write(response)
	if err != nil {
		log.Output(1, "Error sending response: "+err.Error())
		return
	}

	log.Output(1, "after sending response")
}

// Функция для логирования массива байтов в шестнадцатеричном формате
func logHex(prefix string, data []byte) {
	hexString := fmt.Sprintf("%s: ", prefix)
	for i, b := range data {
		hexString += fmt.Sprintf("%02X ", b)
		if (i+1)%16 == 0 {
			hexString += "\n"
		}
	}
	log.Output(1, hexString)
}


func readFile(conn net.Conn, connFinishingStatus *connectionFinishingStatus) {
	defer func() {
		conn.Close()
	
	}()
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
	binary.BigEndian.PutUint64(sizeBuffer, uint64(fileSize))
	response = slices.Concat(response, sizeBuffer)
	response = slices.Concat(response, data)

	conn.Write(response)
}

func handlersErrorHandling(connFinishingStatus *connectionFinishingStatus, errString string, conn net.Conn) {
	connFinishingStatus.err = errString
	exitCode := make([]byte, 1)
	exitCode[0] = 1
	_, err := conn.Write(slices.Concat(exitCode, []byte(errString)))
	if err != nil {
		log.Output(1, "Error writing error message to client: "+err.Error())
	}
	connFinishingStatus.status = 1
}


func  getPathFromRequest(conn net.Conn, connFinishingStatus *connectionFinishingStatus) string {

	pathLengthBytes := make([]byte, 4)
	_, err := conn.Read(pathLengthBytes)
	if err != nil {
		handlersErrorHandling(connFinishingStatus, err.Error(), conn)
		return ""
	}
	pathLength := utils.Bytes4ToInt32(pathLengthBytes)
	if pathLength < 0 {
		handlersErrorHandling(connFinishingStatus, "Negative path length: pathLength="+string(pathLength), conn)
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
