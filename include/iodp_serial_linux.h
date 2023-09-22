#ifndef _IODPTTY_H_
#define _IODPTTY_H_

#include "stdio.h"
#include "stdlib.h"
#include "eiodp.h"
#include "pthread.h"
#include "fcntl.h"
#include "time.h"
#include "string.h"
#include <termios.h>
#include "unistd.h"




typedef struct 
{
    int ttyfd;
    int mode;
    eIODP_TYPE* eiodp_fd;
    pthread_t pth;
    int pth_flag;
}IODP_TTY_TYPE;


/***********************************************************************
 * @brief 串口iodp初始化函数
 * @param iMode 0服务端，1客户端
 * @param devname 串口设备名,一般为"/dev/***"
 * @param baud 波特率
 * @param nEvent 校验位 'N' 'O' 'E'
 * @return IODP_TTY_TYPE* 
 ***********************************************************************/
IODP_TTY_TYPE* iodptty_init(int iMode ,const char* devname, uint32_t baud, char nEvent);

/***********************************************************************
 * @brief 添加服务函数
 * @param pDev 通讯句柄
 * @param cmd 命令字
 * @param callbackFunc 回调函数
 * @return int32_t <0 错误 0成功
 ***********************************************************************/
int32_t iodptty_addFunc(IODP_TTY_TYPE* pDev, uint32_t cmd, int (*callbackFunc)(eIODP_FUNC_MSG msg));

/***********************************************************************
 * @brief 销毁iodptty
 * @param pDev 通讯句柄
 ***********************************************************************/
void iodptty_destory(IODP_TTY_TYPE* pDev);

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
int32_t iodptty_request_get(
    IODP_TTY_TYPE* pDev, uint32_t cmd, 
    uint8_t* data, uint32_t len, 
    uint8_t* rdata, uint32_t maxretlen
);


#endif