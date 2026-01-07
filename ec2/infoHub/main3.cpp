#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
//get time string
#include "../../ec2/public/_public.h"
#include "./infoHub.h"
#include "./rateStatistic.h"
#include "./rateStatisticTable.h"

using namespace std;

std::shared_ptr<ec2::infoHub> infohub_instance =\
                        ec2::infoHub::getInstance();

ec2::rateStatistic tx_rate2_statistic;
ec2::rateStatisticTable tx_rate2_statistic_table;

int main(void)
{
    tx_rate2_statistic.init("epollcomm", "tx_rate_value");
    tx_rate2_statistic_table.init("epollcomm", "tx_cid_rate_table");


    tx_rate2_statistic.begin();

    tx_rate2_statistic_table.begin(100);
    tx_rate2_statistic_table.begin(101);
    tx_rate2_statistic_table.begin(102);

    std::string strbuffer = "1234567890";
    size_t sizebuffer = strbuffer.capacity(); //bytes

    while(true){
        //pass rate to infohub
        double ret = tx_rate2_statistic.pass(sizebuffer);
        printf("ret    : epollcomm tx_rate=%lf\n", ret);
        printf("-----------------------------\n");
        //get rate from infohub
        double epoll_rate=infohub_instance->value_get<double>("epollcomm", "tx_rate_value");
        printf("infohub: epollcomm tx_rate=%lf\n", epoll_rate);
        printf("\n");

        //pass rate to infohub
        double ret0 = tx_rate2_statistic_table.pass(100, sizebuffer);
        double ret1 = tx_rate2_statistic_table.pass(101, sizebuffer);
        double ret2 = tx_rate2_statistic_table.pass(102, sizebuffer);
        printf("ret0   : epollcomm tx_cid_rate_table cid:100=%lf\n", ret0);
        printf("ret1   : epollcomm tx_cid_rate_table cid:101=%lf\n", ret1);
        printf("ret2   : epollcomm tx_cid_rate_table cid:102=%lf\n", ret2);
        printf("-----------------------------\n");
        //get rate from infohub
        double epoll_rate0=infohub_instance->table_get<double>("epollcomm", "tx_cid_rate_table", 100);
        double epoll_rate1=infohub_instance->table_get<double>("epollcomm", "tx_cid_rate_table", 101);
        double epoll_rate2=infohub_instance->table_get<double>("epollcomm", "tx_cid_rate_table", 102);

        int len=infohub_instance->table_len("epollcomm", "tx_cid_rate_table");
        printf("infohub: epollcomm tx_cid_rate_table cid:100=%lf\n", epoll_rate0);
        printf("infohub: epollcomm tx_cid_rate_table cid:101=%lf\n", epoll_rate1);
        printf("infohub: epollcomm tx_cid_rate_table cid:102=%lf\n", epoll_rate2);
        printf("infohub: epollcomm tx_cid_rate_table len=%d\n", len);
        printf("\n");

        sleep(0.5);
    }
    printf("\n");

    infohub_instance->value_del("epollcomm", "tx_rate_value");

    infohub_instance->table_del("epollcomm", "tx_cid_rate_table", 100);
    infohub_instance->table_del("epollcomm", "tx_cid_rate_table", 101);
    infohub_instance->table_del("epollcomm", "tx_cid_rate_table", 102);

    return 0;
}

