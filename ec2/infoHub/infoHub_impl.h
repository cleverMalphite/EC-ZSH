#ifndef __INFO_HUB_IMPL__
#define __INFO_HUB_IMPL__

#include <map>
#include <set>
#include <vector>
#include <cstdio>

#include "../../ec2/public/utility.h"


class infoHub_impl
{
public:
    infoHub_impl();
    ~infoHub_impl();

    /***** [module]:key value ****/
    /*包含: 
        1. map<str, int> kv_int_map int类型数据存在这里
        2. map<str, str> kv_str_map str类型数据存在这里
    **/
    // string:int   ==> map<str, int>
    bool    kv_set_int(std::string module, std::string key, int value);
    bool    kv_del_int(std::string module, std::string key);
    int     kv_get_int(std::string module, std::string key);
    // string:string   ==> map<str, str>
    bool    kv_set_str(std::string module, std::string key, std::string value);
    bool    kv_del_str(std::string module, std::string key);
    std::string kv_get_str(std::string module, std::string key);
    // string:double   ==> map<str, double>
    bool    kv_set_lf(std::string module, std::string key, double value);
    bool    kv_del_lf(std::string module, std::string key);
    double  kv_get_lf(std::string module, std::string key);
    // string:uint32_t   ==> map<str, uint32_t>
    bool    kv_set_u32(std::string module, std::string key, uint32_t value);
    bool    kv_del_u32(std::string module, std::string key);
    uint32_t kv_get_u32(std::string module, std::string key);

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
    // map: int:double
    bool    km_set_i2lf(std::string module, std::string key, int field, double value);
    bool    km_del_i2lf(std::string module, std::string key, int field);
    double  km_get_i2lf(std::string module, std::string key, int field);
    int     km_len_i2lf(std::string module, std::string key);//获取key对应的hash有多少个键值对
    // map: int:uint32_t
    bool    km_set_i2u32(std::string module, std::string key, int field, uint32_t value);
    bool    km_del_i2u32(std::string module, std::string key, int field);
    uint32_t km_get_i2u32(std::string module, std::string key, int field);
    int     km_len_i2u32(std::string module, std::string key);//获取key对应的hash有多少个键值对

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
    std::set<int>   ks_get_int(std::string module, std::string key);//返回set的全部成员
    // set: str a, str b, str c ...
    bool    ks_add_str(std::string module, std::string key, std::string member);
    bool    ks_del_str(std::string module, std::string key, std::string member);
    bool    ks_ism_str(std::string module, std::string key, std::string member);//判断mem是否是set的成员
    int     ks_cnt_str(std::string module, std::string key);//计算集合元素的个数
    std::set<std::string> ks_get_str(std::string module, std::string key);//返回set的全部成员

private:
    //确保全局的module-key是唯一的,不会在kv中也有而在km、ks中也有
    std::map<std::string, std::string>                  _modulekey_kx_map;//储存module-key
    dd::Lock _modulekey_kx_map_lock;

    /***** [module]:key value ****/
    dd::Lock _kv_int_map_lock;
    std::map<std::string, int>                          _kv_int_map; //int类型数据存在这里
                                                                     
    dd::Lock _kv_str_map_lock;
    std::map<std::string, std::string>                  _kv_str_map; //str类型数据存在这里
                                                                     
    dd::Lock _kv_lf_map_lock;
    std::map<std::string, double>                  _kv_lf_map; //double类型数据存在这里
                                                              
    dd::Lock _kv_u32_map_lock;
    std::map<std::string, uint32_t>                  _kv_u32_map; //double类型数据存在这里

    /***** [module]:key map ****/
    dd::Lock _km_intint_map_lock;
    std::map<std::string, std::map<int, int>>           _km_intint_map; //int:int类型数据存在这里

    dd::Lock _km_intstr_map_lock;
    std::map<std::string, std::map<int, std::string>>   _km_intstr_map; //int:str类型数据存在这里

    dd::Lock _km_intlf_map_lock;
    std::map<std::string, std::map<int, double>>   _km_intlf_map; //int:double类型数据存在这里
                                                                  //
    dd::Lock _km_intu32_map_lock;
    std::map<std::string, std::map<int, uint32_t>>   _km_intu32_map; //int:uint32_t类型数据存在这里

    /***** [module]:key set ****/
    dd::Lock _ks_intset_map_lock;
    std::map<std::string, std::set<int>>                _ks_intset_map; //intset类型数据存在这里
    dd::Lock _ks_strset_map_lock;
    std::map<std::string, std::set<std::string>>        _ks_strset_map; //strset类型数据存在这里

};

#endif// __INFO_HUB_IMPL__
