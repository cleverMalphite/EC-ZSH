#ifndef __INFO_HUB_H__
#define __INFO_HUB_H__

#include <map>
#include <set>
#include <vector>
#include <cstdio>
#include <memory>
#include "./infoHub_impl.h"

namespace ec2{

class infoHub;

//1. 可以延迟初始化,
//   实例在第一次被使用时才被创建
//2. 也可以手动初始化，
//   以便统一在程序开始初始化，
//   防止运行时的初始化时延
//   影响其他程序运行
//2. 同时,提供destroyInstance方法,
//   以便程序退出前手动销毁单例实例,
//   防止内存泄漏
class infoHub
{
protected:
    //禁止外部构造
    infoHub() = default;

public:
    // 防止复制和赋值
    infoHub(const infoHub&) = delete;
    infoHub& operator=(const infoHub&) = delete;

    //获取实例的静态方法
    static std::shared_ptr<infoHub> getInstance();

    //--------------- kv -------------------
    bool    value_set(std::string module, std::string key, int value);
    bool    value_set(std::string module, std::string key, std::string value);
    bool    value_set(std::string module, std::string key, double value);
    bool    value_set(std::string module, std::string key, uint32_t value);

    bool    value_del(std::string module, std::string key);

    template<typename T>
    T value_get(std::string module, std::string key) {
    	T value;
        bool ret = value_get(module, key, value);
        return value;
    }
    bool    value_get(std::string module, std::string key, int &value);
    bool    value_get(std::string module, std::string key, std::string &value);
    bool    value_get(std::string module, std::string key, double &value);
    bool    value_get(std::string module, std::string key, uint32_t &value);

    //--------------- km -------------------
    bool    table_set(std::string module, std::string key, int field, int value);
    bool    table_set(std::string module, std::string key, int field, std::string value);
    bool    table_set(std::string module, std::string key, int field, double value);
    bool    table_set(std::string module, std::string key, int field, uint32_t value);

    bool    table_del(std::string module, std::string key, int field);

    template<typename T>
    T table_get(std::string module, std::string key, int field) {
        T value;
        bool ret = table_get(module, key, field, value);
        return value;
    }
    bool    table_get(std::string module, std::string key, int field, int &value);
    bool    table_get(std::string module, std::string key, int field, std::string &value);
    bool    table_get(std::string module, std::string key, int field, double &value);
    bool    table_get(std::string module, std::string key, int field, uint32_t &value);

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
    T set_get(std::string module, std::string key)
    {
        T set_t;
        bool ret = set_get(module, key, set_t);
        return set_t;
    }
    bool    set_get(std::string module, std::string key, std::set<int> &set_int);//返回set的全部成员
    bool    set_get(std::string module, std::string key, std::set<std::string> &set_str);//返回set的全部成员

private:

    infoHub_impl _infohub;
};


}//namespace ec2


#endif// __INFO_HUB__

