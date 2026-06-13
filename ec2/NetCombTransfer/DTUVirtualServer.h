#ifndef __DTU_VIRTUAL_SERVER__
#define __DTU_VIRTUAL_SERVER__

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <sys/types.h>
#include  <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <mutex>
#include <memory>

#include "IniHandleApi.h"




/*
 * DTU Virtual Server
 * create a map from local port to nat port
 * such as local port range : 7000~7999
 * while the nat port range : 8000~8999
 * so the transfer from local port to nat port is :
 *                              7000 -> 8000
 *                              7001 -> 8001
 */

class DTUVirtualServer{
public:
	DTUVirtualServer(){
		LocalPortBegin	=   GetIntegerKeyIni("DTU", "LocalPortBegin", 7000 );
		LocalPortEnd 	=   GetIntegerKeyIni("DTU", "LocalPortEnd", 7999 );
		NatPortBegin  	=   GetIntegerKeyIni("DTU", "NatPortBegin", 8000 );
		NatPortEnd    	=   GetIntegerKeyIni("DTU", "NatPortEnd", 8999 );

        printf("[DEBUG] DTUVirtualServer init:\n");
        printf("LocalPortBegin	=   %d\n", LocalPortBegin); printf("LocalPortEnd 	=   %d\n", LocalPortEnd  );
        printf("NatPortBegin  	=   %d\n", NatPortBegin  );
        printf("NatPortEnd    	=   %d\n", NatPortEnd    );
	}

	//初始化DTU虚拟服务器表
	DTUVirtualServer(int localbegin, int localend, int natbegin, int natend){

		LocalPortBegin	= localbegin;	
        LocalPortEnd 	= localend;
        NatPortBegin  	= natbegin;
        NatPortEnd    	= natend;
        printf("[DEBUG] DTUVirtualServer init:\n");
        printf("LocalPortBegin	=   %d\n", LocalPortBegin);
        printf("LocalPortEnd 	=   %d\n", LocalPortEnd  );
        printf("NatPortBegin  	=   %d\n", NatPortBegin  );
        printf("NatPortEnd    	=   %d\n", NatPortEnd    );

	}

	//获取DTU映射范围内的本地可用端口号
	uint16_t GetLocalPort(){

        printf("[DEBUG] DTUVirtualServer GetLocalPort():\n");

        printf("[DEBUG] DTUVirtualServer init test1\n");
        printf("LocalPortBegin	=   %d\n", LocalPortBegin);
        printf("[DEBUG] DTUVirtualServer init test2 \n");

        if(LocalPortBegin<0 ){
            printf("[DEBUG] DTUVirtualServer LocalPortBegin<0\n");
            return -1;
        } 
        printf("[DEBUG] DTUVirtualServer init test3 \n");

        printf("[DEBUG] DTUVirtualServer go througn from LocalPortBegin:%d to LocalPortEnd:%d\n",
               LocalPortBegin, LocalPortEnd);
	    for (int port = LocalPortBegin; port <= LocalPortEnd; ++port) {
            if (is_port_available(port)) {
                printf("[DEBUG] DTUVirtualServer get available port:%d\n",
                       port);
                return port;
            }
        }
        return -1;
	}

	//获取本地端口号对应的公网端口号
	uint16_t GetNatPort(const uint16_t localport){

        printf("\n[DEBUG] DTUVirtualServer GetNatPort():\n");
        printf("LocalPortBegin	=   %d\n", LocalPortBegin);
        printf("LocalPortEnd 	=   %d\n", LocalPortEnd  );
        printf("NatPortBegin  	=   %d\n", NatPortBegin  );
        printf("NatPortEnd    	=   %d\n", NatPortEnd    );

	    if(localport < LocalPortBegin || localport > LocalPortEnd)
            return -1;

        uint16_t natport = (localport - LocalPortBegin) + NatPortBegin ; 

        printf("\nlocalport:%d->natport:%d\n", localport, natport);

        return natport; 
	}
public:
    bool is_port_available(int port) {
        printf("DEBUG: is_port_availbel 1\n");
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            std::cerr << "Cannot create socket: " << strerror(errno) << std::endl;
            return false;
        }

        printf("DEBUG: is_port_availbel 2\n");
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        printf("DEBUG: is_port_availbel 3\n");
        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0) {
            printf("DEBUG: is_port_availbel 4\n");
            close(sock);
            return true;
        } else if (errno == EADDRINUSE) {
            printf("DEBUG: is_port_availbel 5\n");
            close(sock);
            return false;
        } else {
            std::cerr << "Bind failed: " << strerror(errno) << std::endl;
            close(sock);
            return false;
        }
    }

public:
	uint16_t LocalPortBegin;
	uint16_t LocalPortEnd;    
	uint16_t NatPortBegin;    
	uint16_t NatPortEnd;    
} ;





#endif
