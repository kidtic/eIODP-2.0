

#include "iodp_tcp_win.h"


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
    if (argc < 3) {
		fprintf(stderr, "UseageL %s [ip] [port]\n", argv[0]);
	}

    IODP_TCP_TYPE* myfd = iodptcp_init_server(argv[1], atoi(argv[2]));

    iodptcp_addFunc(myfd, 0x10, myfunc);

    /* code */
    while(1);
    
    iodptcp_close(myfd);
}