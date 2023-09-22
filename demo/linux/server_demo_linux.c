#include "iodp_serial_linux.h"

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
		printf("error UseageL %s [com] [baud]\n", argv[0]);
        return;
	}
    printf("version 1.2\r\n");

    IODP_TTY_TYPE* myfd = iodptty_init(IODP_MODE_SERVER ,argv[1], atoi(argv[2]),'N');

    iodptty_addFunc(myfd, 0x10, myfunc);


    /* code */
    while(1){
        sleep(1);
    }
    
    iodptty_destory(myfd);
}