package main

import (
    "backend"
    "backend/server"
	"fmt"
	"os"
)

func main() {
    // Store the SERVER_PORT environment variable in a variable
    port, exists := os.LookupEnv("SERVER_PORT")
    if exists == false {
        fmt.Errorf("Cannot find SERVER_PORT env")
    }

    // Init backend application
    status, err := backend.initServer(port)
    if status != 0 {
        fmt.Errorf("Error %s", err)
    }
}
