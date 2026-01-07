#include <map>
#include <set>
#include <vector>
#include "./infoHub.h"


infoHub::infoHub()
{}

infoHub::~infoHub()
{}

/***** [module]:key value ****/
/*包含: 
    1. map<str, int> kv_int_map int类型数据存在这里
    2. map<str, str> kv_str_map str类型数据存在这里
**/
// string:int   ==> map<str, int>
//
bool
infoHub::kv_set_int(std::string module, std::string key, int value)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="kv_int";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="kv_int")
        {
            _modulekey_kx_map_lock.LockEnd();
            printf("[kv_set_int] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_kv_int_map_lock);
    _kv_int_map[module_key] = value;
    return true;
}
bool
infoHub::kv_del_int(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_int_map_lock);
    if(_kv_int_map.find(module_key) == _kv_int_map.end())
    {
        printf("[kv_del_int] wrong: kv_del_int not found!\n");
        return false;
    }
    _kv_int_map.erase(module_key);
    return true;
}
int
infoHub::kv_get_int(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_int_map_lock);
    if(_kv_int_map.find(module_key) == _kv_int_map.end())
    {
        printf("[kv_get_int] wrong: kv_get_int not found!\n");
        return -1;
    }
    return _kv_int_map[module_key];
}

// string:string   ==> map<str, str>
bool
infoHub::kv_set_str(std::string module, std::string key, std::string value)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="kv_str";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="kv_str")
        {
            _modulekey_kx_map_lock.LockEnd();
            printf("[kv_set_str] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_kv_str_map_lock);
    _kv_str_map[module_key] = value;
    return true;
}
bool
infoHub::kv_del_str(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_str_map_lock);
    if(_kv_str_map.find(module_key) == _kv_str_map.end())
    {
        printf("[kv_del_str] wrong: kv_del_str not found!\n");
        return false ;
    }
    _kv_str_map.erase(module_key);
    return true;
}
std::string
infoHub::kv_get_str(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_str_map_lock);
    if(_kv_str_map.find(module_key) == _kv_str_map.end())
    {
        printf("[kv_get_str] wrong: kv_get_str not found!\n");
        return "";
    }
    return _kv_str_map[module_key];
}




/***** [module]:key map ****/
/*包含: 
    1. map<str, map<int, int>> km_intint_map int:int类型数据存在这里
    2. map<str, map<int, str>> km_intstr_map str:str类型数据存在这里
**/
// map: int:int
bool
infoHub::km_set_i2i(std::string module, std::string key, int field, int value)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="km_i2i";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="km_i2i")
        {
            _modulekey_kx_map_lock.LockEnd();
            printf("[km_set_i2i] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_km_intint_map_lock);
    _km_intint_map[module_key][field] = value;
    return true;
}
bool
infoHub::km_del_i2i(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intint_map_lock);
    if(_km_intint_map.find(module_key) == _km_intint_map.end())
    {
        printf("[km_del_i2i] wrong: km_del_i2i:module_key not found!\n");
        return false;
    }
    if(_km_intint_map[module_key].find(field) == _km_intint_map[module_key].end())
    {
        printf("[km_del_i2i] wrong: km_del_i2i:field not found!\n");
        return false;
    }
    //删除键为module_key下的键为field的元素
    _km_intint_map[module_key].erase(field);
    //删除整个module_key及所有子元素
    //_km_intint_map.erase(module_key);
    return true;
}
int
infoHub::km_get_i2i(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intint_map_lock);
    if(_km_intint_map.find(module_key) == _km_intint_map.end())
    {
        printf("[km_get_i2i] wrong: km_get_i2i:module_key not found!\n");
        return -1;
    }
    if(_km_intint_map[module_key].find(field) == _km_intint_map[module_key].end())
    {
        printf("[km_get_i2i] wrong: km_get_i2i:field not found!\n");
        return -1;
    }
    return _km_intint_map[module_key][field];
}
//获取key对应的hash有多少个键值对
int
infoHub::km_len_i2i(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intint_map_lock);
    if(_km_intint_map.find(module_key) == _km_intint_map.end())
    {
        printf("[km_len_i2i] wrong: km_len_i2i:module_key not found!\n");
        return -1;
    }
    return _km_intint_map[module_key].size();
}

// map: int:str
bool
infoHub::km_set_i2s(std::string module, std::string key, int field, std::string value)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="km_i2s";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="km_i2s")
        {
            _modulekey_kx_map_lock.LockEnd();
            printf("[km_set_i2s] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_km_intstr_map_lock);
    _km_intstr_map[module_key][field] = value;
    return true;
}
bool
infoHub::km_del_i2s(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intstr_map_lock);
    if(_km_intstr_map.find(module_key) == _km_intstr_map.end())
    {
        printf("[km_del_i2s] wrong: km_del_i2s:module_key not found!\n");
        return false;
    }
    if(_km_intstr_map[module_key].find(field) == _km_intstr_map[module_key].end())
    {
        printf("[km_del_i2s] wrong: km_del_i2s:field not found!\n");
        return false;
    }
    //删除键为module_key下的键为field的元素
    _km_intstr_map[module_key].erase(field);
    //删除整个module_key及所有子元素
    //_km_intstr_map.erase(module_key);
    return true;
}
std::string
infoHub::km_get_i2s(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intstr_map_lock);
    if(_km_intstr_map.find(module_key) == _km_intstr_map.end())
    {
        printf("[km_get_i2s] wrong: km_get_i2s:module_key not found!\n");
        return "";
    }
    if(_km_intstr_map[module_key].find(field) == _km_intstr_map[module_key].end())
    {
        printf("[km_get_i2s] wrong: km_get_i2s:field not found!\n");
        return "";
    }
    return _km_intstr_map[module_key][field];
}
//获取key对应的hash有多少个键值对
int
infoHub::km_len_i2s(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intstr_map_lock);
    if(_km_intstr_map.find(module_key) == _km_intstr_map.end())
    {
        printf("[km_len_i2s] wrong: km_len_i2i:module_key not found!\n");
        return -1;
    }
    return _km_intstr_map[module_key].size();
}




/***** [module]:key set ****/
/*包含: 
    1. map<str, set<int>> ks_intset_map intset类型数据存在这里
    2. map<str, set<str>> ks_strset_map strset类型数据存在这里
**/
// set: int a, int b, int c ...
bool
infoHub::ks_add_int(std::string module, std::string key, int member)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="ks_int";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="ks_int")
        {
            _modulekey_kx_map_lock.LockEnd();
            printf("[ks_add_int] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_ks_intset_map_lock);
    _ks_intset_map[module_key].insert(member);
    return true;
}
bool
infoHub::ks_del_int(std::string module, std::string key, int member)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_intset_map_lock);
    if(_ks_intset_map.find(module_key)==_ks_intset_map.end())
    {
        printf("[ks_del_int] wrong ks_del_int not find module_key\n");
        return false;
    }
    if(_ks_intset_map[module_key].find(member)==_ks_intset_map[module_key].end())
    {
        printf("[ks_del_int] wrong ks_del_int not find member\n");
        return false;
    }
    _ks_intset_map[module_key].erase(member);
    return true;
}
//判断mem是否是set的成员
bool
infoHub::ks_ism_int(std::string module, std::string key, int member)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_intset_map_lock);
    if(_ks_intset_map.find(module_key)==_ks_intset_map.end())return false;
    if(_ks_intset_map[module_key].find(member)==_ks_intset_map[module_key].end())return false;
    else return true;
}
//计算集合元素的个数
int
infoHub::ks_cnt_int(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_intset_map_lock);
    if(_ks_intset_map.find(module_key)==_ks_intset_map.end())
    {
        printf("ks_cnt_int not find module_key\n");
        return -1;
    }
    return _ks_intset_map[module_key].size();
}
//返回set的全部成员
std::set<int>
infoHub::ks_shw_int(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_intset_map_lock);
    if(_ks_intset_map.find(module_key)==_ks_intset_map.end())
    {
        printf("ks_cnt_int not find module_key\n");
        std::set<int> nullset;
        return nullset;
    }
    return _ks_intset_map[module_key];
}

// set: str a, str b, str c ...
bool
infoHub::ks_add_str(std::string module, std::string key, std::string member)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="ks_str";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="ks_str")
        {
            _modulekey_kx_map_lock.LockEnd();
            printf("[ks_add_str] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_ks_strset_map_lock);
    _ks_strset_map[module_key].insert(member);
    return true;
}
bool
infoHub::ks_del_str(std::string module, std::string key, std::string member)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_strset_map_lock);
    if(_ks_strset_map.find(module_key)==_ks_strset_map.end())
    {
        printf("[ks_del_str] wrong ks_del_int not find module_key\n");
        return false;
    }
    if(_ks_strset_map[module_key].find(member)==_ks_strset_map[module_key].end())
    {
        printf("[ks_del_str] wrong ks_del_int not find member\n");
        return false;
    }
    _ks_strset_map[module_key].erase(member);
    return true;
}
//判断mem是否是set的成员
bool
infoHub::ks_ism_str(std::string module, std::string key, std::string member)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_strset_map_lock);
    if(_ks_strset_map.find(module_key)==_ks_strset_map.end())return false;
    if(_ks_strset_map[module_key].find(member)==_ks_strset_map[module_key].end())return false;
    else return true;
}
//计算集合元素的个数
int
infoHub::ks_cnt_str(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_strset_map_lock);
    if(_ks_strset_map.find(module_key)==_ks_strset_map.end())
    {
        printf("ks_cnt_int not find module_key\n");
        return -1;
    }
    return _ks_strset_map[module_key].size();
}
//返回set的全部成员
std::set<std::string>
infoHub::ks_shw_str(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_strset_map_lock);
    if(_ks_strset_map.find(module_key)==_ks_strset_map.end())
    {
        printf("ks_cnt_int not find module_key\n");
        std::set<std::string> nullset;
        return nullset;
    }
    return _ks_strset_map[module_key];
}
