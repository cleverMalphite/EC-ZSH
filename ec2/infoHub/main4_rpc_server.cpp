#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
#include "unistd.h"
//get time string
#include "../../ec2/public/_public.h"
#include "./infoHub.h"
#include "./rateStatistic.h"
#include "./rateStatisticTable.h"

#include "./infoHubRpcServer.h"

using namespace std;

std::shared_ptr<ec2::infoHub> infohub_instance =\
                        ec2::infoHub::getInstance();

ec2::rateStatistic tx_rate2_statistic;
ec2::rateStatisticTable tx_rate2_statistic_table;

int main(void)
{

    //-------infohub rpc server----------
    //infoHubRpcServer infohub_rpcserver(infohub_instance);
    infoHubRpcServer infohub_rpcserver;
    infohub_rpcserver.init(infohub_instance, 8000);
    infohub_rpcserver.async_run(1);



    //-------infohub rate statisitc----------
    tx_rate2_statistic.init("epollcomm", "tx_rate_value");
    tx_rate2_statistic_table.init("epollcomm", "tx_cid_rate_table");


    tx_rate2_statistic.begin();

    tx_rate2_statistic_table.begin(100);
    tx_rate2_statistic_table.begin(101);
    tx_rate2_statistic_table.begin(102);

    std::string strbuffer = "1234567890";
    size_t sizebuffer = strbuffer.capacity(); //bytes

    int value_cnt=0;
    int table_cnt_100=0;
    int table_cnt_101=0;
    int table_cnt_102=0;

    while(true){
        sleep(1);
        infohub_instance->value_set("epollcomm", "value_cnt", ++value_cnt);
        infohub_instance->table_set("epollcomm", "table_cnt", 100, ++table_cnt_100);
        infohub_instance->table_set("epollcomm", "table_cnt", 101, ++table_cnt_101);
        infohub_instance->table_set("epollcomm", "table_cnt", 102, ++table_cnt_102);


        //pass rate to infohub
        double ret = tx_rate2_statistic.pass(sizebuffer);
        printf("<cnt:%d>", value_cnt); printf("ret    : epollcomm tx_rate=%lf\n", ret);
        printf("-----------------------------\n");

        //get rate from infohub
        double epoll_rate=infohub_instance->value_get<double>("epollcomm", "tx_rate_value");
        printf("<cnt:%d>", value_cnt); printf("infohub: epollcomm tx_rate=%lf\n", epoll_rate);
        printf("+++++++++++++++++++++++++++++++\n");

        //pass rate to infohub
        double ret0 = tx_rate2_statistic_table.pass(100, sizebuffer);
        double ret1 = tx_rate2_statistic_table.pass(101, sizebuffer);
        double ret2 = tx_rate2_statistic_table.pass(102, sizebuffer);
        printf("<cnt:%d>", table_cnt_100); printf("ret0   : epollcomm tx_cid_rate_table cid:100=%lf\n", ret0);
        printf("<cnt:%d>", table_cnt_101); printf("ret1   : epollcomm tx_cid_rate_table cid:101=%lf\n", ret1);
        printf("<cnt:%d>", table_cnt_102); printf("ret2   : epollcomm tx_cid_rate_table cid:102=%lf\n", ret2);
        printf("-----------------------------\n");
        //get rate from infohub
        double epoll_rate0=infohub_instance->table_get<double>("epollcomm", "tx_cid_rate_table", 100);
        double epoll_rate1=infohub_instance->table_get<double>("epollcomm", "tx_cid_rate_table", 101);
        double epoll_rate2=infohub_instance->table_get<double>("epollcomm", "tx_cid_rate_table", 102);
        printf("<cnt:%d>", table_cnt_100); printf("infohub: epollcomm tx_cid_rate_table cid:100=%lf\n", epoll_rate0);
        printf("<cnt:%d>", table_cnt_101); printf("infohub: epollcomm tx_cid_rate_table cid:101=%lf\n", epoll_rate1);
        printf("<cnt:%d>", table_cnt_102); printf("infohub: epollcomm tx_cid_rate_table cid:102=%lf\n", epoll_rate2);
        printf("=============================\n");
        printf("\n");



    }
    printf("\n");

    infohub_instance->value_del("epollcomm", "tx_rate_value");

    infohub_instance->table_del("epollcomm", "tx_cid_rate_table", 100);
    infohub_instance->table_del("epollcomm", "tx_cid_rate_table", 101);
    infohub_instance->table_del("epollcomm", "tx_cid_rate_table", 102);

    return 0;
}

