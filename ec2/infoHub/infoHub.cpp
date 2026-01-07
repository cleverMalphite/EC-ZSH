#include "./infoHub.h"

#include "../../ec2/public/utility.h"
#include "./infoHub_impl.h"


namespace ec2{

static std::shared_ptr<infoHub> _instance = nullptr;
static dd::Lock _instance_lock;

//获取实例的静态方法
std::shared_ptr<infoHub> 
infoHub::getInstance(){
    //只有在第一次调用时，才回初始化实例
    if(nullptr==_instance){
        dd::AutoLock autolock(_instance_lock);
        //双重检查，确保只有一个实例被创建
        if(nullptr==_instance){
            _instance = std::shared_ptr<infoHub>(new infoHub());
        }
    }
    return _instance;
}

//--------------- kv -------------------
bool    
infoHub::value_set(std::string module, std::string key, int value)
{
    return _infohub.kv_set_int(module, key, value);
}
bool    
infoHub::value_set(std::string module, std::string key, std::string value)
{
    return _infohub.kv_set_str(module, key, value);
}
bool    
infoHub::value_set(std::string module, std::string key, double value)
{
    return _infohub.kv_set_lf(module, key, value);
}
bool    
infoHub::value_set(std::string module, std::string key, uint32_t value)
{
    return _infohub.kv_set_u32(module, key, value);
}

bool    
infoHub::value_del(std::string module, std::string key)
{
    if( true == _infohub.kv_del_int(module, key) )  return true;
    if( true == _infohub.kv_del_str(module, key) )  return true;
    if( true == _infohub.kv_del_lf(module, key) )  return true;
    if( true == _infohub.kv_del_u32(module, key) )  return true;
    return false;
}

//return -1 if wrong
bool
infoHub::value_get(std::string module, std::string key, int &value)
{
    value = _infohub.kv_get_int(module, key);
    if(-1!=value) return true;
    return false;
}
// return "" if wrong
bool
infoHub::value_get(std::string module, std::string key, std::string &value)
{
    value = _infohub.kv_get_str(module, key);
    if(""!=value) return true;
    return false;
}
//return -1 if wrong
bool
infoHub::value_get(std::string module, std::string key, double &value)
{
    value = _infohub.kv_get_lf(module, key);
    if(-1.0!=value) return true;
    return false;
}
//return -1 if wrong
bool
infoHub::value_get(std::string module, std::string key, uint32_t &value)
{
    value = _infohub.kv_get_u32(module, key);
    if(-1!=value) return true;
    return false;
}

//------::--------- km -------------------
bool    
infoHub::table_set(std::string module, std::string key, int field, int value)
{
    return _infohub.km_set_i2i(module, key, field, value);

}
bool    
infoHub::table_set(std::string module, std::string key, int field, std::string value)
{
    return _infohub.km_set_i2s(module, key, field, value);
}
bool    
infoHub::table_set(std::string module, std::string key, int field, double value)
{
    return _infohub.km_set_i2lf(module, key, field, value);
}
bool    
infoHub::table_set(std::string module, std::string key, int field, uint32_t value)
{
    return _infohub.km_set_i2u32(module, key, field, value);
}

bool    
infoHub::table_del(std::string module, std::string key, int field)
{
    if( true == _infohub.km_del_i2i(module, key, field) )  return true;
    if( true == _infohub.km_del_i2s(module, key, field) )  return true;
    if( true == _infohub.km_del_i2lf(module, key, field) )  return true;
    if( true == _infohub.km_del_i2u32(module, key, field) )  return true;
    return false;
}


bool
infoHub::table_get(std::string module, std::string key, int field, int &value)
{
    value = _infohub.km_get_i2i(module, key, field);
    if(-1!=value) return true;
    return false;
}
bool
infoHub::table_get(std::string module, std::string key, int field, std::string &value)
{
    value = _infohub.km_get_i2s(module, key, field);
    if(""!=value) return true;
    return false;
}
bool
infoHub::table_get(std::string module, std::string key, int field, double &value)
{
    value = _infohub.km_get_i2lf(module, key, field);
    if(-1.0!=value) return true;
    return false;
}
bool
infoHub::table_get(std::string module, std::string key, int field, uint32_t &value)
{
    value = _infohub.km_get_i2u32(module, key, field);
    if(-1!=value) return true;
    return false;
}

int     
infoHub::table_len(std::string module, std::string key)
{
    int len = -1;
    len = _infohub.km_len_i2i(module, key); if( -1 != len ) return len;
    len = _infohub.km_len_i2s(module, key); if( -1 != len ) return len;
    len = _infohub.km_len_i2lf(module, key); if( -1 != len ) return len;
    len = _infohub.km_len_i2u32(module, key); if( -1 != len ) return len;
    return -1;
}

//--------------- ks -------------------
bool    
infoHub::set_add(std::string module, std::string key, int member)
{
    return _infohub.ks_add_int(module, key, member);
}
bool    
infoHub::set_add(std::string module, std::string key, std::string member)
{
    return _infohub.ks_add_str(module, key, member);
}

bool    
infoHub::set_del(std::string module, std::string key, int member)
{
    return _infohub.ks_del_int(module, key, member);
}
bool    
infoHub::set_del(std::string module, std::string key, std::string member)
{
    return _infohub.ks_del_str(module, key, member);
}

bool    
infoHub::set_ism(std::string module, std::string key, int member)
{
    return _infohub.ks_ism_int(module, key, member);
}
bool    
infoHub::set_ism(std::string module, std::string key, std::string member)
{
    return _infohub.ks_ism_str(module, key, member);
}
        
int     
infoHub::set_cnt(std::string module, std::string key)
{
    int cnt = -1;
    cnt = _infohub.ks_cnt_int(module, key); if(-1!=cnt)return cnt;
    cnt = _infohub.ks_cnt_str(module, key); if(-1!=cnt)return cnt;
    return -1;
}

//set_get.size=0 if wrong
bool
infoHub::set_get(std::string module, std::string key, std::set<int> &set_int)
{
    set_int = _infohub.ks_get_int(module, key);
    if(0!=set_int.size()) return true;
    return false;
}
//set_get.size=0 if wrong
bool
infoHub::set_get(std::string module, std::string key, std::set<std::string> &set_str)
{
    set_str = _infohub.ks_get_str(module, key);
    if(0!=set_str.size()) return true;
    return false;
}

}//namespace ec2

