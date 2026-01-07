#pragma once

#include "./FileBlock.h"
#include "./msg_cpp/FileBlockMsg.pb.h"
#include <sys/stat.h> // struct stat
#include <stdio.h>
#include <memory>
#include <string>
#include <vector>
#include <string.h>
  

//#define FILE_BLOCK_SINGLE_SIZE 1024 //byte
                      
// 通过stat结构体 获得文件大小，单位字节
size_t getFileSize_by_Stat(const char *fileName) {

	if (fileName == NULL) {
		return 0;
	}
	
	// 这是一个存储文件(夹)信息的结构体，其中有文件大小和创建时间、访问时间、修改时间等
	struct stat statbuf;

	// 提供文件名字符串，获得文件属性结构体
	stat(fileName, &statbuf);
	
	// 获取文件大小
	size_t filesize = statbuf.st_size;

	return filesize;
}

class ReadFileBlock
{
public:
  ReadFileBlock(std::string filename,  
                uint32_t single_read_cnt=3,
                uint32_t single_block_size=FILE_BLOCK_SINGLE_SIZE) 
  {
    filename_ = filename;
    file_block_size_ = single_block_size;
    file_read_cnt_ = single_read_cnt;

    fp_ = fopen(filename_.c_str(), "rb+") ;
    if(fp_ == NULL)
    {
      //wrong;
    }
    file_size_ = getFileSize_by_Stat(filename_.c_str());

    file_block_number_total_ = file_size_ / file_block_size_;
    if(file_block_number_total_ * file_block_size_ < file_size_)
      file_block_number_total_ = file_block_number_total_ + 1 ;

    count_current_block_number_ = 0;//read from first block 
  
  }

  bool readNextBlock (hh::FileBlockMsg &tmp_file_block)
  {
    if(count_current_block_number_+1>file_block_number_total_)
      return false; 
    
    if(v_file_buffer_cnt_==0)
    {
        v_file_buffer_cnt_ = file_read_cnt_*file_block_size_;
        v_file_buffer_.resize(v_file_buffer_cnt_);
        size_t read_size = fread(&v_file_buffer_[0], 1, v_file_buffer_cnt_, fp_);
        if(read_size < v_file_buffer_cnt_)
        {
          v_file_buffer_.resize(read_size);
          v_file_buffer_cnt_ = read_size;
        }
        else if(read_size==0) return false;
    }


    {
        tmp_file_block.set_fileid_(0);
        tmp_file_block.set_fileblocknumber_(count_current_block_number_+1);
        tmp_file_block.set_fileblocknumber_total_(file_block_number_total_);
        tmp_file_block.set_filesize_total_(file_size_);
        tmp_file_block.set_filename_(filename_);

        if(v_file_buffer_cnt_ > file_block_size_)
        {
          tmp_file_block.set_fileblockbuffer_(
                              &v_file_buffer_[0] + v_file_buffer_.size() - v_file_buffer_cnt_, 
                              file_block_size_ );
          v_file_buffer_cnt_ = v_file_buffer_cnt_ - file_block_size_;

        }else{
          tmp_file_block.set_fileblockbuffer_(
                              &v_file_buffer_[0] + v_file_buffer_.size() - v_file_buffer_cnt_, 
                              v_file_buffer_cnt_ );
          v_file_buffer_cnt_ = 0;
        }

        tmp_file_block.set_fileblocksize_( tmp_file_block.fileblockbuffer_().size() );

        count_current_block_number_++;
    }

    return true;
  }

  ~ReadFileBlock()
  {
    fclose(fp_);
  }

public:
  FILE* fp_;
  std::string filename_;
  uint64_t file_size_;
  
  uint64_t file_block_size_;

  uint64_t file_read_cnt_;
  std::vector<uint8_t> v_file_buffer_;
  int v_file_buffer_cnt_ = 0;

  uint64_t file_block_number_total_;

  uint64_t count_current_block_number_;//1->n
};

