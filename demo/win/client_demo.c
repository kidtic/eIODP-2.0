

#include "iodp_tcp_win.h"
#include "time.h"

#define TEST_DATA_LEN 256

void main(int argc, char **argv)
{
    if (argc < 3) {
		fprintf(stderr, "UseageL %s [ip] [port]\n", argv[0]);
        return -1;
	}
    const char* ipstr = argv[1];
    uint16_t port = atoi(argv[2]);

    IODP_TCP_TYPE* myfd = iodptcp_init_client();
    if(myfd == NULL){
        printf("iodptcp_init_client error\r\n");
        return -1;
    }

    int ret = iodptcp_connect(myfd, ipstr, port);
    if(ret < 0)
    {
        printf("connect error\r\n");
        return -1;
    }

    //数据准备
    srand(time(NULL));
    uint8_t sbuf[TEST_DATA_LEN]={0};
    uint8_t rbuf[TEST_DATA_LEN]={0};
    uint8_t tempbuf[TEST_DATA_LEN]={0};
    int i=0;
    int rlen;
    int errcnt=0,allcnt=0;
    printf("strat request_get\r\n");
    while(1)
    {
        for(i=0;i<TEST_DATA_LEN;i++)
        {
            sbuf[i] = rand()%255;
            tempbuf[i] = sbuf[i]+1;
        }
        memset(rbuf,0,TEST_DATA_LEN);
        allcnt++;
        rlen = iodptcp_request_get(myfd, 0x10, sbuf, TEST_DATA_LEN, rbuf, TEST_DATA_LEN);
        if(rlen < 0){
            errcnt++;
            printf("allcnt:%d errcnt:%d\r\n",allcnt,errcnt);
            continue;
        }

        //check
        for(i=0;i<TEST_DATA_LEN;i++)
        {
            if(rbuf[i]!=tempbuf[i]){
                errcnt++;
                printf("allcnt:%d errcnt:%d\r\n",allcnt,errcnt);
                break;
            }
        }

        //print
        if(allcnt%1000 == 0)
        {
            printf("allcnt:%d errcnt:%d\r\n",allcnt,errcnt);
        }
    }
    

}