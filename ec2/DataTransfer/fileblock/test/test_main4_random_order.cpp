#include "FileBlock.h"
#include "ReadFileBlock.h"
#include "WriteFileBlock.h"
#include <iostream>
#include <stdio.h>

//#define TEST_MAIN4_RANDOM_ORDER_DEBUG

int main()
{ 
  std::string filename = "test_file.txt";
  std::string filename2 = "test_file2.txt";

  std::unique_ptr<WriteFileBlock> p_write_file_block;
  hh::FileBlockMsg tmp_file_block ;
  
  uint8_t cnt_block = 0;

  {
    ReadFileBlock readFileBlock(filename, 100);


    printf("start read file by block.\n");


    cnt_block = 0;
    
    while( readFileBlock.readNextBlock( tmp_file_block ) )
    {
      if(p_write_file_block==nullptr)
      {
        p_write_file_block = std::move(std::unique_ptr<WriteFileBlock>(
                                          new WriteFileBlock( 
                                              //tmp_file_block->FileName_, 
                                              filename2, 
                                              tmp_file_block.filesize_total_(),
                                              100
                                      )));
      }

      if(cnt_block%2==1)
        p_write_file_block->writeNextBlock(tmp_file_block);
#ifdef TEST_MAIN4_RANDOM_ORDER_DEBUG
      printf("read a file block.[%d]\n", cnt_block);
#endif
      cnt_block++ ;
    }

    printf("finish read file by block.\n");
  }



  {
    ReadFileBlock readFileBlock2(filename, 100);
    cnt_block = 0;
    while( readFileBlock2.readNextBlock( tmp_file_block ) )
    {
      if(p_write_file_block==nullptr)
      {
        p_write_file_block = std::move(std::unique_ptr<WriteFileBlock>(
                                          new WriteFileBlock( 
                                              //tmp_file_block->FileName_, 
                                              filename2, 
                                              tmp_file_block.filesize_total_(), 
                                              100
                                      )));
      }

      if(cnt_block%2==0)
        p_write_file_block->writeNextBlock(tmp_file_block);
#ifdef TEST_MAIN4_RANDOM_ORDER_DEBUG
      printf("read a file block.[%d]\n", cnt_block);
#endif
      cnt_block++ ;
    }
    printf("finish read file by block.\n");
  }
  

  return 0;
}

