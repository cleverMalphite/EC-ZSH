
#ifndef __PROGRESS_STATISTIC_H__
#define __PROGRESS_STATISTIC_H__

#include <sys/time.h>
#include <functional>

#include "./infoHub.h"
#include "../public/utility.h"

namespace ec2{



//usage:
// 1. when create Rate instance, timer start
// 2. when call pass(size_passed), return current rate.

class progressStatistic
{
public:
  progressStatistic() = default;
  ~progressStatistic() = default;

  //module_name 模块名
  //key_name    表名
  //cid         表段号
  //*cid=-1时，表示这是一个value类型的统计量
  //*cid>=0时，表示这是一个table类型的统计量
  progressStatistic(std::string module_name, std::string key_name, int cid=-1){
    dd::AutoLock      autolock(_inner_lock);
    _ps_infohub_instance = infoHub::getInstance();
    _module_name = module_name;
    _key_name = key_name;
    _field = cid;

    _total_size = 1;
    _did_size = 0;
    _progress = 0;  // 单位 %
  }

  void init(std::string module_name, std::string key_name, int cid=-1){
    dd::AutoLock      autolock(_inner_lock);
    _ps_infohub_instance = infoHub::getInstance();
    _module_name = module_name;
    _key_name = key_name;
    _field = cid;

    _total_size = 1;
    _did_size = 0;
    _progress = 0;  // 单位 %
  }


  // 开始时，要设置任务的总大小
  void begin(uint64_t size_total) {
    dd::AutoLock      autolock(_inner_lock);
    if(_isbegin==false){
        _isbegin = true;
        _total_size = size_total;
        //_total_size = 1;
        _did_size = 0;
        _progress = 0;  // 单位 %
    }
  }

  double pass(uint64_t size_did){
    dd::AutoLock      autolock(_inner_lock);
    _did_size = _did_size +  size_did;
    _progress = ( 100.0 * (double)_did_size / (double)_total_size ); 

    if(-1==_field){
        _ps_infohub_instance->value_set(_module_name, _key_name, _progress);
    }else{
        _ps_infohub_instance->table_set(_module_name, _key_name, _field, _progress);
    }

    return _progress;
  }

private:
  dd::Lock      _inner_lock;
  //infohub
  std::shared_ptr<infoHub> _ps_infohub_instance;
  //init
  bool          _isbegin=false;
  //key
  std::string   _module_name;
  std::string   _key_name;
  int           _field;
  //prograss params
  uint64_t      _total_size = 1;
  uint64_t      _did_size = 0;
  double        _progress = 0;  // 单位 %

};


}// namespace ec2
 
#endif //#define __RATE_STATISTIC_H__
