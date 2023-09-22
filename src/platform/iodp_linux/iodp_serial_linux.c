#include "iodp_serial_linux.h"

void serverThreadTask(void* arg)
{
    
    IODP_TTY_TYPE* pDev =  (IODP_TTY_TYPE*)arg;
    printf("start server fd=%d \r\n",pDev->ttyfd);
    int rlen;
    uint8_t rbuf[1024];
    int ret=1;
    int tempret;
    while (pDev->pth_flag)
    {          
        rlen = read(pDev->ttyfd, rbuf,1024);
        
        if(rlen>0){
            printf("recv %d %d\r\n",rlen,pDev->ttyfd);
            eiodp_put(pDev->eiodp_fd, rbuf,rlen);
            ret = 1;
            while(ret!=0){
                ret = eiodp_process(pDev->eiodp_fd);
                if(ret == -100)
                {
                    printf("close\r\n");
                    break;
                }
            }
        }
        else
        {
            usleep(1000);
        }
    
    }
    
}

//用于向eiodp注册读写函数
int ttyrecv(int fd, char* buf, int size)
{
    //printf("recv %d:",fd);
    int ret = 0;
    ret = read(fd,buf,size);
    if(ret>=0){
        return ret;
    }
    else{
        return 0;
    }
     
}

int ttysend(int fd, char* buf, int size)
{
    int i=0;
    return write(fd,buf,size);
}


int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;
	if  ( tcgetattr( fd,&oldtio)  !=  0) { 
		perror("SetupSerial 1");
		return -1;
	}
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag  |=  CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;
 
	switch( nBits )
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
	}
 
	switch( nEvent )
	{
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'E': 
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;
	case 'N':  
		newtio.c_cflag &= ~PARENB;
		break;
	}
 
	switch( nSpeed )
	{
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
        case 230400:
            cfsetispeed(&newtio, B230400);
			cfsetospeed(&newtio, B230400);
			break;
		case 460800:
			cfsetispeed(&newtio, B460800);
			cfsetospeed(&newtio, B460800);
			break;
        case 921600:
			cfsetispeed(&newtio, B921600);
			cfsetospeed(&newtio, B921600);
			break;
		default:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
	}
	if( nStop == 1 )
		newtio.c_cflag &=  ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |=  CSTOPB;
		newtio.c_cc[VTIME]  = 100;///* 设置超时10 seconds*/
		newtio.c_cc[VMIN] = 0;
		tcflush(fd,TCIFLUSH);
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	
	//	printf("set done!\n\r");
	return 0;
}


/***********************************************************************
 * @brief 串口iodp初始化函数
 * @param iMode 0服务端，1客户端
 * @param devname 串口设备名,一般为"/dev/***"
 * @param baud 波特率
 * @param nEvent 校验位 'N' 'O' 'E'
 * @return IODP_TTY_TYPE* 
 ***********************************************************************/
IODP_TTY_TYPE* iodptty_init(int iMode ,const char* devname, uint32_t baud, char nEvent)
{
    if(iMode<0 || iMode>1){
        printf("param error\r\n");
        return NULL;
    }

    if(devname == NULL){
        printf("param error\r\n");
        return NULL;
    }

    if(nEvent!='N' && nEvent!='O' && nEvent!='E'){
        printf("param error\r\n");
        return NULL;
    }

    //打开串口
    int fd = open(devname, O_RDWR | O_NOCTTY |O_NDELAY);
    if(fd<0){
        printf("open error\r\n");
        return NULL;
    }
    set_opt(fd, baud, 8, nEvent, 1);

    //eiodp
    IODP_TTY_TYPE* pDev = (IODP_TTY_TYPE*)malloc(sizeof(IODP_TTY_TYPE));
    pDev->ttyfd = fd;
    pDev->mode = iMode;
    pDev->pth_flag = 0;
    pDev->eiodp_fd = eiodp_init(iMode, ttyrecv, ttysend, fd);

    if(iMode == IODP_MODE_SERVER){
        printf("pthread_create\r\n");
        pDev->pth_flag = 1;
        pthread_create(&pDev->pth, NULL, serverThreadTask,pDev);
    }
    return pDev;
}


/***********************************************************************
 * @brief 添加服务函数
 * @param pDev 通讯句柄
 * @param cmd 命令字
 * @param callbackFunc 回调函数
 * @return int32_t <0 错误 0成功
 ***********************************************************************/
int32_t iodptty_addFunc(IODP_TTY_TYPE* pDev, uint32_t cmd, int (*callbackFunc)(eIODP_FUNC_MSG msg))
{
    if(pDev == NULL)
    {
        return -1;
    }
    if(pDev->mode != IODP_MODE_SERVER)
    {
        return -2;
    }
    return eiodp_get_register(pDev->eiodp_fd, cmd, callbackFunc);
}


/***********************************************************************
 * @brief 销毁iodptty
 * @param pDev 通讯句柄
 ***********************************************************************/
void iodptty_destory(IODP_TTY_TYPE* pDev)
{
    if(pDev == NULL)
    {
        return -1;
    }
    pDev->pth_flag = 0;
    eiodp_destroy(pDev->eiodp_fd);
    free(pDev);
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
int32_t iodptty_request_get(
    IODP_TTY_TYPE* pDev, uint32_t cmd, 
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