#include <qiodp.h>
#include <qdebug.h>
#include <qwidget.h>
#include <QApplication>



Qiodp::Qiodp(QWidget *parent, Mode_t mode, ConnType_t connType)
{
    
    if(mode == CLIENT){
        eiodp_fd = eiodp_init(IODP_MODE_CLIENT,NULL,NULL);
    }
    else{
        qDebug()<<"目前QT不支持服务端";
        return;
        //eiodp_fd = eiodp_init(IODP_MODE_SERVER,NULL,NULL);
    }

    m_mode = mode;
    m_connType = connType;

    tcp_fd = new QTcpSocket(this);
    connStatus = false;
    //连接成功后
    QObject::connect(tcp_fd,&QTcpSocket::connected,this,[&](){
        connStatus = true;

    });

    connect(tcp_fd,&QTcpSocket::readyRead,this,[&](){
        int rlen;
        rlen = tcp_fd->read((char*)recvBuf,1024);
        eiodp_put(eiodp_fd, recvBuf, rlen);
        qDebug()<<"recv tcp";
    });


}
Qiodp::~Qiodp()
{
    eiodp_destroy(eiodp_fd);
    delete tcp_fd;
}

//返回连接状态
bool Qiodp::isConnect(void)
{
    return connStatus;
}

//如果是TCP客户端，调用连接
qint32 Qiodp::tcpConn(QString ip, quint16 port)
{
    if(m_connType != TCP){
        qDebug()<<"error 当前链接模式非TCP";
        emit uilog("error 当前链接模式非TCP");
        return -1;
    }
    tcp_fd->connectToHost(ip,port);
    return 0;
}

void Qiodp::tcpClose(void)
{
    tcp_fd->close();
    connStatus = false;
}

//get请求
QByteArray Qiodp::requestGET(quint32 cmd, QByteArray data)
{
    QByteArray retdata;
    if(connStatus == false)
    {
        qDebug()<<"未连接，无法发送";
        emit uilog("未连接，无法发送");
        return retdata;
    }

    //clear
    eiodp_clear(eiodp_fd);

    eiodp_fd->send_len = eiodp_pktset_typeGet(eiodp_fd->send_buffer,cmd,(uint8_t*)data.data(),(uint32_t)data.size());

    tcp_fd->write((const char*)eiodp_fd->send_buffer,eiodp_fd->send_len);
    int ret = 0;
    int32_t rlen=0;
    uint32_t outtime = 0;
    while (ret >= 0) {

        ret = eiodp_process(eiodp_fd);
        if(ret == 101){
            //有正确的返回
            break;
        }

        qApp->processEvents();
        outtime++;
        if(outtime > 100000){
            qDebug()<<"request outtime";
            emit uilog("request outtime");
            return retdata;
        }
    }

    //返回数据
    uint32_t retlen = 0;
    eIODP_RETDATA_TYPE* pRetData = (eIODP_RETDATA_TYPE*)eiodp_fd->content_data;
    retlen = pRetData->len;
    retdata.append((const char*)pRetData->data, retlen);
    return  retdata;

}


