#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
//get time string
#include "../../ec2/public/_public.h"
#include "./infoHub.h"

#include "./progressStatistic.h"
#include "./progressStatisticTable.h"

using namespace std;

std::shared_ptr<ec2::infoHub> infohub_instance =\
                        ec2::infoHub::getInstance();

ec2::progressStatistic recv_progress;
ec2::progressStatisticTable recv_progress_table;

int main(void)
{
    recv_progress.init("epollcomm", "tx_rate_value");
    recv_progress_table.init("epollcomm", "tx_cid_rate_table");
    //tx_rate2_statistic_table.init("epollcomm", "tx_cid_rate_table");


    recv_progress.begin(1000);
    recv_progress_table.begin(1, 1000);

    //tx_rate2_statistic_table.begin(100);
    //tx_rate2_statistic_table.begin(101);
    //tx_rate2_statistic_table.begin(102);

    //std::string strbuffer = "1234567890";
    //size_t sizebuffer = strbuffer.capacity(); //bytes

    for(int i=0; i < 100; i++){
        //pass rate to infohub
        double ret = recv_progress.pass(10);
        double ret1 = recv_progress_table.pass(1, 10);
        printf("ret    : epollcomm tx_rate=%lf\n", ret);
        printf("ret    : epollcomm tx_rate_table=%lf\n", ret1);
        printf("-----------------------------\n");
        //get rate from infohub
        double epoll_rate=infohub_instance->value_get<double>("epollcomm", "tx_rate_value");
        double epoll_rate1=infohub_instance->table_get<double>("epollcomm", "tx_cid_rate_table", 1);
        printf("infohub: epollcomm progess=%lf\n", epoll_rate);
        printf("infohub: epollcomm progess table =%lf\n", epoll_rate1);
        printf("\n");

        sleep(0.5);
    }
    printf("\n");

    return 0;
}

