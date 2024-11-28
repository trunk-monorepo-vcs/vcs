package utils

import (
	"math"
)

func Bytes4ToInt32(arr []byte) (result int32) {
	if len(arr) < 4 {
		return 
	}
	result += int32(arr[3])
	result += int32(arr[2]) * int32(math.Pow(2, 8))
	result += int32(arr[1]) * int32(math.Pow(2, 16))
	result += int32(arr[0]) * int32(math.Pow(2, 32))

	return
}