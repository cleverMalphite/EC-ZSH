#include <FileBlock.h>
#include <ReadFileBlock.h>
#include <WriteFileBlock.h>
#include <iostream>
#include <stdio.h>

#include "../../../Util/SystemTimeFunc.h"

//#define TEST_MAIN_DEBUG

int main()
{ 

  ReadFileBlock readFileBlock("../../../../FileSend/file.txt", 1, 5000);

  std::unique_ptr<WriteFileBlock> p_write_file_block;

  printf("start read file by block.\n");

  uint64_t cnt_block = 0;

  hh::FileBlockMsg tmp_file_block ;
  
  while( readFileBlock.readNextBlock( tmp_file_block ) )
  {
    if(p_write_file_block==nullptr)
    {
      p_write_file_block = std::move(std::unique_ptr<WriteFileBlock>(
                                        new WriteFileBlock( 
                                            //tmp_file_block->FileName_, 
                                            "file.txt", 
                                            tmp_file_block.filesize_total_(),
                                            1 ,5000
                                    )));
    }
    p_write_file_block->writeNextBlock(tmp_file_block);
#ifdef TEST_MAIN_DEBUG
    printf("read a file block.[%ld]\n", cnt_block);
#endif
    cnt_block++ ;
  }

  printf("finish read file by block.\n");

  printf("use time ms:%d.\n", GetTickCount());


  

  return 0;
}

