
#ifndef __RATE_STATISTIC_H__
#define __RATE_STATISTIC_H__

#include <sys/time.h>
#include <functional>

#include "./infoHub.h"

#include "../public/utility.h"

namespace ec2{



//usage:
// 1. when create Rate instance, timer start
// 2. when call pass(size_passed), return current rate.

class rateStatistic
{
public:
  rateStatistic() = default;
  ~rateStatistic() = default;

  static uint64_t TimeGetTimeStampNow_us() {
      timeval tv; 
      gettimeofday(&tv, nullptr);
      return tv.tv_sec * 1000000 + tv.tv_usec;
  }

  //module_name 模块名
  //key_name    表名
  //cid         表段号
  //*cid=-1时，表示这是一个value类型的统计量
  //*cid>=0时，表示这是一个table类型的统计量
  rateStatistic(std::string module_name, std::string key_name, int cid=-1){
    dd::AutoLock      autolock(_inner_lock);
    _rs_infohub_instance = infoHub::getInstance();
    _module_name = module_name;
    _key_name = key_name;
    _field = cid;
  }
  void init(std::string module_name, std::string key_name, int cid=-1){
    dd::AutoLock      autolock(_inner_lock);
    _rs_infohub_instance = infoHub::getInstance();
    _module_name = module_name;
    _key_name = key_name;
    _field = cid;
  }

  //开始统计
  void begin(){
    dd::AutoLock      autolock(_inner_lock);
    if(false == _isbegin) {
        _isbegin = true;

        _rate_ = 0.0;
        _total_size_passed_ = 0;
        _begin_time_us_ = TimeGetTimeStampNow_us();
        _begin_time_ms_ = _begin_time_us_ / 1000;

        if(-1==_field){
            _rs_infohub_instance->value_set(_module_name, _key_name, _rate_);
        }else{
            _rs_infohub_instance->table_set(_module_name, _key_name, _field, _rate_);
        }
    }
  }

  //每次传数据时调用，及时更新统计传输速率的变化
  //不传数据时，获取传输速率时通过参数为0，获取实时速率
  //计算的_rate_是每ms的速率，而对于是byte/ms，还是bit/ms，取决于调用时输入的size_passed的单位.
  double pass(uint64_t size_passed){
    dd::AutoLock      autolock(_inner_lock);
    _total_size_passed_ += size_passed;
    _rate_ =  _total_size_passed_ / ((double)TimeGetTimeStampNow_us() / 1000.0 - (double)_begin_time_ms_);
    if(-1==_field){
        _rs_infohub_instance->value_set(_module_name, _key_name, _rate_);
    }else{
        _rs_infohub_instance->table_set(_module_name, _key_name, _field, _rate_);
    }

    return _rate_;
  }

private:
  dd::Lock      _inner_lock;
  //statistic params
  uint64_t      _total_size_passed_ = 0;
  uint64_t      _begin_time_us_ = 0;  
  uint64_t      _begin_time_ms_ = 0;  
  double        _rate_ = 0; 
  //key
  std::string   _module_name;
  std::string   _key_name;
  int           _field;
  //init
  bool          _isbegin=false;
  //infohub
  std::shared_ptr<infoHub> _rs_infohub_instance;
};


}// namespace ec2
 
#endif //#define __RATE_STATISTIC_H__
