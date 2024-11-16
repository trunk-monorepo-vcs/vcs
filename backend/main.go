package main

import (
    "backend"        // 引入backend包，用于初始化和管理后端服务
    "backend/server" // 引入server包，可能包含与后端服务器相关的功能
	"fmt"            // 引入fmt包，用于格式化输出
	"os"             // 引入os包，操作系统相关功能，如获取环境变量
)

func main() {
    // 获取环境变量 SERVER_PORT 的值，并检查是否存在
    port, exists := os.LookupEnv("SERVER_PORT") // LookupEnv函数返回环境变量的值和是否存在的标志
    if exists == false { // 如果没有找到 SERVER_PORT 环境变量
        fmt.Errorf("Cannot find SERVER_PORT env") // 输出错误信息，提示找不到 SERVER_PORT 环境变量
    }

    // 初始化后端应用程序，传入 SERVER_PORT 作为参数
    status, err := backend.initServer(port) // 调用backend包中的initServer函数初始化服务器，返回状态码和错误信息
    if status != 0 { // 如果服务器初始化失败（假设状态码0表示成功）
        fmt.Errorf("Error %s", err) // 输出初始化失败的错误信息
    }
}
