#include "iodp_tcp.h"

//#pragma comment(lib, "ws2_32.lib")   //网络库文件

static void InitNetwork() {
	DWORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return;
	}
}

void serverThreadTask(void* arg)
{
    
    IODP_TCP_TYPE* pDev =  (IODP_TCP_TYPE*)arg;
    socklen_t addrlen;
    printf("start server %d\r\n",ntohs(pDev->serveraddr.sin_port));
    int rlen;
    uint8_t rbuf[1024];
    int ret=1;
    int tempret;
    while (1)
    {
        int acceptfd;
        struct sockaddr_in clientaddr;
        addrlen = sizeof(clientaddr);
        printf("accept ..\r\n");
        pDev->accept_fd = accept(pDev->sock_fd, (struct sockaddr *)&clientaddr, &addrlen);
        if (pDev->accept_fd == -1) {
            perror("fail to accept");
            exit(1);
        }
        printf("ip:%s ,port：%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
        pDev->clientaddr = clientaddr;
        while (1)
        {
            
            rlen = recv(pDev->accept_fd, rbuf,1024, 0);
            printf("recv %d\r\n",rlen);
            if(rlen<0){
                closesocket(pDev->accept_fd);
                printf("close\r\n");
                break;
            }
            
            eiodp_put(pDev->eiodp_fd, rbuf,rlen);
            ret = 1;
            while(ret!=0){
                ret = eiodp_process(pDev->eiodp_fd);
                if(ret == 100)
                {
                    tempret = send(pDev->accept_fd, pDev->eiodp_fd->send_buffer, pDev->eiodp_fd->send_len ,0 );
                    if(tempret == -1){
                        closesocket(pDev->accept_fd);
                        printf("close\r\n");
                        break;
                    }
                }
            }
            
        }
        

        
    }
    
}

//初始化
IODP_TCP_TYPE* iodptcp_init(unsigned int mode, const char* ip, uint16_t port)
{
    InitNetwork();
    
    printf("iodptcp_init %s %d\r\n",ip, port);
    //第一步：通过socket函数创建一个TCP套接字
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd  == -1) {
        printf("fail to socket\r\n");
		exit(1);
	}
    //第二步：将套接字与服务器网络信息结构体绑定
	struct sockaddr_in serveraddr;
	socklen_t addrlen = sizeof(serveraddr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(ip);
	serveraddr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&serveraddr, addrlen) == -1)
	{
		perror("fail to bind");
		exit(1);
	}
    printf("listen\r\n");
    //第三步：将套接字设置为被动监听状态
	if (listen(sockfd, 10) == -1) {
		perror("fail to listen");
		exit(1);
	}

    IODP_TCP_TYPE* pDev = (IODP_TCP_TYPE*)malloc(sizeof(IODP_TCP_TYPE));
    pDev->sock_fd = sockfd;
    pDev->serveraddr = serveraddr;
    pDev->eiodp_fd = eiodp_init(mode, NULL,NULL);
    printf("pthread_create\r\n");
    int ret = pthread_create(&pDev->pth, NULL, serverThreadTask,pDev);

    return pDev;
}

//添加服务函数
int32_t iodptcp_addFunc(IODP_TCP_TYPE* tcpdp, uint32_t cmd, int (*callbackFunc)(eIODP_FUNC_MSG msg))
{
    return eiodp_get_register(tcpdp->eiodp_fd, cmd, callbackFunc);
}



void iodptcp_close(IODP_TCP_TYPE* tcpdp)
{
    pthread_cancel(tcpdp->pth);
    eiodp_destroy(tcpdp->eiodp_fd);
    free(tcpdp);
}