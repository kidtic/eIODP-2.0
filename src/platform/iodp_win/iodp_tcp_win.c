#include "iodp_tcp_win.h"


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
        eiodp_rwfd(pDev->eiodp_fd, pDev->accept_fd);
        while (1)
        {
            
            rlen = recv(pDev->accept_fd, rbuf,1024, 0);
            printf("recv %d %d\r\n",rlen,pDev->accept_fd);
            if(rlen<=0){
                closesocket(pDev->accept_fd);
                printf("close\r\n");
                break;
            }
            
            eiodp_put(pDev->eiodp_fd, rbuf,rlen);
            ret = 1;
            while(ret!=0){
                ret = eiodp_process(pDev->eiodp_fd);
                if(ret == -100)
                {
                    closesocket(pDev->accept_fd);
                    printf("close\r\n");
                    break;
                }
            }
            printf("new-0\r\n");
        }
        

        
    }
    
}

//用于向eiodp注册读写函数
int tcprecv(int fd, char* buf, int size)
{
    return recv(fd,buf,size,0);
}

int tcpsend(int fd, uint8_t* buf, int size)
{
    int i=0;
    printf("send %d:",fd);
    for(i=0; i<size;i++)
    {
        if(i%8==0)printf("\r\n");
        printf("%02x ",buf[i]);
    }
    printf("\r\n");
    return send(fd,buf,size,0);
}


/***********************************************************************
 * @brief 初始化服务端接口
 * @param ip 本地IP地址 默认0.0.0.0
 * @param port 端口号
 * @return IODP_TCP_TYPE* 
 ***********************************************************************/
IODP_TCP_TYPE* iodptcp_init_server(const char* ip, uint16_t port)
{
    InitNetwork();
    
    printf("iodptcp_init %s %d\r\n",ip, port);
    //通过socket函数创建一个TCP套接字
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd  == -1) {
        printf("fail to socket\r\n");
        WSACleanup();
		return NULL;
	}
    //将套接字与服务器网络信息结构体绑定
	struct sockaddr_in serveraddr;
	socklen_t addrlen = sizeof(serveraddr);
	serveraddr.sin_family = AF_INET;
    if(ip == NULL)
    {
        serveraddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    }
    else{
        serveraddr.sin_addr.s_addr = inet_addr(ip);
    }
	serveraddr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&serveraddr, addrlen) == -1)
	{
		perror("fail to bind");
        WSACleanup();
		return NULL;
	}
    printf("listen\r\n");
    //将套接字设置为被动监听状态
	if (listen(sockfd, 10) == -1) {
		perror("fail to listen");
        WSACleanup();
		return NULL;
	}

    IODP_TCP_TYPE* pDev = (IODP_TCP_TYPE*)malloc(sizeof(IODP_TCP_TYPE));
    pDev->sock_fd = sockfd;
    pDev->serveraddr = serveraddr;
    pDev->mode = IODP_MODE_SERVER;
    //服务端只需要发送便可
    pDev->eiodp_fd = eiodp_init(IODP_MODE_SERVER, NULL,tcpsend,sockfd);
    printf("pthread_create\r\n");
    int ret = pthread_create(&pDev->pth, NULL, serverThreadTask,pDev);

    return pDev;
}


/***********************************************************************
 * @brief 初始化客户端接口
 * @return IODP_TCP_TYPE* 
 ***********************************************************************/
IODP_TCP_TYPE* iodptcp_init_client(void)
{
    InitNetwork();
    //通过socket函数创建一个TCP套接字
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd  == -1) {
        printf("fail to socket\r\n");
        WSACleanup();
		return NULL;
	}

    //设置Socket为非阻塞模式
	int iMode = 1;
	int retVal = ioctlsocket(sockfd,FIONBIO,(u_long FAR*)&iMode);
	if (retVal == -1)
	{
		printf("ioctlsocket failed!\n");
		WSACleanup();
		return -1;
	}


    IODP_TCP_TYPE* pDev = (IODP_TCP_TYPE*)malloc(sizeof(IODP_TCP_TYPE));
    if(pDev==NULL)
    {
        closesocket(sockfd);
        return NULL;
    }
    pDev->sock_fd = sockfd;
    pDev->mode = IODP_MODE_CLIENT;
    pDev->eiodp_fd = eiodp_init(IODP_MODE_CLIENT, tcprecv,tcpsend,sockfd);

    return pDev;
}

/***********************************************************************
 * @brief 客户端用于连接服务器
 * @param pDev tcpiodp通讯句柄
 * @param ip 远程服务端ip
 * @param port 远程服务端 端口号
 * @return int32_t 
 ***********************************************************************/
int32_t iodptcp_connect(IODP_TCP_TYPE* pDev, const char* ip, uint16_t port)
{
    if(pDev == NULL || ip == NULL)
    {
        return -1;
    }
    if(pDev->mode != IODP_MODE_CLIENT)
    {
        return -2;
    }
    int ret;
    struct sockaddr_in server_addr = {0};	
	server_addr.sin_family = AF_INET;                       // 设置地址族为IPv4
	server_addr.sin_port = htons(port);						// 设置地址的端口号信息
	server_addr.sin_addr.s_addr = inet_addr(ip);	        //　设置IP地址
    // 3、链接到服务器
    ret = connect(pDev->sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0){
        perror("connect");
        return -3;
    }
    else{
        printf("connect result, ret = %d.\n", ret);
        return 0;
    }
	    
}

/***********************************************************************
 * @brief 客户端调用的接口，GET请求
 * @param pDev 协议栈句柄
 * @param cmd 请求命令字
 * @param data 输入参数
 * @param len 输入参数长度
 * @param retData 返回数据
 * @param maxretlen retData的容量
 * @return int32_t 返回数据长度  负数为错误
 ***********************************************************************/
int32_t iodptcp_request_get(
    IODP_TCP_TYPE* pDev, uint32_t cmd, 
    uint8_t* data, uint32_t len, 
    uint8_t* rdata, uint32_t maxretlen
)
{
    if(pDev == NULL)
    {
        return -1;
    }
    return eiodp_request_GET(pDev->eiodp_fd,cmd,data,len,rdata,maxretlen);
}

/***********************************************************************
 * @brief 添加服务函数
 * @param tcpdp tcpiodp通讯句柄
 * @param cmd 命令子
 * @param callbackFunc 回调函数
 * @return int32_t <0 错误 0成功
 ***********************************************************************/
int32_t iodptcp_addFunc(IODP_TCP_TYPE* tcpdp, uint32_t cmd, int (*callbackFunc)(eIODP_FUNC_MSG msg))
{
    if(tcpdp == NULL)
    {
        return -1;
    }
    if(tcpdp->mode != IODP_MODE_SERVER)
    {
        return -2;
    }
    return eiodp_get_register(tcpdp->eiodp_fd, cmd, callbackFunc);
}


/***********************************************************************
 * @brief 关闭
 * @param tcpdp tcpiodp通讯句柄
 ***********************************************************************/
void iodptcp_close(IODP_TCP_TYPE* tcpdp)
{
    if(tcpdp == NULL)
    {
        return -1;
    }
    pthread_cancel(tcpdp->pth);
    eiodp_destroy(tcpdp->eiodp_fd);
    free(tcpdp);
}