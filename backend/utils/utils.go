package utils

import (
	"math"
)

func bytes4ToInt32(arr [4]byte) (result int32) {
	result += int32(arr[3])
	result += int32(arr[2]) * int32(math.Pow(2, 8))
	result += int32(arr[1]) * int32(math.Pow(2, 16))
	result += int32(arr[0]) * int32(math.Pow(2, 32))

	return
}