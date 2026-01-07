#include "./infoHub_impl.h"



#include <map>
#include <set>
#include <vector>
#include <cstdio>

#include "../../ec2/public/utility.h"


// 定义一个宏，用于调试打印
#ifdef DEBUG_INFO_HUB_IMPL
//#define DEBUG_PRINT(x) std::cout << x << std::endl
#define DEBUG_PRINT(x) printf( x )
#else
#define DEBUG_PRINT(x) (void)0
#endif


infoHub_impl::infoHub_impl()
{}

infoHub_impl::~infoHub_impl()
{}

/***** [module]:key value ****/
/*包含: 
    1. map<str, int> kv_int_map int类型数据存在这里
    2. map<str, str> kv_str_map str类型数据存在这里
**/
// string:int   ==> map<str, int>
bool
infoHub_impl::kv_set_int(std::string module, std::string key, int value)
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
            DEBUG_PRINT("[kv_set_int] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_kv_int_map_lock);
    _kv_int_map[module_key] = value;
    return true;
}
bool
infoHub_impl::kv_del_int(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_int_map_lock);
    if(_kv_int_map.find(module_key) == _kv_int_map.end())
    {
        DEBUG_PRINT("[kv_del_int] wrong: kv_del_int not found!\n");
        return false;
    }
    _kv_int_map.erase(module_key);
    return true;
}
int
infoHub_impl::kv_get_int(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_int_map_lock);
    if(_kv_int_map.find(module_key) == _kv_int_map.end())
    {
        DEBUG_PRINT("[kv_get_int] wrong: kv_get_int not found!\n");
        return -1;
    }
    return _kv_int_map[module_key];
}

// string:string   ==> map<str, str>
bool
infoHub_impl::kv_set_str(std::string module, std::string key, std::string value)
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
            DEBUG_PRINT("[kv_set_str] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_kv_str_map_lock);
    _kv_str_map[module_key] = value;
    return true;
}
bool
infoHub_impl::kv_del_str(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_str_map_lock);
    if(_kv_str_map.find(module_key) == _kv_str_map.end())
    {
        DEBUG_PRINT("[kv_del_str] wrong: kv_del_str not found!\n");
        return false ;
    }
    _kv_str_map.erase(module_key);
    return true;
}
std::string
infoHub_impl::kv_get_str(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_str_map_lock);
    if(_kv_str_map.find(module_key) == _kv_str_map.end())
    {
        DEBUG_PRINT("[kv_get_str] wrong: kv_get_str not found!\n");
        return "";
    }
    return _kv_str_map[module_key];
}

// string:double   ==> map<str, double>
bool
infoHub_impl::kv_set_lf(std::string module, std::string key, double value)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="kv_lf";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="kv_lf")
        {
            _modulekey_kx_map_lock.LockEnd();
            DEBUG_PRINT("[kv_set_lf] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_kv_lf_map_lock);
    _kv_lf_map[module_key] = value;
    return true;
}
bool
infoHub_impl::kv_del_lf(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_lf_map_lock);
    if(_kv_lf_map.find(module_key) == _kv_lf_map.end())
    {
        DEBUG_PRINT("[kv_del_lf] wrong: kv_del_int not found!\n");
        return false;
    }
    _kv_lf_map.erase(module_key);
    return true;
}
double
infoHub_impl::kv_get_lf(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_lf_map_lock);
    if(_kv_lf_map.find(module_key) == _kv_lf_map.end())
    {
        DEBUG_PRINT("[kv_get_lf] wrong: kv_get_lf not found!\n");
        return -1.0;
    }
    return _kv_lf_map[module_key];
}

// string:uint32_t   ==> map<str, uint32_t>
bool
infoHub_impl::kv_set_u32(std::string module, std::string key, uint32_t value)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="kv_u32";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="kv_u32")
        {
            _modulekey_kx_map_lock.LockEnd();
            DEBUG_PRINT("[kv_set_u32] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_kv_u32_map_lock);
    _kv_u32_map[module_key] = value;
    return true;
}
bool
infoHub_impl::kv_del_u32(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_u32_map_lock);
    if(_kv_u32_map.find(module_key) == _kv_u32_map.end())
    {
        DEBUG_PRINT("[kv_del_u32] wrong: kv_del_int not found!\n");
        return false;
    }
    _kv_u32_map.erase(module_key);
    return true;
}
uint32_t
infoHub_impl::kv_get_u32(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_kv_u32_map_lock);
    if(_kv_u32_map.find(module_key) == _kv_u32_map.end())
    {
        DEBUG_PRINT("[kv_get_u32] wrong: kv_get_u32 not found!\n");
        return -1;
    }
    return _kv_u32_map[module_key];
}





/***** [module]:key map ****/
/*包含: 
    1. map<str, map<int, int>> km_intint_map int:int类型数据存在这里
    2. map<str, map<int, str>> km_intstr_map str:str类型数据存在这里
**/
// map: int:int
bool
infoHub_impl::km_set_i2i(std::string module, std::string key, int field, int value)
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
            DEBUG_PRINT("[km_set_i2i] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_km_intint_map_lock);
    _km_intint_map[module_key][field] = value;
    return true;
}
bool
infoHub_impl::km_del_i2i(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intint_map_lock);
    if(_km_intint_map.find(module_key) == _km_intint_map.end())
    {
        DEBUG_PRINT("[km_del_i2i] wrong: km_del_i2i:module_key not found!\n");
        return false;
    }
    if(_km_intint_map[module_key].find(field) == _km_intint_map[module_key].end())
    {
        DEBUG_PRINT("[km_del_i2i] wrong: km_del_i2i:field not found!\n");
        return false;
    }
    //删除键为module_key下的键为field的元素
    _km_intint_map[module_key].erase(field);
    //删除整个module_key及所有子元素
    //_km_intint_map.erase(module_key);
    return true;
}
int
infoHub_impl::km_get_i2i(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intint_map_lock);
    if(_km_intint_map.find(module_key) == _km_intint_map.end())
    {
        DEBUG_PRINT("[km_get_i2i] wrong: km_get_i2i:module_key not found!\n");
        return -1;
    }
    if(_km_intint_map[module_key].find(field) == _km_intint_map[module_key].end())
    {
        DEBUG_PRINT("[km_get_i2i] wrong: km_get_i2i:field not found!\n");
        return -1;
    }
    return _km_intint_map[module_key][field];
}
//获取key对应的hash有多少个键值对
int
infoHub_impl::km_len_i2i(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intint_map_lock);
    if(_km_intint_map.find(module_key) == _km_intint_map.end())
    {
        DEBUG_PRINT("[km_len_i2i] wrong: km_len_i2i:module_key not found!\n");
        return -1;
    }
    return _km_intint_map[module_key].size();
}

// map: int:str
bool
infoHub_impl::km_set_i2s(std::string module, std::string key, int field, std::string value)
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
            DEBUG_PRINT("[km_set_i2s] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_km_intstr_map_lock);
    _km_intstr_map[module_key][field] = value;
    return true;
}
bool
infoHub_impl::km_del_i2s(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intstr_map_lock);
    if(_km_intstr_map.find(module_key) == _km_intstr_map.end())
    {
        DEBUG_PRINT("[km_del_i2s] wrong: km_del_i2s:module_key not found!\n");
        return false;
    }
    if(_km_intstr_map[module_key].find(field) == _km_intstr_map[module_key].end())
    {
        DEBUG_PRINT("[km_del_i2s] wrong: km_del_i2s:field not found!\n");
        return false;
    }
    //删除键为module_key下的键为field的元素
    _km_intstr_map[module_key].erase(field);
    //删除整个module_key及所有子元素
    //_km_intstr_map.erase(module_key);
    return true;
}
std::string
infoHub_impl::km_get_i2s(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intstr_map_lock);
    if(_km_intstr_map.find(module_key) == _km_intstr_map.end())
    {
        DEBUG_PRINT("[km_get_i2s] wrong: km_get_i2s:module_key not found!\n");
        return "";
    }
    if(_km_intstr_map[module_key].find(field) == _km_intstr_map[module_key].end())
    {
        DEBUG_PRINT("[km_get_i2s] wrong: km_get_i2s:field not found!\n");
        return "";
    }
    return _km_intstr_map[module_key][field];
}
//获取key对应的hash有多少个键值对
int
infoHub_impl::km_len_i2s(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intstr_map_lock);
    if(_km_intstr_map.find(module_key) == _km_intstr_map.end())
    {
        DEBUG_PRINT("[km_len_i2s] wrong: km_len_i2i:module_key not found!\n");
        return -1;
    }
    return _km_intstr_map[module_key].size();
}

// map: int:double
bool
infoHub_impl::km_set_i2lf(std::string module, std::string key, int field, double value)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="km_i2lf";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="km_i2lf")
        {
            _modulekey_kx_map_lock.LockEnd();
            DEBUG_PRINT("[km_set_i2lf] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_km_intlf_map_lock);
    _km_intlf_map[module_key][field] = value;
    return true;
}
bool
infoHub_impl::km_del_i2lf(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intlf_map_lock);
    if(_km_intlf_map.find(module_key) == _km_intlf_map.end())
    {
        DEBUG_PRINT("[km_del_i2lf] wrong: km_del_i2lf:module_key not found!\n");
        return false;
    }
    if(_km_intlf_map[module_key].find(field) == _km_intlf_map[module_key].end())
    {
        DEBUG_PRINT("[km_del_i2lf] wrong: km_del_i2lf:field not found!\n");
        return false;
    }
    //删除键为module_key下的键为field的元素
    _km_intlf_map[module_key].erase(field);
    //删除整个module_key及所有子元素
    //_km_intint_map.erase(module_key);
    return true;
}
double
infoHub_impl::km_get_i2lf(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intlf_map_lock);
    if(_km_intlf_map.find(module_key) == _km_intlf_map.end())
    {
        DEBUG_PRINT("[km_get_i2lf] wrong: km_get_i2lf:module_key not found!\n");
        return -1.0;
    }
    if(_km_intlf_map[module_key].find(field) == _km_intlf_map[module_key].end())
    {
        DEBUG_PRINT("[km_get_i2lf] wrong: km_get_i2lf:field not found!\n");
        return -1.0;
    }
    return _km_intlf_map[module_key][field];
}
//获取key对应的hash有多少个键值对
int
infoHub_impl::km_len_i2lf(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intlf_map_lock);
    if(_km_intlf_map.find(module_key) == _km_intlf_map.end())
    {
        DEBUG_PRINT("[km_len_i2lf] wrong: km_len_i2lf:module_key not found!\n");
        return -1;
    }
    return _km_intlf_map[module_key].size();
}


//=================================================
// map: int:uint32_t
bool
infoHub_impl::km_set_i2u32(std::string module, std::string key, int field, uint32_t value)
{
    std::string module_key = module + key;
    _modulekey_kx_map_lock.LockBegin();
    if(_modulekey_kx_map.find(module_key)==_modulekey_kx_map.end())
    {
        _modulekey_kx_map[module_key]="km_i2u32";
    }
    else //如果已经存在module_key对应的map
    {
        //且已经存在的map不是kv类型
        if(_modulekey_kx_map[module_key]!="km_i2u32")
        {
            _modulekey_kx_map_lock.LockEnd();
            DEBUG_PRINT("[km_set_i2u32] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_km_intu32_map_lock);
    _km_intu32_map[module_key][field] = value;
    return true;
}
bool
infoHub_impl::km_del_i2u32(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intu32_map_lock);
    if(_km_intu32_map.find(module_key) == _km_intu32_map.end())
    {
        DEBUG_PRINT("[km_del_i2u32] wrong: km_del_i2u32:module_key not found!\n");
        return false;
    }
    if(_km_intu32_map[module_key].find(field) == _km_intu32_map[module_key].end())
    {
        DEBUG_PRINT("[km_del_i2u32] wrong: km_del_i2u32:field not found!\n");
        return false;
    }
    //删除键为module_key下的键为field的元素
    _km_intu32_map[module_key].erase(field);
    //删除整个module_key及所有子元素
    //_km_intint_map.erase(module_key);
    return true;
}
uint32_t
infoHub_impl::km_get_i2u32(std::string module, std::string key, int field)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intu32_map_lock);
    if(_km_intu32_map.find(module_key) == _km_intu32_map.end())
    {
        DEBUG_PRINT("[km_get_i2u32] wrong: km_get_i2u32:module_key not found!\n");
        return -1;
    }
    if(_km_intu32_map[module_key].find(field) == _km_intu32_map[module_key].end())
    {
        DEBUG_PRINT("[km_get_i2u32] wrong: km_get_i2u32:field not found!\n");
        return -1;
    }
    return _km_intu32_map[module_key][field];
}
//获取key对应的hash有多少个键值对
int
infoHub_impl::km_len_i2u32(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_km_intu32_map_lock);
    if(_km_intu32_map.find(module_key) == _km_intu32_map.end())
    {
        DEBUG_PRINT("[km_len_i2u32] wrong: km_len_i2u32:module_key not found!\n");
        return -1;
    }
    return _km_intu32_map[module_key].size();
}

/***** [module]:key set ****/
/*包含: 
    1. map<str, set<int>> ks_intset_map intset类型数据存在这里
    2. map<str, set<str>> ks_strset_map strset类型数据存在这里
**/
// set: int a, int b, int c ...
bool
infoHub_impl::ks_add_int(std::string module, std::string key, int member)
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
            DEBUG_PRINT("[ks_add_int] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_ks_intset_map_lock);
    _ks_intset_map[module_key].insert(member);
    return true;
}
bool
infoHub_impl::ks_del_int(std::string module, std::string key, int member)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_intset_map_lock);
    if(_ks_intset_map.find(module_key)==_ks_intset_map.end())
    {
        DEBUG_PRINT("[ks_del_int] wrong ks_del_int not find module_key\n");
        return false;
    }
    if(_ks_intset_map[module_key].find(member)==_ks_intset_map[module_key].end())
    {
        DEBUG_PRINT("[ks_del_int] wrong ks_del_int not find member\n");
        return false;
    }
    _ks_intset_map[module_key].erase(member);
    return true;
}
//判断mem是否是set的成员
bool
infoHub_impl::ks_ism_int(std::string module, std::string key, int member)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_intset_map_lock);
    if(_ks_intset_map.find(module_key)==_ks_intset_map.end())return false;
    if(_ks_intset_map[module_key].find(member)==_ks_intset_map[module_key].end())return false;
    else return true;
}
//计算集合元素的个数
int
infoHub_impl::ks_cnt_int(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_intset_map_lock);
    if(_ks_intset_map.find(module_key)==_ks_intset_map.end())
    {
        DEBUG_PRINT("ks_cnt_int not find module_key\n");
        return -1;
    }
    return _ks_intset_map[module_key].size();
}
//返回set的全部成员
std::set<int>
infoHub_impl::ks_get_int(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_intset_map_lock);
    if(_ks_intset_map.find(module_key)==_ks_intset_map.end())
    {
        DEBUG_PRINT("ks_cnt_int not find module_key\n");
        std::set<int> nullset;
        return nullset;
    }
    return _ks_intset_map[module_key];
}

// set: str a, str b, str c ...
bool
infoHub_impl::ks_add_str(std::string module, std::string key, std::string member)
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
            DEBUG_PRINT("[ks_add_str] wrong! exist other type table!\n");
            return false;
        }
    }
    _modulekey_kx_map_lock.LockEnd();
    dd::AutoLock autolock(_ks_strset_map_lock);
    _ks_strset_map[module_key].insert(member);
    return true;
}
bool
infoHub_impl::ks_del_str(std::string module, std::string key, std::string member)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_strset_map_lock);
    if(_ks_strset_map.find(module_key)==_ks_strset_map.end())
    {
        DEBUG_PRINT("[ks_del_str] wrong ks_del_int not find module_key\n");
        return false;
    }
    if(_ks_strset_map[module_key].find(member)==_ks_strset_map[module_key].end())
    {
        DEBUG_PRINT("[ks_del_str] wrong ks_del_int not find member\n");
        return false;
    }
    _ks_strset_map[module_key].erase(member);
    return true;
}
//判断mem是否是set的成员
bool
infoHub_impl::ks_ism_str(std::string module, std::string key, std::string member)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_strset_map_lock);
    if(_ks_strset_map.find(module_key)==_ks_strset_map.end())return false;
    if(_ks_strset_map[module_key].find(member)==_ks_strset_map[module_key].end())return false;
    else return true;
}
//计算集合元素的个数
int
infoHub_impl::ks_cnt_str(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_strset_map_lock);
    if(_ks_strset_map.find(module_key)==_ks_strset_map.end())
    {
        DEBUG_PRINT("ks_cnt_int not find module_key\n");
        return -1;
    }
    return _ks_strset_map[module_key].size();
}
//返回set的全部成员
std::set<std::string>
infoHub_impl::ks_get_str(std::string module, std::string key)
{
    std::string module_key = module + key;
    dd::AutoLock autolock(_ks_strset_map_lock);
    if(_ks_strset_map.find(module_key)==_ks_strset_map.end())
    {
        DEBUG_PRINT("ks_cnt_int not find module_key\n");
        std::set<std::string> nullset;
        return nullset;
    }
    return _ks_strset_map[module_key];
}

