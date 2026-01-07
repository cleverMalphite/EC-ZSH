#include <iostream>
#include <cstdio>
#include "EpollThreads.h"
#include "CNetCommEpoll.h"

extern CNetCommEpoll *gNetCommManager;
/*std::shared_ptr<FILE> pSourceFile(fopen("/home/ubuntu/test.pdf", "r"));*/
/*std::shared_ptr<FILE> pTestFile(fopen("/home/ubuntu/TestFile.txt", "w+b"));*/
DWORD filelength = 0;
DWORD callbackCounts = 0;
FILE *pTestFile2 /*= fopen("/home/ubuntu/TestFile2.txt", "a+")*/;

//PDATACALLBACK
//--------------------------------------------------------------------------------------------------
bool CallBackTest(DWORD nChannel, const std::shared_ptr<BYTE>& pBuffer, DWORD nLength, bool isNeedSlit) {
    int ret = 0;
    callbackCounts++;
    if (!(pBuffer && pBuffer.get() && pTestFile2 /*&& pTestFile.get()*/)) {
        perror("CallBack False\n");
    } else {
        /*ret = fputs((char *) pBuffer.get(), pTestFile.get());*/
        ret = fputs((char *) pBuffer.get(), pTestFile2);
    }
    if (ret < 0) {
        return false;
    } else {
        filelength += nLength;
        printf("callbackcounts:%d\n", callbackCounts);
        printf("size:%d\n", filelength);
        return true;
    }
}
//--------------------------------------------------------------------------------------------------

/*TEST(ipsockTest, setsock) {
    //EXPECT_EQ(1, testsock.mLocalSock);
    EXPECT_EQ(false,testsock.setsockNoblock(0));
}*/

//#define GoogleTest_1
#ifdef GoogleTest_1
std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
CNetCommEpoll *testNetCommManager = NetCommManager.get();

TEST(ChannelTest, channelList) {
    EXPECT_TRUE(testNetCommManager->CreateTCP());
    auto iter = testNetCommManager->mTcpList->NetChannel.begin();
    printf("size{%zu}\n", testNetCommManager->mTcpList->NetChannel.size());
    void *ptr = malloc(256);
    free(ptr);
    EXPECT_DEATH(*(char *) ptr, "");
    int a[10] = {0};
    EXPECT_EQ(0, a[12]);
    auto iter2 = testNetCommManager->mTcpList->NetChannel.begin();
    printf("size{%zu}\n", testNetCommManager->mTcpList->NetChannel.size());
    //EXPECT_EQ(*(iter+1), *iter2);
}

#endif

//#define NETCOMM_CHANNEL_TEST
#ifdef NETCOMM_CHANNEL_TEST

int main(int argc, char *argv[]) {
    if (*argv[1] == 's') {
        std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
        gNetCommManager = NetCommManager.get();
        DWORD Port = 9999;
        CreateTcpServerChannel(Port, "127.0.0.1", 3200, TcpMaxChannelNum, true);
        pthread_t pid;
        pthread_create(&pid, NULL, TcpWorkerThread, gNetCommManager);
        DWORD Port2 = 9998;
        DWORD ClientPort = 9997;
        DWORD RemotePort = 9996;
        //CreateTcpServerChannel(Port, "127.0.0.1", 3250, TcpMaxChannelNum, true);
        std::shared_ptr<CTCP> temptcpchannel = nullptr;
        int pscount = 0;
        int serverchannelnum = 0;
        while (1) {
            sleep(3);
            serverchannelnum = 0;
            int nowChannelNum = gNetCommManager->mTcpList->GetNetnum();
            printf("NetComm's TcpList has %d TcpChannel：\n", nowChannelNum);
            for (int i = 0; i < nowChannelNum; i++) {
                temptcpchannel = gNetCommManager->mTcpList->NetChannel[i];
                if (temptcpchannel) {
                    if (temptcpchannel->tcp_IsServer()) {
                        serverchannelnum++;
                        printf("Server %d ,Channel Use:%d/%d\n", serverchannelnum,
                               temptcpchannel->m_dwChannelCurrentNumber,
                               temptcpchannel->m_dwChannelNumber);
                    }
                    if (i == 0) {
                        printf("Server %d {%ld}\n", serverchannelnum, temptcpchannel->GetChannel());
                    } else {
                        printf("TransChannel %d {%ld}\n", i, temptcpchannel->GetChannel());
                    }
                }
            }
            pscount++;
            if (pscount == 6) {
                printf("======Ready to Create New Tcp Server======\n");
                CreateTcpServerChannel(Port2, "127.0.0.1", 3400, TcpMaxChannelNum, true);
            }
            /*if (pscount == 6) {
                printf("======Ready to Create New Tcp Client======\n");
                CreateTcpClientChannel(ClientPort, RemotePort, "127.0.0.1", "127.0.0.1", 3249, false);
            }*/
            if (pscount == 12) {
                printf("======Ready to Close Tcp Server======\n");
                CloseTcpChannel(3200);
            }
        }
    }
    if (*argv[1] == 'c') {
        std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
        gNetCommManager = NetCommManager.get();
        pthread_t pid;
        pthread_create(&pid, NULL, TcpWorkerThread, gNetCommManager);
        DWORD Port = 8888;
        DWORD RemotePort = 9999;
        //TcpChannel:3000——3999
        //isForceChannelPort?
        CreateTcpClientChannel(Port, RemotePort, "127.0.0.1", "127.0.0.1", 3501, false);
        printf("Serverport is %ld\n", Port);
        int i = 0;
        while (i < 2) {
            sleep(5);
            i++;
        }
        CloseTcpChannel(3501);
        while (1) {
            sleep(5);
        }
    }
    return 0;
}

#endif

//#define UDP_BCAST_TEST
#ifdef UDP_BCAST_TEST
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("need more params");
        return 0;
    }
    DWORD BCastPort = 9999;
    if (*argv[1] == 's') {

        int BCastServerChannel1 = 900;
        std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
        gNetCommManager = NetCommManager.get();
        pthread_t pid;
        pthread_create(&pid, nullptr, BCastWorkerThread, gNetCommManager);
        unsigned char sendata[1024] = {"hello,this is bcast server"};
        std::shared_ptr<BYTE> sendbuf(sendata);
        bool ret = CreateBCastServerChannel(BCastPort, "10.0.12.6", "255.255.255.255", BCastServerChannel1);
        if (!ret) {
            perror("Create Bcast Server Channel");
        }
        int length = 0;
        while (1) {
            sleep(2);
            gNetCommManager->WriteBCastChannel(BCastServerChannel1, sendbuf, sizeof(sendata));
        }
    } else if ('c' == *argv[1]) {

        std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
        pthread_t pid;
        gNetCommManager = NetCommManager.get();
        bool ret = CreateUdpClientChannel(BCastPort, 0, "10.0.12.6", "", 100, true);//接收广播的udp的端口号要和广播地址的端口号对应
        if (!ret) {
            perror("Create UdpChannel Failed");
        }
        pthread_create(&pid, nullptr, UdpWorkerThread, gNetCommManager);
        unsigned char sendata[1024] = {"hello UdpChannel"};
        std::shared_ptr<BYTE> sendbuf(sendata);
        while (1) {
            sleep(1);
            //NetCommManager->WriteUdpChannel(100, sendbuf, sizeof(sendata));
        }
    }
}
#endif

#define NETCOMM_UDP_TEST
#ifdef NETCOMM_UDP_TEST

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("need more params");
        return 0;
    }
    if (*argv[1] == 's') {

        DWORD LocalPort1 = 9999;
        int UdpServerChannel1 = 900;
        DWORD LocalPort2 = 8888;
        int UdpServerChannel2 = 800;
        DWORD LocalPort3 = 7777;
        int UdpServerChannel3 = 700;
        std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
        gNetCommManager = NetCommManager.get();
        pthread_t pid, pid2, pid3, pid4, pid5, pid6;
        pthread_create(&pid, nullptr, UdpWorkerThread, gNetCommManager);
        pthread_create(&pid2, nullptr, UdpWorkerThread, gNetCommManager);
        pthread_create(&pid3, nullptr, UdpWorkerThread, gNetCommManager);
        pthread_create(&pid4, nullptr, UdpWorkerThread, gNetCommManager);
        pthread_create(&pid5, nullptr, UdpWorkerThread, gNetCommManager);
        pthread_create(&pid6, nullptr, UdpWorkerThread, gNetCommManager);
        sleep(1);
        bool ret = CreateUdpServerChannel(LocalPort1, "127.0.0.1", UdpServerChannel1, true);
        if (!ret) {
            perror("Create UdpServer Channel");
        }
        /*ret = CreateUdpServerChannel(LocalPort2, "127.0.0.1", UdpServerChannel2, true);
        if (!ret) {
            perror("Create UdpServer Channel");
        }
        ret = CreateUdpServerChannel(LocalPort3, "127.0.0.1", UdpServerChannel3, true);
        if (!ret) {
            perror("Create UdpServer Channel");
        }*/
        int length = 0;
        unsigned char sendata[1400] = {"hello UdpChannel"};
        std::shared_ptr<BYTE> sendbuf(sendata);
        sleep(10);
        DWORD counts = 0;
        sleep(3);
        while (1) {
            /*usleep(10);
            counts++;
            if (counts == 100000) {
                CloseUdpChannel(700);
            }
            if (counts == 200000) {
                CloseUdpChannel(800);
            }
            if (counts == 300000) {
                CloseUdpChannel(900);
            }*/
            //NetCommManager->WriteUdpChannel(UdpServerChannel1, sendbuf, sizeof(sendata));
            /*NetCommManager->WriteUdpChannel(800, sendbuf, sizeof(sendata));
            NetCommManager->WriteUdpChannel(900, sendbuf, sizeof(sendata));*/
        }
    } else if ('c' == *argv[1]) {
        DWORD LocalPort = 8888;
        std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
        gNetCommManager = NetCommManager.get();
        bool ret = CreateUdpClientChannel(LocalPort, 9999, "127.0.0.1", "127.0.0.1", 100, true);
        if (!ret) {
            perror("Create UdpChannel Failed");
        }
        /*ret = CreateUdpClientChannel(LocalPort, 8888, "127.0.0.1", "127.0.0.1", 101, false);
        if (!ret) {
            perror("Create UdpChannel Failed");
        }
        ret = CreateUdpClientChannel(LocalPort, 7777, "127.0.0.1", "127.0.0.1", 102, false);
        if (!ret) {
            perror("Create UdpChannel Failed");
        }*/
        pthread_t pid;
        pthread_create(&pid, nullptr, UdpWorkerThread, gNetCommManager);
        unsigned char sendata[1400] = {"hello UdpChannel"};
        std::shared_ptr<BYTE> sendbuf(sendata);
        while (1) {
            //sleep(1);
            NetCommManager->WriteUdpChannel(100, sendbuf, sizeof(sendata));
            /*NetCommManager->WriteUdpChannel(101, sendbuf, sizeof(sendata));
            NetCommManager->WriteUdpChannel(102, sendbuf, sizeof(sendata));*/
        }
    }
}

#endif

//#define NETCOMM_TCP_TEST
#ifdef NETCOMM_TCP_TEST
//todo：TCP方式下Eventdataex的方式冗余，需要删除。
int length = 1400;
std::shared_ptr<BYTE> sendbuf(new BYTE[length], releaseArrays<BYTE>);
char sendata[1400] = {"nihao～"};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("need more params...\n");
        exit(0);
    }
    signal(SIGPIPE, SIG_IGN);
    pthread_t pid;
    std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
    gNetCommManager = NetCommManager.get();
    gNetCommManager->SetInterfaceParam(nullptr, CallBackTest);
    if (*argv[1] == 's') {
        DWORD localport = 9999;
        printf("server...\n");
        CreateTcpServerChannel(localport, "127.0.0.1", 3000, TcpMaxChannelNum, true);
        pthread_create(&pid, NULL, TcpWorkerThread, gNetCommManager);
        memcpy(sendbuf.get(), sendata, length);
        std::shared_ptr<CTCP> temptcpchannel = nullptr;
        DWORD count = 0;
        pTestFile2 = fopen("/home/ubuntu/TestFile2.txt", "w+");
        while (count < 5) {
            sleep(2);
            count++;
            /*int serverchannelnum = 0;
            int nowChannelNum = gNetCommManager->mTcpList->GetNetnum();
            printf("====================================================\n");
            printf("NetComm's TcpList has %d TcpChannel：\n", nowChannelNum);
            for (int i = 0; i < nowChannelNum; i++) {
                temptcpchannel = gNetCommManager->mTcpList->NetChannel[i];
                if (temptcpchannel) {
                    if (temptcpchannel->tcp_IsServer()) {
                        serverchannelnum++;
                        printf("Server %d ,Channel Use:%ld/%ld\n", serverchannelnum,
                               temptcpchannel->m_dwChannelCurrentNumber,
                               temptcpchannel->m_dwChannelNumber);
                    }
                    if (i == 0) {
                        printf("Server %d {%ld}\n", serverchannelnum, temptcpchannel->GetChannel());
                    } else {
                        printf("TransChannel %d {%ld}\n", i, temptcpchannel->GetChannel());
                    }
                }
            }
            printf("====================================================\n");*/
            //gNetCommManager->WriteTcpChannel(3001, sendbuf, length);
            /*count++;
            if (count == 400000) {
                CloseTcpChannel(3001);
            }*/
        }
        fclose(pTestFile2);
    }
    if (*argv[1] == 'c') {
        printf("client...\n");
        pthread_create(&pid, NULL, TcpWorkerThread, gNetCommManager);
        DWORD localport = 8888;
        CreateTcpClientChannel(localport, 9999, "127.0.0.1", "127.0.0.1", 3050, true);
        //CreateTcpClientChannel(localport, 9999, "127.0.0.1", "127.0.0.1", 3050, true);
        memcpy(sendbuf.get(), sendata, length);
        while (1) {
            //usleep(1);
            /* if (fgets(sendata, 1400 + 1, pSourceFile.get()) == nullptr) {
                 perror("");
                 break;
             }
             memcpy(sendbuf.get(), sendata, length);*/
            usleep(10000);
            gNetCommManager->WriteTcpChannel(3050, sendbuf, length);
        }
    }
    return 0;
}

#endif

/*
int main() {
    Init_NetComm(nullptr, nullptr, 10);
    sleep(10);
    UnInit_NetComm();
    printf("Test over\n");
    return 0;
}*/
