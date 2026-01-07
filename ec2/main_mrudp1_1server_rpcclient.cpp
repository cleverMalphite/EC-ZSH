#include <iostream>
#include <unistd.h>
#include "public/_public.h"
#include "infoHub/infoHub.h"
#include "infoHub/infoHubRpcClient.h"

struct st_arg{
	int role;
  int runtime;
} starg;

CLogFile logfile;

infoHubRpcClient sender_infohub_rpcclient; 
infoHubRpcClient receiver_infohub_rpcclient; 

//退出函数
void EXIT(int sig);

//显示程序帮助
void _help(char *argv[]);

//把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer);

int main(int argc, char *argv[]) {
		
	if(argc!=3) { _help(argv); return -1; }

    //关闭全部的信号和输入输出
    //CloseIOAndSignal();

    //处理程序退出的信号
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

	if(logfile.Open(argv[1], "a+")==false)
	{ 
	    printf("打开日志文件失败(%s). \n", argv[1]); return -1;
	}

    if(_xmltoarg(argv[2])==false) return -1;

    //logfile.Write("System_start 运行成功!\n");

      sleep(5);//等待接收方启动后，再创建发送端

    //发送方
    if(starg.role==1){

        sender_infohub_rpcclient.init("127.0.0.1", 8000); 

        printf("__>> connetion state: %d\n", sender_infohub_rpcclient.get_connection_state());

        while(1){
            sleep(1);
            printf("___________________________________________\n");
            printf("__>> connetion state: %d\n", sender_infohub_rpcclient.get_connection_state());

            printf("+++++++++++++++++++++++++++++++++++++++++++\n");
            std::string program_name = sender_infohub_rpcclient.value_get<std::string>("EC", "main_mrudp1_1server");
            printf("[infohub_rpcclient] rpcserver program name = %s\n", program_name.c_str());

            printf("--------------- epollcomm infohub --------------\n");
            double epollcomm_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("epollcomm", "tx_rate_stat");
            double epollcomm_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("epollcomm", "rx_rate_stat");
            printf("<infohub_rpcclient> epollcomm tx_rate_value = %lf\n", epollcomm_tx_rate_stat);
            printf("<infohub_rpcclient> epollcomm rx_rate_value = %lf\n", epollcomm_rx_rate_stat);

            uint32_t epollcomm_qos_tx_rx_rtt_stat = sender_infohub_rpcclient.value_get<uint32_t>("epollcomm", "qos_tx_rx_rtt_stat");
            uint32_t epollcomm_qos_tx_rx_loss_stat = sender_infohub_rpcclient.value_get<uint32_t>("epollcomm", "qos_tx_rx_loss_stat");
            printf("<infohub_rpcclient> epollcomm qos_tx_rx_rtt_stat  = %u\n", epollcomm_qos_tx_rx_rtt_stat);
            printf("<infohub_rpcclient> epollcomm qos_tx_rx_loss_stat = %u\n", epollcomm_qos_tx_rx_loss_stat);

            printf("--------------- netcombtransfer infohub --------------\n");
            double nct_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("netcombtransfer", "rx_rate_stat");
            double nct_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("netcombtransfer", "tx_rate_stat");
            double nct_msg_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("netcombtransfer", "msg_rx_rate_stat");
            double nct_msg_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("netcombtransfer", "msg_tx_rate_stat");
            double nct_data_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("netcombtransfer", "data_rx_rate_stat");
            double nct_data_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("netcombtransfer", "data_tx_rate_stat");
            printf("<infohub_rpcclient> netcombtransfer tx_rate_value = %lf\n", nct_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer rx_rate_value = %lf\n", nct_rx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer msg_tx_rate_value = %lf\n", nct_msg_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer msg_rx_rate_value = %lf\n", nct_msg_rx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer data_tx_rate_value = %lf\n", nct_data_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer data_rx_rate_value = %lf\n", nct_data_rx_rate_stat);

            printf("--------------- speedcontrol infohub --------------\n");
            double spc_expect_speed = sender_infohub_rpcclient.value_get<double>("speedcontrol", "expect_speed_stat");
            double spc_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("speedcontrol", "rx_rate_stat");
            double spc_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("speedcontrol", "tx_rate_stat");
            double spc_mrudp_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("speedcontrol", "mrudp_rx_rate_stat");
            double spc_mrudp_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("speedcontrol", "mrudp_tx_rate_stat");
            double spc_rbudp_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("speedcontrol", "rbudp_rx_rate_stat");
            double spc_rbudp_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("speedcontrol", "rbudp_tx_rate_stat");
            printf("<infohub_rpcclient> speedcontrol spc_expect_speed = %lf\n", spc_expect_speed);
            printf("<infohub_rpcclient> speedcontrol tx_rate_value = %lf\n", spc_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rx_rate_value = %lf\n", spc_rx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol mrudp_tx_rate_value = %lf\n", spc_mrudp_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol mrudp_rx_rate_value = %lf\n", spc_mrudp_rx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rbudp_tx_rate_value = %lf\n", spc_rbudp_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rbudp_rx_rate_value = %lf\n", spc_rbudp_rx_rate_stat);

            // ------------- bigdatatransfer infohub -----------------
            printf("--------------- bigdatatransfer infohub --------------\n");
            double bdt_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("bigdatatransfer", "rx_rate_stat");
            double bdt_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("bigdatatransfer", "tx_rate_stat");
            printf("<infohub_rpcclient> bigdatatransfer tx_rate_value = %lf\n", bdt_tx_rate_stat);
            printf("<infohub_rpcclient> bigdatatransfer rx_rate_value = %lf\n", bdt_rx_rate_stat);
            printf("<infohub_rpcclient> byte/ms\n");

            int snd_file_len = sender_infohub_rpcclient.table_len("bigdatatransfer", "snd_fid_progress_sttable");
            printf("<infohub_rpcclient> snd_file_len: %d\n", snd_file_len);
            for(int i = 0; i < snd_file_len; i++){
                double bdt_snd_fid_progress = sender_infohub_rpcclient.table_get<double>("bigdatatransfer", "snd_fid_progress_sttable", i);
                printf("<infohub_rpcclient> bigdatatransfer snd_progress[%d] = %lf\n", i, bdt_snd_fid_progress);
            }
            int rcv_file_cnt = sender_infohub_rpcclient.table_len("bigdatatransfer", "rcv_fid_progress_sttable"); 
            printf("<infohub_rpcclient> rcv_file_cnt: %d\n", rcv_file_cnt);
            for(int i = 0; i < rcv_file_cnt; i++){
                double bdt_rcv_fid_progress = sender_infohub_rpcclient.table_get<double>("bigdatatransfer", "rcv_fid_progress_sttable", i);
                printf("<infohub_rpcclient> bigdatatransfer rcv_progress[%d] = %lf\n", i, bdt_rcv_fid_progress);
            }


            printf("=======================================\n");
        }


    }
    //接收方
    else if(starg.role==2){
        receiver_infohub_rpcclient.init("127.0.0.1", 8001); 

        printf("__>> connetion state: %d\n", receiver_infohub_rpcclient.get_connection_state());

        while(1){
            sleep(1);

            printf("___________________________________________\n");
            printf("-->> connetion state: %d\n", receiver_infohub_rpcclient.get_connection_state());

            printf("+++++++++++++++++++++++++++++++++++++++++++\n");
            std::string program_name = receiver_infohub_rpcclient.value_get<std::string>("EC", "main_mrudp1_1server");
            printf("[infohub_rpcclient] rpcserver program name = %s\n", program_name.c_str());


            printf("--------------- epollcomm infohub --------------\n");
            double epollcomm_tx_rate_stat = receiver_infohub_rpcclient.value_get<double>("epollcomm", "tx_rate_stat");
            double epollcomm_rx_rate_stat = receiver_infohub_rpcclient.value_get<double>("epollcomm", "rx_rate_stat");
            printf("<infohub_rpcclient> epollcomm tx_rate_value = %lf\n", epollcomm_tx_rate_stat);
            printf("<infohub_rpcclient> epollcomm rx_rate_value = %lf\n", epollcomm_rx_rate_stat);

            uint32_t epollcomm_qos_tx_rx_rtt_stat = receiver_infohub_rpcclient.value_get<uint32_t>("epollcomm", "qos_tx_rx_rtt_stat");
            uint32_t epollcomm_qos_tx_rx_loss_stat = receiver_infohub_rpcclient.value_get<uint32_t>("epollcomm", "qos_tx_rx_loss_stat");
            printf("<infohub_rpcclient> epollcomm qos_tx_rx_rtt_stat  = %u\n", epollcomm_qos_tx_rx_rtt_stat);
            printf("<infohub_rpcclient> epollcomm qos_tx_rx_loss_stat = %u\n", epollcomm_qos_tx_rx_loss_stat);


            printf("--------------- netcombtransfer infohub --------------\n");
            double nct_rx_rate_stat = receiver_infohub_rpcclient.value_get<double>("netcombtransfer", "rx_rate_stat");
            double nct_tx_rate_stat = receiver_infohub_rpcclient.value_get<double>("netcombtransfer", "tx_rate_stat");
            printf("<infohub_rpcclient> netcombtransfer tx_rate_value = %lf\n", nct_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer rx_rate_value = %lf\n", nct_rx_rate_stat);
            double nct_msg_rx_rate_stat = receiver_infohub_rpcclient.value_get<double>("netcombtransfer", "msg_rx_rate_stat");
            double nct_msg_tx_rate_stat = receiver_infohub_rpcclient.value_get<double>("netcombtransfer", "msg_tx_rate_stat");
            printf("<infohub_rpcclient> netcombtransfer msg_tx_rate_value = %lf\n", nct_msg_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer msg_rx_rate_value = %lf\n", nct_msg_rx_rate_stat);
            double nct_data_rx_rate_stat = receiver_infohub_rpcclient.value_get<double>("netcombtransfer", "data_rx_rate_stat");
            double nct_data_tx_rate_stat = receiver_infohub_rpcclient.value_get<double>("netcombtransfer", "data_tx_rate_stat");
            printf("<infohub_rpcclient> netcombtransfer data_tx_rate_value = %lf\n", nct_data_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer data_rx_rate_value = %lf\n", nct_data_rx_rate_stat);

            printf("--------------- speedcontrol infohub --------------\n");
            double spc_expect_speed = receiver_infohub_rpcclient.value_get<double>("speedcontrol", "expect_speed_stat");
            double spc_rx_rate_stat = receiver_infohub_rpcclient.value_get<double>("speedcontrol", "rx_rate_stat");
            double spc_tx_rate_stat = receiver_infohub_rpcclient.value_get<double>("speedcontrol", "tx_rate_stat");
            double spc_mrudp_rx_rate_stat = receiver_infohub_rpcclient.value_get<double>("speedcontrol", "mrudp_rx_rate_stat");
            double spc_mrudp_tx_rate_stat = receiver_infohub_rpcclient.value_get<double>("speedcontrol", "mrudp_tx_rate_stat");
            double spc_rbudp_rx_rate_stat = receiver_infohub_rpcclient.value_get<double>("speedcontrol", "rbudp_rx_rate_stat");
            double spc_rbudp_tx_rate_stat = receiver_infohub_rpcclient.value_get<double>("speedcontrol", "rbudp_tx_rate_stat");
            printf("<infohub_rpcclient> speedcontrol spc_expect_speed = %lf\n", spc_expect_speed);
            printf("<infohub_rpcclient> speedcontrol tx_rate_value = %lf\n", spc_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rx_rate_value = %lf\n", spc_rx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol mrudp_tx_rate_value = %lf\n", spc_mrudp_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol mrudp_rx_rate_value = %lf\n", spc_mrudp_rx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rbudp_tx_rate_value = %lf\n", spc_rbudp_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rbudp_rx_rate_value = %lf\n", spc_rbudp_rx_rate_stat);

                        // ------------- bigdatatransfer infohub -----------------
            printf("--------------- bigdatatransfer infohub --------------\n");
            double bdt_rx_rate_stat = receiver_infohub_rpcclient.value_get<double>("bigdatatransfer", "rx_rate_stat");
            double bdt_tx_rate_stat = receiver_infohub_rpcclient.value_get<double>("bigdatatransfer", "tx_rate_stat");
            double bdt_cur_spd_stat=  receiver_infohub_rpcclient.value_get<double>("bigdatatransfer", "rx_current_speedkBps_stat");
            printf("<infohub_rpcclient> bigdatatransfer tx_rate_value = %lf\n", bdt_tx_rate_stat);
            printf("<infohub_rpcclient> bigdatatransfer rx_rate_value = %lf\n", bdt_rx_rate_stat);
            printf("<infohub_rpcclient> bigdatatransfer rx_current_speedkBps_stat = %lf\n", bdt_cur_spd_stat);

            printf("<infohub_rpcclient> byte/ms\n");

            int snd_file_len = receiver_infohub_rpcclient.table_len("bigdatatransfer", "snd_fid_progress_sttable");
            printf("<infohub_rpcclient> snd_file_len: %d\n", snd_file_len);
            for(int i = 0; i < snd_file_len; i++){
                double bdt_snd_fid_progress = receiver_infohub_rpcclient.table_get<double>("bigdatatransfer", "snd_fid_progress_sttable", i);
                printf("<infohub_rpcclient> bigdatatransfer snd_progress[%d] = %lf\n", i, bdt_snd_fid_progress);
            }
            int rcv_file_cnt = receiver_infohub_rpcclient.table_len("bigdatatransfer", "rcv_fid_progress_sttable"); 
            printf("<infohub_rpcclient> rcv_file_cnt: %d\n", rcv_file_cnt);
            for(int i = 0; i < rcv_file_cnt; i++){
                double bdt_rcv_fid_progress = receiver_infohub_rpcclient.table_get<double>("bigdatatransfer", "rcv_fid_progress_sttable", i);
                printf("<infohub_rpcclient> bigdatatransfer rcv_progress[%d] = %lf\n", i, bdt_rcv_fid_progress);
            }


            printf("==============================================\n");

        }

    }
    //待机进入后运行
    sleep(starg.runtime);

    return 0;
}

void EXIT(int sig)
{

  logfile.Write("程序退出, sig=%d\n\n", sig);

  exit(0);
}

void _help(char *argv[])
{
  printf("\n");
  printf("Using:./main_mrudp1_1server_rpcclient logfilename xmlbuffer\n\n");

  printf("Sample0.1 [非DTU，发送方]:\n"\
		"./main_mrudp1_1server_rpcclient ../log/log_mrudp1_infohub_rpcclient_1.log \""\
			"<role>1</role>"\
			"<runtime>10000</runtime>"\
      "\"\n\n");
  printf("Sample0.2 [非DTU，接收方]:\n"\
		"./main_mrudp1_1server_rpcclient ../log/log_mrudp1_infohub_rpcclient_2.log \""\
			"<role>2</role>"\
			"<runtime>10000</runtime>"\
      "\"\n\n\n");

  printf("本程序是应急系统的辅助程序，用于运行时调试infohub信息。\n");
  printf("logfilename是本程序运行的日志文件。\n");
  printf("xmlbuffer为文件传输的参数，如下：\n");
  printf("  <role>1</role>    传输角色，1-发送方，2-接收方。\n");
	printf("  <runtime>10000</runtime>            最长待机时间\n");  
	printf("\n\n\n");  
}

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer)                                                                                       
{   
    memset(&starg,0,sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer,"role",&starg.role);
  if ( (starg.role!=1) && (starg.role!=2) ) starg.role=1;
  
  GetXMLBuffer(strxmlbuffer,"runtime",&starg.runtime);
  if ( (starg.runtime<=0) || (starg.runtime>60*60*24) ){ logfile.Write("runtime is error.\n"); return false; }

  return true;

}
