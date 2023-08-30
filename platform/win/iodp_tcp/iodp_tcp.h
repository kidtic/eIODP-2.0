#ifndef _IODPTCP_H_
#define _IODPTCP_H_

#include "stdio.h"
#include "stdlib.h"
#include "winsock2.h"
#include "eiodp.h"
#include "pthread.h"
#include <sys/types.h>
#include <ws2tcpip.h>
#include "windows.h"



typedef struct 
{
    int sock_fd;
    int accept_fd;  //接收连接后的句柄
    struct sockaddr_in serveraddr;  //保存服务器信息
    struct sockaddr_in clientaddr;  //保存对端信息
    eIODP_TYPE* eiodp_fd;
    pthread_t pth;

}IODP_TCP_TYPE;



//初始化
IODP_TCP_TYPE* iodptcp_init_server(const char* ip, uint16_t port);

//添加服务函数
int32_t iodptcp_addFunc(IODP_TCP_TYPE* tcpdp, uint32_t cmd, int (*callbackFunc)(eIODP_FUNC_MSG msg));

void iodptcp_close(IODP_TCP_TYPE* tcpdp);


#endif