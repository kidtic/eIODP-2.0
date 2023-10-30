#include "iodp_tcp_linux.h"



static void serverThreadTask(void* arg)
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
                close(pDev->accept_fd);
                printf("close\r\n");
                break;
            }
            
            eiodp_put(pDev->eiodp_fd, rbuf,rlen);
            ret = 1;
            while(ret!=0){
                ret = eiodp_process(pDev->eiodp_fd);
                if(ret == -100)
                {
                    close(pDev->accept_fd);
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
    //printf("recv %d:",fd);
    int ret = 0;
    ret = recv(fd,buf,size,0);
    if(ret>=0){
        return ret;
    }
    else{
        return 0;
    }
     
}

int tcpsend(int fd, char* buf, int size)
{
    int i=0;
    /*
    printf("send %d:",fd);
    for(i=0; i<size;i++)
    {
        if(i%8==0)printf("\r\n");
        printf("%02x ",buf[i]);
    }
    printf("\r\n");
    */
    //printf("send %d:",fd);
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
    
    printf("iodptcp_init %s %d\r\n",ip, port);
    //通过socket函数创建一个TCP套接字
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd  == -1) {
        printf("fail to socket\r\n");
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
		return NULL;
	}
    printf("listen\r\n");
    //将套接字设置为被动监听状态
	if (listen(sockfd, 10) == -1) {
		perror("fail to listen");
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