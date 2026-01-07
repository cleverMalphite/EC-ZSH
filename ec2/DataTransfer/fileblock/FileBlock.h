#pragma once

#include <string>
#include <stdint.h>
#include <vector>


#ifndef FILE_BLOCK_BYTE
  #define FILE_BLOCK_BYTE uint8_t
#endif

#ifndef FILE_BLOCK_SINGLE_SIZE
  //#define FILE_BLOCK_SINGLE_SIZE 1024 //single file block size: 1024 byte
  #define FILE_BLOCK_SINGLE_SIZE 1280 //single file block size: 1024 byte
#endif
