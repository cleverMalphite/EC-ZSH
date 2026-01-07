#ifndef __INFO_HUB__
#define __INFO_HUB__

#include <map>
#include <set>
#include <vector>
#include <cstdio>

#include "./infoHub_impl.h"

class infoHub
{
public:
    infoHub(){}
    ~infoHub(){}

    //--------------- kv -------------------
    bool    value_set(std::string module, std::string key, int value);
    bool    value_set(std::string module, std::string key, std::string value);

    bool    value_del(std::string module, std::string key);

    template<typename T>
    T       value_get(std::string module, std::string key);
    bool    value_get(std::string module, std::string key, int &value);
    bool    value_get(std::string module, std::string key, std::string &value);

    //--------------- km -------------------
    bool    table_set(std::string module, std::string key, int field, int value);
    bool    table_set(std::string module, std::string key, int field, std::string value);

    bool    table_del(std::string module, std::string key, int field);

    template<typename T>
    T       table_get(std::string module, std::string key, int field);
    bool    table_get(std::string module, std::string key, int field, int &value);
    bool    table_get(std::string module, std::string key, int field, std::string &value);

    int     table_len(std::string module, std::string key);//获取key对应的hash有多少个键值对

    //--------------- ks -------------------
    bool    set_add(std::string module, std::string key, int member);
    bool    set_add(std::string module, std::string key, std::string member);

    bool    set_del(std::string module, std::string key, int member);
    bool    set_del(std::string module, std::string key, std::string member);

    bool    set_ism(std::string module, std::string key, int member);//判断mem是否是set的成员
    bool    set_ism(std::string module, std::string key, std::string member);//判断mem是否是set的成员
            
    int     set_cnt(std::string module, std::string key);//计算集合元素的个数
            
    template<typename T>
    T       set_get(std::string module, std::string key);//返回set的全部成员
    bool    set_get(std::string module, std::string key, std::set<int> &set_int);//返回set的全部成员
    bool    set_get(std::string module, std::string key, std::set<std::string> &set_str);//返回set的全部成员

private:
    //确保全局的module-key是唯一的,不会在kv中也有而在km、ks中也有

    infoHub_impl _infohub;
};



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
infoHub::value_del(std::string module, std::string key)
{
    if( true == _infohub.kv_del_int(module, key) )  return true;
    if( true == _infohub.kv_del_str(module, key) )  return true;
    return false;
}

template<typename T>
T infoHub::value_get(std::string module, std::string key)
{
	T value;
    bool ret = value_get(module, key, value);
    return value;
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
infoHub::table_del(std::string module, std::string key, int field)
{
    if( true == _infohub.km_del_i2i(module, key, field) )  return true;
    if( true == _infohub.km_del_i2s(module, key, field) )  return true;
    return false;
}


template<typename T>
T infoHub::table_get(std::string module, std::string key, int field)
{
    T value;
    bool ret = table_get(module, key, field, value);
    return value;
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

int     
infoHub::table_len(std::string module, std::string key)
{
    int len = -1;
    len = _infohub.km_len_i2i(module, key); if( -1 != len ) return len;
    len = _infohub.km_len_i2s(module, key); if( -1 != len ) return len;
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

template<typename T>
T infoHub::set_get(std::string module, std::string key)
{
    T set_t;
    bool ret = set_get(module, key, set_t);
    return set_t;
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



#endif// __INFO_HUB__
