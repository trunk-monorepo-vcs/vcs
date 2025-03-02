package main

import (
    "backend/server"
	"fmt"
	"os"
    "github.com/joho/godotenv"
)


// init is invoked before main()
func init() {
    // loads values from .env into the system
    if err := godotenv.Load("../.env"); err != nil {
        fmt.Println("No .env file found")
    }
}


func main() {
    // Store the SERVER_PORT environment variable in a variable
    port, exists := os.LookupEnv("SERVER_PORT")
    if !exists {
        fmt.Println("Cannot find SERVER_PORT env")
        return
    }

    // Init backend application
    status, err := server.InitServer(port)
    if status != 0 {
        err.Error()
        fmt.Errorf("Cannot init server on %d port", port)
    }
}