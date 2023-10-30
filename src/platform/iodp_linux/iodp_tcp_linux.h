#ifndef _IODPTCP_H_
#define _IODPTCP_H_

#include "stdio.h"
#include "stdlib.h"
#include "eiodp.h"
#include "pthread.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <fcntl.h>



typedef struct 
{
    int sock_fd;
    int accept_fd;  //接收连接后的句柄
    struct sockaddr_in serveraddr;  //保存服务器信息
    struct sockaddr_in clientaddr;  //保存对端信息
    eIODP_TYPE* eiodp_fd;
    pthread_t pth;
    int mode;

}IODP_TCP_TYPE;



/***********************************************************************
 * @brief 初始化服务端接口
 * @param ip 本地IP地址 默认0.0.0.0
 * @param port 端口号
 * @return IODP_TCP_TYPE* 
 ***********************************************************************/
IODP_TCP_TYPE* iodptcp_init_server(const char* ip, uint16_t port);

/***********************************************************************
 * @brief 初始化客户端接口
 * @return IODP_TCP_TYPE* 
 ***********************************************************************/
IODP_TCP_TYPE* iodptcp_init_client(void);

/***********************************************************************
 * @brief 客户端用于连接服务器
 * @param pDev tcpiodp通讯句柄
 * @param ip 远程服务端ip
 * @param port 远程服务端 端口号
 * @return int32_t 
 ***********************************************************************/
int32_t iodptcp_connect(IODP_TCP_TYPE* pDev, const char* ip, uint16_t port);

/***********************************************************************
 * @brief 添加服务函数
 * @param tcpdp tcpiodp通讯句柄
 * @param cmd 命令子
 * @param callbackFunc 回调函数
 * @return int32_t <0 错误 0成功
 ***********************************************************************/
int32_t iodptcp_addFunc(IODP_TCP_TYPE* tcpdp, uint32_t cmd, int (*callbackFunc)(eIODP_FUNC_MSG msg));

/***********************************************************************
 * @brief 关闭
 * @param tcpdp tcpiodp通讯句柄
 ***********************************************************************/
void iodptcp_close(IODP_TCP_TYPE* tcpdp);



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
);


#endif