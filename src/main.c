#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "eiodp.h"
#include "time.h"

int zrecv(int fd, char* buf, int size)
{

}

int zsend(int fd, uint8_t* buf, int size)
{
    int i=0;
    printf("send %d:",fd);
    for(i=0; i<size;i++)
    {
        if(i%8==0)printf("\r\n");
        printf("%02x ",buf[i]);
    }
    printf("\r\n");
}

int testFunc(eIODP_FUNC_MSG msg)
{
    int i = 0;
    printf("testFunc msg:0x%x\r\n",msg.cmd);
    for(i=0; i<msg.len; i++){
        msg.retdata[i] = msg.data[i]+1;
    }
    *msg.retlen = msg.len;
    return 0;
}



int main(int argc, char** argv)
{
    int i;
    int ret = 0;
    eIODP_TYPE* eiodp_fd = eiodp_init(IODP_MODE_SERVER, zrecv, zsend, 100);

    srand(time(NULL));

    eiodp_get_register(eiodp_fd, 0x200, testFunc);

    int okcnt = 0;
    int errorcnt = 0;
    int allcnt = 0;

/*
    uint8_t rxdata[128] = {0xeb,0x90,0x11,0xaa,0xeb,0x55,0xeb,0x90,0x55,0xa0,0xaa,0xeb,0x90,0x55,0xeb,0x90,0xeb,0xeb,0xeb,0x90,0x55,  
                            0xeb,0x90,0x55,0xaa,  0x2,0x0,0x0,0x1,  0x0,0x0,0x0,16,     //0x27a + 0x5 +0x10 = 0x28f   + 
                            1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
                            0x0,0x0,0x3,0x15};  
*/   
    uint8_t rxdata[1024];

    uint8_t pdata[128]={
        0x0,0x0,0x1,0x00,   //cnt
        0x0,0x0,0x2,0x00,   //cmd
        0x0,0x0,0x0,0x10,   //get
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
    };

    int slen = eiodp_pktset(rxdata,pdata,28,IODP_PTC_TYPE_GET);
    int slen2 = eiodp_pktset(&rxdata[slen-5],pdata,28,IODP_PTC_TYPE_GET);  //可以缺页会导致下一帧也丢失
    int slen3 = eiodp_pktset(&rxdata[slen+slen2],pdata,28,IODP_PTC_TYPE_GET);  

    int scnt = 0;
    int dt = 0;
    while(scnt<1024){
        dt = rand()%10;
        ret = eiodp_put(eiodp_fd, &rxdata[scnt], dt);
        scnt += ret;
        ret = eiodp_process(eiodp_fd);
        if(ret == 100){
            printf("eiodp_process ok\r\n");
        }
        //eiodp_info(eiodp_fd,0);
    }

    //eiodp_info(eiodp_fd,0);
    
    
    return 0;

}
