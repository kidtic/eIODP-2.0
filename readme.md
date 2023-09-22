# eIODP 2.0通讯协议 核心代码
通过制定一套协议框架，让用户快速实现主从设备的交互，尤其是让从设备能够轻松构建服务响应（类似http）  

eIODP2.0 主要用于嵌入式与嵌入式或者嵌入式与上位机之间通讯。嵌入式部分会提供纯.c代码（不包含多线程与信号量相关代码）以及实际应用层代码（linux平台的网络通讯与串口通讯）。上位机部分主要提供qt相关代码（封装好网络通讯与串口通讯）。  

eIODP2.0 主要以点对点字符通讯方式为基础，进行上层协议开发。每个eIODP2.0端口要选择作为服务端还是客户端（与1.0不同，只能作为一个端）。例如在作为串口通讯时候，你必须要选择一边为服务端一边为客户端，客户端主动发出请求，服务端响应。而服务端无法主动发出请求。eIODP2.0主要参考http的实现方式来制定。

## 1 使用说明
文件说明     

    demo -              各个平台的客户端与服务端示例代码
      |- linux
      |- qt
      |- win
    include -           核心代码头文件
    src -               核心代码与各个平台库
      |- iodp_qt        qt平台封装库，目前只支持客户端接口
      |- iodp_win       win平台c语言封装库
      |- iodp_linux     linux平台c语言封装库
      eiodp.c           核心代码
      main.c            核心代码测试

### 1.1 核心代码使用说明
协议栈核心代码均由C语言代码编写，与操作系统和平台无关。可供移植到任意嵌入式平台。服务端/从端移植步骤如下：    
1. 自己声明发送/接受数据函数，确保不阻塞：
```c
//fd代表读写文件句柄，和初始化时候传入的fd参数一样
int zrecv(int fd, char* buf, int size)
{
    ...
}
int zsend(int fd, char* buf, int size)
{
    ...
}
```
2. 初始化，指明是服务端模式、提供发送/接受数据函数、提供读写文件句柄
```c
eIODP_TYPE* eiodp_fd = eiodp_init(IODP_MODE_SERVER, zrecv, zsend, fd);
```
3. 注册对应命令字的回调函数
```c
//定义0x200的回调函数，从msg.len中获取入参长度，从msg.data中获取入参数据
//将处理玩的数据放入msg.retdata中，并且由*msg.retlen决定返回参数长度。
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
...
//初始化的时候记得注册上
eiodp_get_register(eiodp_fd, 0x200, testFunc);
```
4. 添加任务循环，将接受的数据存入eiodp输入缓存，然后进行处理
```c
rlen = recv(fd, rbuf,1024, 0);
if(rlen<=0){
    ...
}

eiodp_put(eiodp_fd, rbuf,rlen);
ret = 1;
while(ret!=0){
    ret = eiodp_process(eiodp_fd);
    if(ret == -100) //这里代表发送错误了，zsend返回为负数
    {
        printf("close\r\n");
        break;
    }
}
```

客户端/主端移植步骤如下：

1. 与服务端一样
2. 初始化，指明是服务端模式、提供发送/接受数据函数、提供读写文件句柄
```c
eIODP_TYPE* eiodp_fd = eiodp_init(IODP_MODE_CLIENT, zrecv, zsend, fd);
```
3. 调用请求函数eiodp_request_GET,具体参数说明可以查看头文件
```c
eiodp_request_GET(eiodp_fd,cmd,data,len,rdata,maxretlen);
``` 

**值得注意**的是向eiodp注册的发送接收，必须是阻塞的，可以通过各种方式，比如select/ioctl等先查看是否有数据可以读。无论如何这些都不能是阻塞的。

### 1.2 各平台封装库使用说明
"iodp_tcp_win.c" 为windows平台的tcp接口的eiodp库，可以与其它平台的tcp接口的eiodp库相互通讯。    
"qiodp.cpp" 为Qt平台的eiodp库（目前只支持tcp接口），。。

使用方式可以在demo文件夹里查看示例程序,windows下程序编译运行build.bat即可。


## 2 协议说明
协议目前支持TCP与串口这类字节流型通讯接口。协议格式如下：

|4Byte|3Byte|1Byte|4Byte|len Byte|4Byte|
|:----|:----|:----|:----|:-------|:----|
|head |ver  |type |len  |data    |check|

head为头帧，默认为eb 90 55 aa    
ver为版本号，默认为02 00 00，意思是2.0.0  
type为data类型，目前已分配的有GET(0x01)/RETURN(0x03)/ERROR(0x04)
len为data长度   
data为包数据   
check为校验和   

### 2.1 type=0x01 GET
主要用于客户端向服务端请求数据，data数据格式如下：
|4Byte|4Byte|4Byte|lenp Byte|
|:----|:----|:----|:-------|
|cnt  |cmd  |lenp |datap   |  

cnt用于帧计数（目前没用）   
cmd为命令字     
lenp为参数长度   
datap为参数数据   

### 2.2 type=0x03 RETURN
主要用于服务端回应客户端的get请求，data数据格式如下：
|4Byte|4Byte|4Byte|lend Byte|
|:----|:----|:----|:-------|
|cnt  |allpack|lend |datad   |  

cnt返回的包序号 （暂时没用）   
allpack为返回的总包数 （暂时没用）   
lend为返回数据长度      
datad为返回数据   

### 2.3 type=0x04 ERROR
主要用于服务端回应客户端,当前发生了一些错误，data数据就为一个字节：错误码。详细参考eiodp.h中的package error code。