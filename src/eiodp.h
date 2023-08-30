#ifndef _EIODP_H_
#define _EIODP_H_
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

/*============================config区域==============================*/
//定义接受环形缓存大小
#define IODP_RECV_MAX_LEN 1024
//定义返回接受包DATA大小
#define IODP_RETDATA_BUFFERSIZE 2048
//定义发送区域大小
#define IODP_SEND_BUFFERSIZE 1024
//定义大小端
#define IODP_ENDIAN             0       //0大端     1小端  


//malloc
#define MOONOS_MALLOC(size) malloc(size)
#define MOONOS_FREE(P) free(P)

/*====================================================================*/


/*===============================协议相关==============================*/
#define IODP_PTC_MAJOR          0
#define IODP_PTC_VERSION        0       //协议版本迭代

#define IODP_PTC_TYPE_GET       0X01    //get格式的包
#define IODP_PTC_TYPE_POST      0X02    //POST格式的包
#define IODP_PTC_TYPE_RETURN    0X03    //RETURN格式的包
#define IODP_PTC_TYPE_ERROR     0X04    //ERROR格式的包
#define IODP_PTC_TYPE_ACK       0X05    //ACK格式的包

/*====================================================================*/





#define IODP_MODE_CLIENT    1
#define IODP_MODE_SERVER    0

//--------------error code 
#define IODP_OK 0
#define IODP_ERROR_PARAM -10            //参数错误
#define IODP_ERROR_HEAPOVER -11         //MALLOC错误
#define IODP_ERROR_REPEATCODE -12       //有重复cmd
#define IODP_ERROR_TIMEOUT -13
#define IODP_ERROR_SMOLL_RECVLEN -14
#define IODP_ERROR_RECVLEN -15
#define IODP_ERROR_RETCODE -16
#define IODP_ERROR_PKT -17      //返回了一个error数据包
#define IODP_ERROR_NORET -18
#define IODP_ERROR_API_HEAD -19 
#define IODP_ERROR_RADDR_HEAD -20 
#define IODP_ERROR_WADDR_HEAD -21

#define IODP_ERROR_APINODE_REPEAT -22       //有重复cmd


//--------------process code process函数返回
#define IODP_PROC_STATUS_NORECV     0       //代表recvringbuf没有数据
#define IODP_PROC_STATUS_HEAD       1       //代表正在抓取头帧中
#define IODP_PROC_STATUS_TYPE       2       //代表正在接收格式帧中
#define IODP_PROC_STATUS_DATA       3       //代表正在接收数据中
#define IODP_PROC_STATUS_CHECK      4       //代表正在接收检验数据中

#define IODP_PROC_ERROR_LEN         -1      //解析得到的数据长度不符合要求
#define IODP_PROC_ERROR_VER         -2      //版本不兼容
#define IODP_PROC_ERROR_TYPE        -3      //包类型不识别
#define IODP_PROC_ERROR_CHECK       -4      //校验失败
#define IODP_PROC_ERROR_CONTENT     -5      //内容格式错误
#define IODP_PROC_ERROR_SEND        -100    //发送错误


//--------------package error code 放在错误类型帧上的错误代码，用户返回客户端
#define IODP_ERRFREAM_NOCMD         0x81        //代表服务端没找到对应cmd


#define IODP_LOG printf
#define IODP_LOGMSG(str) printf(str)

//func响应入参
typedef struct
{
    uint32_t cmd;
    uint32_t len;
    const uint8_t* data;
    //返回数据
    uint32_t* retlen;
    uint8_t* retdata;
}eIODP_FUNC_MSG;


//eiodp循环缓冲buffer
typedef struct
{
    uint32_t bufSize;

    uint8_t *buf;
    uint32_t pIn; //环形缓冲的头指针
    uint32_t pOut; //环形缓冲的尾指针

}eIODP_RING;

//eiodp服务函数链表结构
typedef struct
{
    uint32_t cmd;
    //回调函数 len，data为输入  retdata是需要输出的数据
    int (*callbackFunc)(eIODP_FUNC_MSG msg); 
    void* pNext;
}eIODP_FUNC_NODE;

//return包的DATA区域格式
typedef struct
{
    uint32_t cnt;
    uint32_t allpack;
    uint32_t len;
    uint8_t data[IODP_RETDATA_BUFFERSIZE];
}eIODP_RETDATA_TYPE;
//get包的DATA区域格式
typedef struct
{
    uint32_t cnt;
    uint32_t cmd;
    uint32_t len;
    uint8_t data[IODP_RETDATA_BUFFERSIZE];
}eIODP_GETDATA_TYPE;

//总包的格式
typedef struct
{
    uint8_t head[4];
    uint8_t ver[3];
    uint8_t type;
    uint32_t len;
    void* data;
    uint32_t check;
}eIODP_PACK_TYPE;


//eiodp服务
typedef struct
{
    uint32_t mode;
    eIODP_RING*  recv_ringbuf;
    
    int32_t ana_size;        //已经解析了的存入了analyze_data的数据
    int32_t anadata_status;  //目前analyze_data的状态 0表示在解析中 1表示解析完成analyze_data中的数据是完整数据(除了头帧以外)
    eIODP_PACK_TYPE analyze_data;   //用于解析记录状态的结构体
    uint8_t content_data[IODP_RETDATA_BUFFERSIZE+12];    //主要内容数据  可以解析成不同形式
    
    
    uint8_t send_buffer[IODP_SEND_BUFFERSIZE];
    uint32_t send_len;      //send_buffer里要发送的数量

    //注册的服务函数链表
    eIODP_FUNC_NODE* pFuncHead;

    //iodevHandle设备的收发函数 一定要是非阻塞的
    int (*iodevRead)(int ,char*, int);
    int (*iodevWrite)(int ,char*, int);
    int rw_fd;

}eIODP_TYPE;





/***********************************************************************
 * @brief 初始化框架
 * @param mode 0服务器 1客户端
 * @param readfunc 读取函数 服务端可以为空
 * @param writefunc 发送函数
 * @param rw_fd 通讯句柄，用于发送与接收函数
 * @return eIODP_TYPE* 创建的eIODP_TYPE指针，可以通过这个指针来操作iodp
 ***********************************************************************/
eIODP_TYPE* eiodp_init(unsigned int mode, int (*readfunc)(char*, int),
                int (*writefunc)(char*, int),int rw_fd);

/***********************************************************************
 * @brief 销毁句柄
 * @param eiodp_fd eiodp句柄
 * @return int32_t 0成功
 ***********************************************************************/
int32_t eiodp_destroy(eIODP_TYPE* eiodp_fd);

/***********************************************************************
 * @brief 修改通讯句柄
 * @param rw_fd 
 * @return int32_t 
 ***********************************************************************/
int32_t eiodp_rwfd(eIODP_TYPE* eiodp_fd, int rw_fd);

/************************************************************
    @brief:
        注册服务函数
    @param:
        eiodp_fd:eiodp句柄
        cmd：API代码
        callbackFunc:服务函数
    @return:
        <0 - 失败（error code）
         0 - 成功
*************************************************************/
int32_t eiodp_get_register(eIODP_TYPE* eiodp_fd,uint16_t cmd,
                int (*callbackFunc)(eIODP_FUNC_MSG msg));

/************************************************************************
 * @brief 往协议栈输入数据
 * @param eiodp_fd 协议栈句柄
 * @param data 数据
 * @param size 数据长度
 * @return int32_t 实际插入数据
 ************************************************************************/
int32_t eiodp_put(eIODP_TYPE* eiodp_fd, uint8_t* data, uint32_t size);

/**
 * @brief 客户端调用的接口，GET请求
 * @param eiodp_fd 协议栈句柄
 * @param cmd 请求命令字
 * @param data 输入参数
 * @param len 输入参数长度
 * @param retData 返回数据
 * @param maxretlen retData的容量
 * @return int32_t 返回数据长度  负数为错误
 */
int32_t eiodp_request_GET(
    eIODP_TYPE* eiodp_fd, uint32_t cmd, 
    uint8_t* data, uint32_t len,
    uint8_t* retData, uint32_t maxretlen
);

/************************************************************************
 * @brief 用于解析ringbuf里的数据
 * @return 当前ringbuf的状态 
 *          =0  代表ringbuf无数据不用解析  
 *          >0  代表当前解析状态  
 *          <0  代表遇到了错误
 ************************************************************************/
int32_t eiodp_process(eIODP_TYPE* eiodp_fd);

/************************************************************************
 * @brief 清楚输入输出
 * @param eiodp_fd 协议栈句柄
 ************************************************************************/
void eiodp_clear(eIODP_TYPE* eiodp_fd);





/************************************************************************
 * @brief 按照帧格式整理数据，整理好的数据放在cache里
 * @param cache 用于存放整理好的数据，可以直接发送
 * @param data 内部数据
 * @param len 内部数据长度
 * @param type 包类型
 * @return uint32_t 整理好的数据总长度
 ************************************************************************/
uint32_t eiodp_pktset(uint8_t* cache,uint8_t* data, uint32_t len, uint8_t type);

/************************************************************************
 * @brief 按照协议格式整理数据，整理好的数据放在cache里，此为错误帧格式
 * @param cache 用于存放整理好的数据，可以直接发送
 * @param code 错误代码
 * @return uint32_t 
 ************************************************************************/
uint32_t eiodp_pktset_typeErr(uint8_t* cache, uint8_t code);

/************************************************************************
 * @brief 按照协议格式整理数据，整理好的数据放在cache里，此为GET帧格式
 * @param cache 用于存放整理好的数据，可以直接发送
 * @param cmd 命令字
 * @param data get传入参数数据
 * @param len 数据长度的
 * @return uint32_t 
 ************************************************************************/
uint32_t eiodp_pktset_typeGet(uint8_t* cache,uint32_t cmd, uint8_t* data, uint32_t len);








/************************************************************************
 * @brief 打印出信息，与接收数据
 * @param eiodp_fd 协议栈句柄
 * @param dataidx 接收数据索引
 ************************************************************************/
void eiodp_info(eIODP_TYPE* eiodp_fd,uint32_t dataidx);

/************************************************************
    @brief:
        打印已经注册的服务函数
    @param:
        eiodp_fd:eiodp句柄
    @return:
        <0 - 失败（error code）
         0 - 成功
*************************************************************/
int32_t eiodpShowRegFunc(eIODP_TYPE* eiodp_fd);


//-----------------------------------------------------------------------------------
//                               eiodp ring buffer
//



eIODP_RING* creat_ring(uint32_t size);

void delate_ring(eIODP_RING* p);

int put_ring(eIODP_RING* p,uint8_t* buf,uint32_t size);
int get_ring(eIODP_RING* p,uint8_t* buf,uint32_t size);
uint32_t size_ring(eIODP_RING* p);
void clear_ring(eIODP_RING* p);

#endif