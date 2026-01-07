#pragma once

#include "./FileBlock.h"
#include "./msg_cpp/FileBlockMsg.pb.h"
#include <sys/stat.h> // struct stat
#include <stdio.h>
#include <memory>
#include <string>


  
//#define FILE_BLOCK_SINGLE_SIZE 1024 //byte
                      
class WriteFileBlock
{
public:
    WriteFileBlock(){
        has_opened = false;
    }
    WriteFileBlock(const WriteFileBlock& writeFileBlock){
        fp_                             = writeFileBlock.fp_;
        filename_                       = writeFileBlock.filename_;
        filename_tmp_                   = writeFileBlock.filename_tmp_;

        is_finish_                      = writeFileBlock.is_finish_;
        file_size_                      = writeFileBlock.file_size_;
        file_block_size_                = writeFileBlock.file_block_size_;
        file_block_number_total_        = writeFileBlock.file_block_number_total_;
        count_current_block_number_     = writeFileBlock.count_current_block_number_;//1->n
    }
  WriteFileBlock(std::string filename, uint64_t write_file_size,  
                 uint32_t single_write_cnt = 3,
                uint32_t single_block_size=FILE_BLOCK_SINGLE_SIZE) 
  {
      open(filename, write_file_size, single_write_cnt, single_block_size);
  }

  void open(std::string filename, uint64_t write_file_size,  
                 uint32_t single_write_cnt = 3,
                uint32_t single_block_size=FILE_BLOCK_SINGLE_SIZE) 
  {
    if(has_opened == true)return ;
    has_opened = true;

    is_finish_ = false;
    filename_ = filename;
    filename_tmp_ = filename+".tmp";

    file_block_size_ = single_block_size;

    //fp_ = fopen(filename_.c_str(), "wb+") ; // not use tmp file
    fp_ = fopen(filename_tmp_.c_str(), "wb+") ; // cover origin file
    //fp_ = fopen(filename_tmp_.c_str(), "ab+") ; // not cover origin file

    if(fp_ == NULL)
    {
      //wrong;
    }
    file_size_ = write_file_size;

    file_block_number_total_ = file_size_ / file_block_size_;
    if(file_block_number_total_ * file_block_size_ < file_size_)
      file_block_number_total_ = file_block_number_total_ + 1 ;

    count_current_block_number_ = 0;//read from first block 
  }

  bool writeNextBlock(hh::FileBlockMsg& p_file_block )
  {
    fseek(fp_, (p_file_block.fileblocknumber_()-1)*file_block_size_, SEEK_SET);
    fwrite(&p_file_block.fileblockbuffer_()[0], 1, p_file_block.fileblockbuffer_().size(), fp_);
    return true;
  }

  void flushWriteFileBlock()
  {
      fflush(fp_);
  }

  void finishWriteFileBlock()
  {
    if(is_finish_) return ;
    else is_finish_ = true;

    fclose(fp_);
    rename(filename_tmp_.c_str(), filename_.c_str());
  }
  ~WriteFileBlock()
  {
      finishWriteFileBlock();
  }

public:
  bool has_opened;
  FILE* fp_;
  std::string filename_;
  std::string filename_tmp_;

  bool is_finish_;
  uint64_t file_size_;
  uint64_t file_block_size_;
  uint64_t file_block_number_total_;
  uint64_t count_current_block_number_;//1->n
};

