#include <FileBlockMsg.pb.h>
#include <ReadFileBlock.h>
#include <WriteFileBlock.h>
#include <iostream>
#include <stdio.h>

int main()
{ 
  printf("create readFileBlock\n");
  ReadFileBlock readFileBlock("makefile");

  printf("create writeFileBlock\n");
  std::unique_ptr<WriteFileBlock> p_write_file_block;

  printf("start read file by block.\n");

  uint8_t cnt_block = 0;

  hh::FileBlockMsg tmp_file_block ;
  
  printf("start read\n");
  while( readFileBlock.readNextBlock( tmp_file_block ) )
  {
    if(p_write_file_block==nullptr)
    {
      p_write_file_block = std::move(std::unique_ptr<WriteFileBlock>(
                                        new WriteFileBlock( 
                                            //tmp_file_block->FileName_, 
                                            "makefile2", 
                                            tmp_file_block.filesize_total_()
                                    )));
    }
    p_write_file_block->writeNextBlock(tmp_file_block);
    printf("read a file block.[%d]\n", cnt_block);
    cnt_block++ ;
  }

  printf("finish read file by block.\n");


  

  return 0;
}

