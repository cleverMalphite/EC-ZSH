#ifndef __PROGRESS_STATISTIC_TABLE_H__
#define __PROGRESS_STATISTIC_TABLE_H__

#include <sys/time.h>
#include <functional>

#include "./progressStatistic.h"
#include "./infoHub.h"

namespace ec2{

class progressStatisticTable{
public:
    progressStatisticTable()=default;
    //module, 模块名
    //key, 表名
    progressStatisticTable(std::string module_name, std::string key_name){
        dd::AutoLock      autolock(_inner_lock);
        _module_name = module_name;
        _key_name = key_name;
    }

    void init(std::string module_name, std::string key_name){
        dd::AutoLock      autolock(_inner_lock);
        _module_name = module_name;
        _key_name = key_name;
    }

    //初始化，设置某个任务号任务，对应的任务总大小
    bool begin(int cid, uint64_t size_total){
        dd::AutoLock      autolock(_inner_lock);
        if(-1==cid){return false;}
        if(_cid_rs_map.find(cid) == _cid_rs_map.end()){
            _cid_rs_map[cid] = std::shared_ptr<progressStatistic>(new progressStatistic());
            _cid_rs_map[cid]->init(_module_name, _key_name, cid);
        }
        _cid_rs_map[cid]->begin(size_total);
        return true;
    }

    double pass(int cid, uint64_t size_did){
        dd::AutoLock      autolock(_inner_lock);
        if(-1 == cid){return -1.0;}
        if(size_did < 0){return -1.0;}
        if(_cid_rs_map.find(cid) == _cid_rs_map.end()){
            return -1;
        }
        double rate = _cid_rs_map[cid]->pass(size_did);
        return rate;
    }

private:
    dd::Lock      _inner_lock;
    std::string _module_name;
    std::string _key_name;
    //ratreStatistic中进行infohub写操作
    std::map<int, std::shared_ptr<progressStatistic>> _cid_rs_map;
};
}//namespace ec2
 
#endif //#define __RATE_STATISTIC_TABLE_H__
