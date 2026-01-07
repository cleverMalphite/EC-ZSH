#ifndef INFO_HUB_API
#define INFO_HUB_API

#include <map>
#include <set>
#include <vector>
#include <cstdio>

#include "../../ec2/public/utility.h"

class infoHub
{
public:
    infoHub();
    ~infoHub();

    /***** [module]:key value ****/
    /*包含: 
        1. map<str, int> kv_int_map int类型数据存在这里
        2. map<str, str> kv_str_map str类型数据存在这里
    **/
    // string:int   ==> map<str, int>
    //
    bool    kv_set_int(std::string module, std::string key, int value);
    bool    kv_del_int(std::string module, std::string key);
    int     kv_get_int(std::string module, std::string key);
    // string:string   ==> map<str, str>
    bool    kv_set_str(std::string module, std::string key, std::string value);
    bool    kv_del_str(std::string module, std::string key);
    std::string kv_get_str(std::string module, std::string key);

    /***** [module]:key map ****/
    /*包含: 
        1. map<str, map<int, int>> km_intint_map int:int类型数据存在这里
        2. map<str, map<int, str>> km_intstr_map str:str类型数据存在这里
    **/
    // map: int:int
    bool    km_set_i2i(std::string module, std::string key, int field, int value);
    bool    km_del_i2i(std::string module, std::string key, int field);
    int     km_get_i2i(std::string module, std::string key, int field);
    int     km_len_i2i(std::string module, std::string key);//获取key对应的hash有多少个键值对
    // map: int:str
    bool    km_set_i2s(std::string module, std::string key, int field, std::string value);
    bool    km_del_i2s(std::string module, std::string key, int field);
    std::string km_get_i2s(std::string module, std::string key, int field);
    int     km_len_i2s(std::string module, std::string key);//获取key对应的hash有多少个键值对

    /***** [module]:key set ****/
    /*包含: 
        1. map<str, set<int>> ks_intset_map intset类型数据存在这里
        2. map<str, set<str>> ks_strset_map strset类型数据存在这里
    **/
    // set: int a, int b, int c ...
    bool    ks_add_int(std::string module, std::string key, int member);
    bool    ks_del_int(std::string module, std::string key, int member);
    bool    ks_ism_int(std::string module, std::string key, int member);//判断mem是否是set的成员
    int     ks_cnt_int(std::string module, std::string key);//计算集合元素的个数
    std::set<int>   ks_shw_int(std::string module, std::string key);//返回set的全部成员
    // set: str a, str b, str c ...
    bool    ks_add_str(std::string module, std::string key, std::string member);
    bool    ks_del_str(std::string module, std::string key, std::string member);
    bool    ks_ism_str(std::string module, std::string key, std::string member);//判断mem是否是set的成员
    int     ks_cnt_str(std::string module, std::string key);//计算集合元素的个数
    std::set<std::string> ks_shw_str(std::string module, std::string key);//返回set的全部成员

private:
    //确保全局的module-key是唯一的,不会在kv中也有而在km、ks中也有
    std::map<std::string, std::string>                  _modulekey_kx_map;//储存module-key
    dd::Lock _modulekey_kx_map_lock;

    /***** [module]:key value ****/
    std::map<std::string, int>                          _kv_int_map; //int类型数据存在这里
    std::map<std::string, std::string>                  _kv_str_map; //str类型数据存在这里
    dd::Lock _kv_int_map_lock;
    dd::Lock _kv_str_map_lock;

    /***** [module]:key map ****/
    std::map<std::string, std::map<int, int>>           _km_intint_map; //int:int类型数据存在这里
    std::map<std::string, std::map<int, std::string>>   _km_intstr_map; //int:str类型数据存在这里
    dd::Lock _km_intint_map_lock;
    dd::Lock _km_intstr_map_lock;

    /***** [module]:key set ****/
    std::map<std::string, std::set<int>>                _ks_intset_map; //intset类型数据存在这里
    std::map<std::string, std::set<std::string>>        _ks_strset_map; //strset类型数据存在这里
    dd::Lock _ks_intset_map_lock;
    dd::Lock _ks_strset_map_lock;

};



#endif// INFO_HUB_API
