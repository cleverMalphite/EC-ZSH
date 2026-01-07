//
// Created by 王炳棋 on 2022/12/31.
//
#include "MRUDPSettingUtil.h"

namespace MRUDP {
    //删除字符串中空格，制表符tab等无效字符
    std::string Trim(std::string &str) {
        //str.find_first_not_of(" \t\r\n"),在字符串str中从索引0开始，返回首次不匹配"\t\r\n"的位置
        str.erase(0, str.find_first_not_of(" \t\r\n"));
        str.erase(str.find_last_not_of(" \t\r\n") + 1);
        return str;
    }


};