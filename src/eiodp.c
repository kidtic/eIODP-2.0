/**
 * @file eiodp.c
 * @brief 嵌入式通用io数据协议框架 2.0
 * @author zouzhiqiang (592113107@qq.com)
 * @version 2.0
 * @date 2023-08-26
 * @copyright Copyright (c) 2023 zouzhiqiang
 * @note
 *                              《嵌入式通用io数据协议框架 2.0》
        通过制定一套协议框架，让用户快速实现主从设备的交互，尤其是让从设备能够轻松构建服务响应（类似http）
        eIODP2.0 主要用于嵌入式与嵌入式或者嵌入式与上位机之间通讯。嵌入式部分会提供纯.c代码（不包含多线程与信号量相关代码）
    以及实际应用层代码（linux平台的网络通讯与串口通讯）。上位机部分主要提供qt相关代码（封装好网络通讯与串口通讯）。
        eIODP2.0 主要以点对点字符通讯方式为基础，进行上层协议开发。每个eIODP2.0端口要选择作为服务端还是客户端（与1.0不同，
    只能作为一个端）。例如在作为串口通讯时候，你必须要选择一边为服务端一边为客户端，客户端主动发出请求，服务端响应。而服务端
    无法主动发出请求。eIODP2.0主要参考http的实现方式来制定。
 */
#include "eiodp.h"

uint8_t fhead[4];


static uint32_t _byte2num(uint8_t* data)
{
    if(IODP_ENDIAN==0){     //大端
        return ((uint32_t)data[0]<<24) | ((uint32_t)data[1]<<16) |((uint32_t)data[2]<<8) |((uint32_t)data[3]);
    }
    else{   //小端
        return ((uint32_t)data[3]<<24) | ((uint32_t)data[2]<<16) |((uint32_t)data[1]<<8) |((uint32_t)data[0]);
    }
}

static void _num2byte(uint8_t* buf, uint32_t num)
{
    if(IODP_ENDIAN==0){     //大端
        buf[0] = (num>>24)&0xff;
        buf[1] = (num>>16)&0xff;
        buf[2] = (num>>8)&0xff;
        buf[3] = (num)&0xff;
    }
    else{   //小端
        buf[0] = num&0xff;
        buf[1] = (num>>8)&0xff;
        buf[2] = (num>>16)&0xff;
        buf[3] = (num>>24)&0xff;
    }
}

/***********************************************************************
 * @brief 初始化框架
 * @param mode 0服务器 1客户端
 * @param readfunc 读取函数 服务端可以为空 客户端不能为空
 * @param writefunc 发送函数 不能为空
 * @return eIODP_TYPE* 创建的eIODP_TYPE指针，可以通过这个指针来操作iodp
 ***********************************************************************/
eIODP_TYPE* eiodp_init(unsigned int mode, int (*readfunc)(char*, int),
                int (*writefunc)(char*, int))
{
    //检查入参
    if(mode>1){
        IODP_LOGMSG("error: eiodp_init mode\n");
        return NULL;
    }
    if(mode == IODP_MODE_CLIENT && readfunc ==NULL){
        IODP_LOGMSG("error: mode == IODP_MODE_CLIENT && readfunc ==NULL\n");
        return NULL;
    }
    if(writefunc == NULL)
    {
        IODP_LOGMSG("error: writefunc == NULL\n");
        return NULL;
    }

    eIODP_TYPE* pDev = MOONOS_MALLOC(sizeof(eIODP_TYPE));
    if(pDev == NULL)return NULL;

    pDev->recv_ringbuf=creat_ring(IODP_RECV_MAX_LEN);
    if(pDev->recv_ringbuf == NULL){
        IODP_LOGMSG("recv_ringbuf melloc error\n");
        goto fail_recvrbuf;
    }

    pDev->analyze_data.data = pDev->content_data;
    pDev->mode = mode;
    pDev->iodevRead = readfunc;
    pDev->iodevWrite = writefunc;
    pDev->pFuncHead = NULL;
    pDev->ana_size = 0;

    //头帧为0xeb9055aa
    if(IODP_ENDIAN == 1) //小端 默认
    {
        fhead[0] = 0xaa;
        fhead[1] = 0x55;
        fhead[2] = 0x90;
        fhead[3] = 0xeb;
    }
    else{
        fhead[0] = 0xeb;
        fhead[1] = 0x90;
        fhead[2] = 0x55;
        fhead[3] = 0xaa;
    }
    
    return pDev;

fail_recvrbuf:
    MOONOS_FREE(pDev);
    return NULL;
}


//通过cmd找到服务函数node
static eIODP_FUNC_NODE* findFuncNode(eIODP_FUNC_NODE* pHead,uint16_t cmd)
{
    eIODP_FUNC_NODE* p=pHead;
    while(p){
        if(p->cmd == cmd){
            return p;
        }
        p=p->pNext;
    }
    return NULL;
}
//往服务函数链表中插入新函数节点
static int32_t addFuncNode(eIODP_FUNC_NODE* pHead,eIODP_FUNC_NODE* node)
{
    if(node == NULL){
        return IODP_ERROR_PARAM;
    }
    if(node->pNext != NULL){
        return IODP_ERROR_PARAM;//不是一个单独的节点
    }
    //find
    eIODP_FUNC_NODE* p=pHead;
    while(p){
        if(p->cmd == node->cmd){
            return IODP_ERROR_APINODE_REPEAT;//有重复code
        }
        if(p->pNext==NULL){
            //add
            p->pNext=node;
            return 1;
        }
        p=p->pNext;
    }

    return -4;
}

//check eIODP输入数据是否正确
int32_t eiodp_check(eIODP_TYPE* eiodp_fd)
{
    int i=0;
    if(eiodp_fd->ana_size != (eiodp_fd->analyze_data.len+16)){
        return 0;
    }
    uint32_t sum=0;
    uint32_t check = eiodp_fd->analyze_data.check;
    uint8_t* ptemp = (uint8_t*)&eiodp_fd->analyze_data;
    for(i=0; i<12; i++)
    {
        sum += ptemp[i];
    }
    ptemp = eiodp_fd->content_data;
    for(i=0; i<eiodp_fd->analyze_data.len; i++)
    {
        sum += ptemp[i];
    }

    if(check == sum){
        return 1;
    }
    else{
        return 0;
    }
    
}

/************************************************************************
    @brief:
        注册服务函数
    @param:
        eiodp_fd:eiodp句柄
        cmd：API代码
        callbackFunc:服务函数
    @return:
        <0 - 失败（error code）
         0 - 成功
*************************************************************************/
int32_t eiodp_get_register(eIODP_TYPE* eiodp_fd,uint16_t cmd,
                int (*callbackFunc)(eIODP_FUNC_MSG msg))
{
    if(eiodp_fd == NULL){
        return IODP_ERROR_PARAM;
    }
    if(callbackFunc == NULL){
        return IODP_ERROR_PARAM;
    }
    eIODP_FUNC_NODE* node = MOONOS_MALLOC(sizeof(eIODP_FUNC_NODE));
    if(node == NULL){
        return IODP_ERROR_HEAPOVER;
    }
    node->cmd=cmd;
    node->callbackFunc = callbackFunc;
    node->pNext=NULL;
    if(eiodp_fd->pFuncHead == NULL){
        eiodp_fd->pFuncHead = node;
        return IODP_OK;
    }
    int st = addFuncNode(eiodp_fd->pFuncHead,node);
    if(st == IODP_ERROR_APINODE_REPEAT){
        IODP_LOGMSG("error addFuncNode have repeat code\n");
        return IODP_ERROR_REPEATCODE;
    }
    return IODP_OK;
}

/************************************************************************
    @brief:
        打印已经注册的服务函数
    @param:
        eiodp_fd:eiodp句柄
    @return:
        <0 - 失败（error code）
         0 - 成功
*************************************************************************/
int32_t eiodpShowRegFunc(eIODP_TYPE* eiodp_fd)
{
    if(eiodp_fd == NULL){
        return IODP_ERROR_PARAM;
    }
    if(eiodp_fd->pFuncHead == NULL){
        IODP_LOGMSG("no Register Function\n");
        return IODP_OK;
    }
    eIODP_FUNC_NODE* p = eiodp_fd->pFuncHead;
    while(p){
        IODP_LOG("function code: 0x%04x   function ptr: 0x%x\n",p->cmd,p->callbackFunc);
        p=p->pNext;
    }

    return IODP_OK;
}


/************************************************************************
 * @brief 服务器处理get请求
 * @param eiodp_fd eiodp句柄
 *************************************************************************/
static void eiodp_process_get(eIODP_TYPE* eiodp_fd)
{
    //解析内容包
    eIODP_GETDATA_TYPE* pData = (eIODP_GETDATA_TYPE*)eiodp_fd->content_data;
    pData->cnt = _byte2num((uint8_t*)&pData->cnt);  //后面做丢包统计时候用，目前不用
    pData->cmd = _byte2num((uint8_t*)&pData->cmd);
    pData->len = _byte2num((uint8_t*)&pData->len);
    //debug
    printf("pData->cnt = %d\r\n",pData->cnt);
    printf("pData->cmd = %d\r\n",pData->cmd);
    printf("pData->len = %d\r\n",pData->len);

    //cmd
    eIODP_FUNC_NODE* funcNode = findFuncNode(eiodp_fd->pFuncHead, pData->cmd);
    if(funcNode==NULL){
        //未找到cmd
        eiodp_fd->send_len = eiodp_pktset_typeErr(eiodp_fd->send_buffer,IODP_ERRFREAM_NOCMD);
        if(eiodp_fd->iodevWrite!=NULL){
            eiodp_fd->iodevWrite(eiodp_fd->send_buffer,eiodp_fd->send_len);
        }
        return;
    }
    eIODP_FUNC_MSG msg;
    uint32_t retlen = 0;
    msg.cmd = pData->cmd;
    msg.len = pData->len;
    msg.data = pData->data;
    msg.retlen = &retlen;
    msg.retdata = &eiodp_fd->send_buffer[24];
    funcNode->callbackFunc(msg);
    //检查返回
    if(retlen > (IODP_SEND_BUFFERSIZE-28)){
        IODP_LOG("callbackFunc retlen too big\r\n");
        retlen = (IODP_SEND_BUFFERSIZE-28);
    }

    //整理发送
    memcpy(eiodp_fd->send_buffer, fhead, 4);
    eiodp_fd->send_buffer[4] = 2; 
    eiodp_fd->send_buffer[5]=IODP_PTC_MAJOR; 
    eiodp_fd->send_buffer[6]=IODP_PTC_VERSION;
    eiodp_fd->send_buffer[7] = IODP_PTC_TYPE_RETURN;
    _num2byte(&eiodp_fd->send_buffer[8], retlen+12);   //content size
    //内容
    _num2byte(&eiodp_fd->send_buffer[12], (uint32_t)0);     //包序号
    _num2byte(&eiodp_fd->send_buffer[16], (uint32_t)1);     //总包数
    _num2byte(&eiodp_fd->send_buffer[20], (uint32_t)retlen);
    //check
    uint32_t sum = 0;
    int i = 0;
    for(i=0; i<24+retlen; i++)
    {
        sum += eiodp_fd->send_buffer[i];
    }
    _num2byte(&eiodp_fd->send_buffer[24+retlen],sum);
    
    eiodp_fd->send_len = retlen + 28;
    if(eiodp_fd->iodevWrite!=NULL){
        eiodp_fd->iodevWrite(eiodp_fd->send_buffer,eiodp_fd->send_len);
    }

}

/************************************************************************
 * @brief 用于解析ringbuf里的数据
 * @return 当前ringbuf的状态 
 *          =0  代表ringbuf无数据不用解析  
 *          >0  代表当前解析状态  
 *          <0  代表遇到了错误
 ************************************************************************/
int32_t eiodp_process(eIODP_TYPE* eiodp_fd)
{
    
    //查看rinfbuf
    if(size_ring(eiodp_fd->recv_ringbuf)==0){
        return 0;
    }
    //目前处于头帧识别阶段
    if(eiodp_fd->ana_size<4){
        uint8_t ctemp;
        int32_t ret;
        while (eiodp_fd->ana_size<4)
        {
            ret = get_ring(eiodp_fd->recv_ringbuf, &ctemp, 1);
            if(ret==0)break;
            if(ctemp == fhead[eiodp_fd->ana_size]){
                eiodp_fd->analyze_data.head[eiodp_fd->ana_size] = ctemp;
                eiodp_fd->ana_size++;
            }
            else{
                if(ctemp == fhead[0]){
                    eiodp_fd->analyze_data.head[0] = ctemp;
                    eiodp_fd->ana_size=1;
                }
                else{
                    eiodp_fd->ana_size=0;
                }
            }
        }
        if(eiodp_fd->ana_size != 4){
            return IODP_PROC_STATUS_HEAD;
        }
        else{
            eiodp_fd->anadata_status = 0;
        }
    }

    //目前处于后续帧捕获阶段（ver、type、len）
    if(eiodp_fd->ana_size>=4 && eiodp_fd->ana_size<12)
    {
        int32_t ret;

        ret = get_ring(eiodp_fd->recv_ringbuf, 
                        ((uint8_t*)&eiodp_fd->analyze_data)+eiodp_fd->ana_size, 
                        12-eiodp_fd->ana_size);
        eiodp_fd->ana_size += ret;
        if(eiodp_fd->ana_size!=12){
            return IODP_PROC_STATUS_TYPE;
        }
        else{
            //解析len
            eiodp_fd->analyze_data.len = _byte2num((uint8_t*)&eiodp_fd->analyze_data.len);

        }
    }

    eIODP_PACK_TYPE* pdata = &eiodp_fd->analyze_data;
    
    //首先判断len是否符合要求
    if(pdata->len>(IODP_RETDATA_BUFFERSIZE+12))
    {
        eiodp_fd->ana_size = 0;
        return IODP_PROC_ERROR_LEN;
    }
    //判断版本是否符合要求
    if(pdata->ver[0]!=2 || pdata->ver[1]!=IODP_PTC_MAJOR || pdata->ver[2]>IODP_PTC_VERSION)
    {
        eiodp_fd->ana_size = 0;
        return IODP_PROC_ERROR_VER;
    }
    //判断类型是否识别
    if(pdata->type>0X05){
        eiodp_fd->ana_size = 0;
        return IODP_PROC_ERROR_TYPE;
    }

    
    //前半部分已经帮忙解析好了len数据长度，接下来接收数据。
    if(eiodp_fd->ana_size>=12 && eiodp_fd->ana_size < (pdata->len+12)){
        int32_t ret;
        uint32_t idata = eiodp_fd->ana_size-12;     //content_data内容size
        ret = get_ring(eiodp_fd->recv_ringbuf, 
                        eiodp_fd->content_data+idata, 
                        pdata->len - idata);
        eiodp_fd->ana_size += ret;
        if(eiodp_fd->ana_size != 12+pdata->len)
        {
            return IODP_PROC_STATUS_DATA;
        }
        
    }

    //接收check数据
    if(eiodp_fd->ana_size>=(pdata->len+12) && eiodp_fd->ana_size<(pdata->len+16))
    {
        int32_t ret;
        uint32_t idata = eiodp_fd->ana_size-12-pdata->len;     //check内容
        ret = get_ring(eiodp_fd->recv_ringbuf, 
                        ((uint8_t*)&eiodp_fd->analyze_data.check) + idata, 
                        4 - idata);
        eiodp_fd->ana_size += ret;
        if(eiodp_fd->ana_size != 16+pdata->len)
        {
            return IODP_PROC_STATUS_CHECK;
        }
    }

    //check数据
    eiodp_fd->analyze_data.check = _byte2num((uint8_t*)&eiodp_fd->analyze_data.check);
    if(!eiodp_check(eiodp_fd)){
        eiodp_fd->ana_size = 0;
        return IODP_PROC_ERROR_CHECK;
    }


    //后面会根据type和mode执行对应函数
    if(eiodp_fd->mode == IODP_MODE_SERVER)
    {
        if(pdata->type == IODP_PTC_TYPE_GET){
            //get解析函数
            eiodp_process_get(eiodp_fd);
        }
        else if(pdata->type == IODP_PTC_TYPE_POST){
            //post解析函数
        }
        
    }
    else if(eiodp_fd->mode == IODP_MODE_CLIENT)
    {
        if(pdata->type == IODP_PTC_TYPE_RETURN){
            eiodp_fd->anadata_status = 1;
            eiodp_fd->ana_size = 0;
            return 101;     //可以直接调用函数取出retdata
        }
        else if(pdata->type == IODP_PTC_TYPE_ERROR){
            if(pdata->len!=1){
                eiodp_fd->ana_size = 0;
                return IODP_PROC_ERROR_CONTENT;
            }
            IODP_LOG("remote server back error: 0x%x\r\n",eiodp_fd->content_data[0]);
            return 102;     
        }
    }



    eiodp_fd->anadata_status = 1;
    eiodp_fd->ana_size = 0;
    return 100;
}

/************************************************************************
 * @brief 往协议栈输入数据
 * @param eiodp_fd 协议栈句柄
 * @param data 数据
 * @param size 数据长度
 * @return int32_t 实际插入数据
 ************************************************************************/
int32_t eiodp_put(eIODP_TYPE* eiodp_fd, uint8_t* data, uint32_t size)
{
    return put_ring(eiodp_fd->recv_ringbuf, data, size);
}

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
)
{
    uint8_t rbuf[128];
    eiodp_fd->send_len = eiodp_pktset_typeGet(eiodp_fd->send_buffer,cmd,data,len);
    if(eiodp_fd->iodevWrite == NULL){
        IODP_LOG("iodevWrite = NULL\r\n");
        return -1;
    }
    if(eiodp_fd->iodevRead == NULL){
        IODP_LOG("iodevRead = NULL\r\n");
        return -2;
    }
    //clear
    eiodp_clear(eiodp_fd);

    eiodp_fd->iodevWrite(eiodp_fd->send_buffer,eiodp_fd->send_len);

    int ret = 0;
    int32_t rlen=0;
    uint32_t outtime = 0;
    while (ret >= 0)
    {
        rlen = eiodp_fd->iodevRead(rbuf,128);   //必须无阻塞
        if(rlen<0){
            IODP_LOG("iodevRead error ret = %d\r\n",rlen);
            return -3;
        }
        eiodp_put(eiodp_fd, rbuf, rlen);
        ret = eiodp_process(eiodp_fd);
        if(ret == 101){
            //有正确的返回
            break;
        }
        if(outtime > 100000000){
            IODP_LOG("request outtime\r\n");
            return -4;
        }
    }

    //返回数据
    uint32_t retlen = 0;
    eIODP_RETDATA_TYPE* pRetData = (eIODP_RETDATA_TYPE*)eiodp_fd->content_data;
    if(maxretlen < pRetData->len)
    {
        retlen = maxretlen;
    }
    else{
        retlen = pRetData->len;
    }
    memcpy(retData, pRetData->data, retlen);
    return retlen;
    
}

/************************************************************************
 * @brief 清楚输入输出
 * @param eiodp_fd 协议栈句柄
 ************************************************************************/
void eiodp_clear(eIODP_TYPE* eiodp_fd)
{
    clear_ring(eiodp_fd->recv_ringbuf);
    eiodp_fd->ana_size = 0;
    eiodp_fd->anadata_status = 0;
    eiodp_fd->send_len=0;
    
}


/************************************************************************
 * @brief 打印出信息，与接收数据
 * @param eiodp_fd 协议栈句柄
 * @param dataidx 接收数据索引
 ************************************************************************/
void eiodp_info(eIODP_TYPE* eiodp_fd,uint32_t dataidx)
{
    int i=0;
    IODP_LOG("analyzeRecv size:%d\r\n", eiodp_fd->ana_size);
    IODP_LOG("analyzeData status:%d\r\n",eiodp_fd->anadata_status);
    IODP_LOG("frame len:%d\r\n",eiodp_fd->analyze_data.len);
    IODP_LOG("frame type:0x%x\r\n",eiodp_fd->analyze_data.type);
    IODP_LOG("frame ver:%d.%d.%d\r\n",eiodp_fd->analyze_data.ver[0],eiodp_fd->analyze_data.ver[1],eiodp_fd->analyze_data.ver[2]);
    

    IODP_LOG("frame data:");
    for(i=0; i<64;i++)
    {
        if(i%8==0)IODP_LOG("\r\n");
        IODP_LOG("%02x ",eiodp_fd->content_data[i+dataidx]);
    }
    IODP_LOG("\r\n");
}


/************************************************************************
 * @brief 按照帧格式整理数据，整理好的数据放在cache里
 * @param cache 用于存放整理好的数据，可以直接发送
 * @param data 内部数据
 * @param len 内部数据长度
 * @param type 包类型
 * @return uint32_t 整理好的数据总长度
 ************************************************************************/
uint32_t eiodp_pktset(uint8_t* cache,uint8_t* data, uint32_t len, uint8_t type)
{
    int i=0;
    memcpy(cache,fhead,4);
    cache[4] = 2; cache[5]=IODP_PTC_MAJOR; cache[6]=IODP_PTC_VERSION;
    cache[7] = type;
    _num2byte(&cache[8], len);
    memcpy(&cache[12], data,len);
    //check
    uint32_t sum = 0;
    for(i=0; i<12+len; i++)
    {
        sum += cache[i];
    }
    _num2byte(&cache[12+len],sum);
    return len+16;
}


/************************************************************************
 * @brief 按照协议格式整理数据，整理好的数据放在cache里，此为错误帧格式
 * @param cache 用于存放整理好的数据，可以直接发送
 * @param code 错误代码
 * @return uint32_t 
 ************************************************************************/
uint32_t eiodp_pktset_typeErr(uint8_t* cache, uint8_t code)
{
    return eiodp_pktset(cache, &code, 1, IODP_PTC_TYPE_ERROR);
}

/************************************************************************
 * @brief 按照协议格式整理数据，整理好的数据放在cache里，此为GET帧格式
 * @param cache 用于存放整理好的数据，可以直接发送
 * @param cmd 命令字
 * @param data get传入参数数据
 * @param len 数据长度的
 * @return uint32_t 
 ************************************************************************/
uint32_t eiodp_pktset_typeGet(uint8_t* cache,uint32_t cmd, uint8_t* data, uint32_t len)
{
    memcpy(cache, fhead, 4);
    cache[4] = 2; 
    cache[5] = IODP_PTC_MAJOR; 
    cache[6] = IODP_PTC_VERSION;
    cache[7] = IODP_PTC_TYPE_RETURN;
    _num2byte(&cache[8], len+12);   //content size
    //内容
    _num2byte(&cache[12], (uint32_t)0);     //CNT  暂时不用
    _num2byte(&cache[16], (uint32_t)cmd);     //CMD
    _num2byte(&cache[20], (uint32_t)len);
    memcpy(&cache[24], data, len);
    //check
    uint32_t sum = 0;
    int i = 0;
    for(i=0; i<24+len; i++)
    {
        sum += cache[i];
    }
    _num2byte(&cache[24+len],sum);
    return 28+len;
}




//---------------------------------------------------------------------------------
//                             eiodp ring buffer

eIODP_RING* creat_ring(uint32_t size)
{
    eIODP_RING* pRet=(eIODP_RING*)MOONOS_MALLOC(sizeof (eIODP_RING));
    pRet->buf=(uint8_t*)MOONOS_MALLOC(size);
    if(pRet->buf!=NULL)
    {
        pRet->bufSize=size;
        pRet->pIn=0;
        pRet->pOut=0;

    }
    else {
        MOONOS_FREE(pRet);
        pRet=NULL;
    }

    return pRet;
}

void delate_ring(eIODP_RING* p)
{
    MOONOS_FREE(p->buf);
    MOONOS_FREE(p);
}

uint32_t size_ring(eIODP_RING* p)
{
    if(p->pIn >= p->pOut){
        return p->pIn-p->pOut;
    }
    else{
        return p->bufSize-(p->pOut-p->pIn);
    }
}

int put_ring(eIODP_RING* p,uint8_t* buf,uint32_t size)
{
		int i;
    //首先要判断是否会写满
    if(size+size_ring(p) >= p->bufSize){
        return 0;
    }

    
    for(i=0;i<size;i++){
        p->buf[p->pIn]=buf[i];

        if((p->pIn+1) >= p->bufSize){
            p->pIn=0;
        }
        else {
            p->pIn++;
        }
    }
    return i;

}

int get_ring(eIODP_RING* p,uint8_t* buf,uint32_t size)
{
	int i;
    if(p->pIn == p->pOut) return 0;

    
    for(i=0;i<size;i++){
        buf[i]=p->buf[p->pOut];

        if((p->pOut+1) >= p->bufSize){
            p->pOut=0;
        }
        else {
            p->pOut++;
        }


        if(p->pIn == p->pOut){
            return i+1;
        }
    }
    return i;
}

void clear_ring(eIODP_RING* p)
{
    p->pIn = 0;
    p->pOut = 0;
}
