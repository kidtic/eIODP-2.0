#include "iodp_serial_linux.h"
#include "iodp_tcp_linux.h"

//把每个数自+1
void myfunc(eIODP_FUNC_MSG msg)
{
    int i = 0;
    printf("testFunc msg:0x%x\r\n",msg.cmd);
    for(i=0; i<msg.len; i++){
        msg.retdata[i] = msg.data[i]+1;
    }
    *msg.retlen = msg.len;
    return 0;
}

void main(int argc, char **argv)
{
    if (argc < 4) {
		printf("error UseageL %s com [com] [baud]\n error UseageL %s tcp [ip] [port]\n", argv[0]);
        return;
	}
    printf("version 1.2\r\n");
    IODP_TTY_TYPE* myfd;
    IODP_TCP_TYPE* tcpfd;

    if(strcmp(argv[1],"tcp")==0){
        tcpfd = iodptcp_init_server(argv[2], atoi(argv[3]));
        iodptcp_addFunc(tcpfd, 0x10, myfunc);
    }
    else if(strcmp(argv[1],"com")==0)
    {
        myfd = iodptty_init(IODP_MODE_SERVER ,argv[2], atoi(argv[3]),'N');
        iodptty_addFunc(myfd, 0x10, myfunc);
    }

    /* code */
    while(1){
        sleep(1);
    }
    
    iodptty_destory(myfd);
}