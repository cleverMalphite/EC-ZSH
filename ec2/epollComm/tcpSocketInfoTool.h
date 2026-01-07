#ifndef __GET_SOCKINFO__
#define __GET_SOCKINFO__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

/*

     struct tcp_info {
         __u8 tcpi_state; 			// TCP状态 //
         __u8 tcpi_ca_state; 		// TCP拥塞状态 //
         __u8 tcpi_retransmits; 		// 超时重传的次数 //
         __u8 tcpi_probes; 			// 持续定时器或保活定时器发送且未确认的段数//
         __u8 tcpi_backoff; 			// 退避指数 //
         __u8 tcpi_options; 			// 时间戳选项、SACK选项、窗口扩大选项、ECN选项是否启用//
     
         __u32 tcpi_rto; 			// 超时时间，单位为微秒//
         __u32 tcpi_ato; 			// 延时确认的估值，单位为微秒//
         __u32 tcpi_snd_mss; 		// 本端的MSS //
         __u32 tcpi_rcv_mss; 		// 对端的MSS //
     
         __u32 tcpi_unacked; 		// 未确认的数据段数，或者current listen backlog //
         __u32 tcpi_sacked; 			// SACKed的数据段数，或者listen backlog set in listen() //
         __u32 tcpi_lost; 			// 丢失且未恢复的数据段数 //
         __u32 tcpi_retrans; 		// 重传且未确认的数据段数 //
         __u32 tcpi_fackets; 		// FACKed的数据段数 //
                                      
         // Times. 单位为毫秒 //      
         __u32 tcpi_last_data_sent; 	// 最近一次发送数据包在多久之前 //
         __u32 tcpi_last_ack_sent;  	// 不能用。Not remembered, sorry. //
         __u32 tcpi_last_data_recv; 	// 最近一次接收数据包在多久之前 //
         __u32 tcpi_last_ack_recv; 	// 最近一次接收ACK包在多久之前 //
                                      
         // Metrics. //               
         __u32 tcpi_pmtu; 			// 最后一次更新的路径MTU //
         __u32 tcpi_rcv_ssthresh; 	// current window clamp，rcv_wnd的阈值 //
         __u32 tcpi_rtt; 			// 平滑的RTT，单位为微秒 //
         __u32 tcpi_rttvar; 			// 四分之一mdev，单位为微秒v //
         __u32 tcpi_snd_ssthresh; 	// 慢启动阈值 //
         __u32 tcpi_snd_cwnd; 		// 拥塞窗口 //
         __u32 tcpi_advmss; 			// 本端能接受的MSS上限，在建立连接时用来通告对端 //
         __u32 tcpi_reordering; 		// 没有丢包时，可以重新排序的数据段数 //
                                      
         __u32 tcpi_rcv_rtt; 		// 作为接收端，测出的RTT值，单位为微秒//
         __u32 tcpi_rcv_space;  		// 当前接收缓存的大小 //
                                      
         __u32 tcpi_total_retrans; 	// 本连接的总重传个数 //
     };

*/

//用于获取tcp socket的rtt等信息，即插即用
class tcpSocketInfoTool
{
public:
    tcpSocketInfoTool(int &sockfd){
    	// 获取tcp info
    	_optlen = sizeof(_tcpi_info);
    	if (getsockopt(sockfd, IPPROTO_TCP, TCP_INFO, &_tcpi_info, &_optlen) == -1) {
			_is_succeed = false;
    	    perror("Getsockopt TCP_INFO failed");
    	    close(sockfd);
    	    exit(EXIT_FAILURE);
    	}

		_is_succeed = true;
    }
	
	void dump(){
    	// 输出RTT信息
    	printf("TCP RTT: %u microseconds\n", _tcpi_info.tcpi_rtt);

    	// 输出丢包信息
    	printf("TCP Loss: %u retransmits\n", _tcpi_info.tcpi_retransmits);
	}

	uint32_t get_rtt(){
		return _tcpi_info.tcpi_rtt;
	} 

	uint32_t get_loss(){
		return _tcpi_info.tcpi_retransmits;
	}

private:
    bool        _is_succeed;
    tcp_info    _tcpi_info;
    socklen_t   _optlen;
};


#endif

