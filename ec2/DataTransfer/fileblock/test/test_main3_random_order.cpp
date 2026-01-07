#include "FileBlock.h"
#include "ReadFileBlock.h"
#include "WriteFileBlock.h"
#include <iostream>
#include <stdio.h>

int main()
{ 
  std::string filename = "test_file.txt";
  std::string filename2 = "test_file2.txt";

  ReadFileBlock readFileBlock(filename);

  std::unique_ptr<WriteFileBlock> p_write_file_block;

  printf("start read file by block.\n");

  uint8_t cnt_block = 0;

  hh::FileBlockMsg tmp_file_block ;
  
  while( readFileBlock.readNextBlock( tmp_file_block ) )
  {
    if(p_write_file_block==nullptr)
    {
      p_write_file_block = std::move(std::unique_ptr<WriteFileBlock>(
                                        new WriteFileBlock( 
                                            //tmp_file_block->FileName_, 
                                            filename2, 
                                            tmp_file_block.filesize_total_()
                                    )));
    }

    if(cnt_block%2==0)
      p_write_file_block->writeNextBlock(tmp_file_block);
    printf("read a file block.[%d]\n", cnt_block);
    cnt_block++ ;
  }



  printf("finish read file by block.\n");


  

  return 0;
}

