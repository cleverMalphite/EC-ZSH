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

#include "./infoHubRpcClient.h"

using namespace std;

int main(void)
{
    infoHubRpcClient infohub_rpcclient;
    infohub_rpcclient.init(8000);

    printf("__> connetion state: %d\n", infohub_rpcclient.get_connection_state());

    while(1){

        sleep(1);

        printf("--> connetion state: %d\n", infohub_rpcclient.get_connection_state());

        double tx_rate_value = infohub_rpcclient.value_get<double>("epollcomm", "tx_rate_value");
        printf("<cnt:%d>", infohub_rpcclient.value_get<int>("epollcomm", "value_cnt"));
        printf("<infohub_rpcclient> tx_rate_value = %lf\n", tx_rate_value);

        double tx_cid_rate_table_100 = infohub_rpcclient.table_get<double>("epollcomm", "tx_cid_rate_table", 100);
        double tx_cid_rate_table_101 = infohub_rpcclient.table_get<double>("epollcomm", "tx_cid_rate_table", 101);
        double tx_cid_rate_table_102 = infohub_rpcclient.table_get<double>("epollcomm", "tx_cid_rate_table", 102);

        printf("<cnt:%d>", infohub_rpcclient.table_get<int>("epollcomm", "table_cnt", 100));
        printf("<infohub_rpcclient> tx_cid_rate_table 100: %lf\n", tx_cid_rate_table_100 );

        printf("<cnt:%d>", infohub_rpcclient.table_get<int>("epollcomm", "table_cnt", 101));
        printf("<infohub_rpcclient> tx_cid_rate_table 101: %lf\n", tx_cid_rate_table_101 );

        printf("<cnt:%d>", infohub_rpcclient.table_get<int>("epollcomm", "table_cnt", 102));
        printf("<infohub_rpcclient> tx_cid_rate_table 102: %lf\n", tx_cid_rate_table_102 );

        printf("-----------------------------\n");

    }


    return 0;
}

