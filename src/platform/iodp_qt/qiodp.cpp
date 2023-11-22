#include <qiodp.h>
#include <qdebug.h>
#include <qwidget.h>
#include <QApplication>



Qiodp::Qiodp(Mode_t mode, ConnType_t connType)
{
    
    if(mode == CLIENT){
        eiodp_fd = eiodp_init(IODP_MODE_CLIENT,NULL,NULL,(int)this);
    }
    else{
        qDebug()<<"目前QT不支持服务端";
        return;
        //eiodp_fd = eiodp_init(IODP_MODE_SERVER,NULL,NULL);
    }

    m_mode = mode;
    m_connType = connType;
    connStatus = false;

    if(m_connType == TCP)
    {
        tcp_fd = new QTcpSocket(this);
        tcp_fd->setReadBufferSize(4096);
        //连接成功后
        QObject::connect(tcp_fd,&QTcpSocket::connected,this,[&](){
            connStatus = true;

        });

        connect(tcp_fd,&QTcpSocket::readyRead,this,[&](){
            int rlen;
            rlen = tcp_fd->read((char*)recvBuf,4096);
            eiodp_put(eiodp_fd, recvBuf, rlen);
            //qDebug()<<"recv tcp";
        });
    }
    else if(m_connType == COM)
    {
        serial_fd = new QSerialPort();
        connect(serial_fd,&QSerialPort::readyRead,this,[&](){
            int rlen;
            rlen = serial_fd->read((char*)recvBuf,1024);
            eiodp_put(eiodp_fd, recvBuf, rlen);
            qDebug()<<"recv serial";
        });
    }
    else{
        qDebug()<<"error 不支持的连接方式";
        emit uilog("error 不支持的连接方式");
    }
    


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

qint32 Qiodp::tcpLocalIP(QString ip)
{
    if(m_connType != TCP){
        qDebug()<<"error 当前链接模式非TCP";
        emit uilog("error 当前链接模式非TCP");
        return -1;
    }
    tcp_fd->bind(QHostAddress(ip));
    return 0;
}

//如果是COM客户端，调用打开接口
qint32 Qiodp::serialOpen(QString name, quint32 band, QSerialPort::Parity par)
{
    if(m_connType != COM){
        qDebug()<<"error 当前链接模式非COM";
        emit uilog("error 当前链接模式非COM");
        return -1;
    }

    serial_fd->setPortName(name);
    serial_fd->setBaudRate(band);
    serial_fd->setDataBits(QSerialPort::Data8);
    serial_fd->setParity(par);

    if(serial_fd->open(QIODevice::ReadWrite))
    {
        connStatus = true;
        return 0;
    }
    else{
        qDebug()<<"error 无法打开串口";
        emit uilog("error 无法打开串口");
        return -2;
    }
    
}

void Qiodp::Close(void)
{
    if(m_connType == TCP){
        tcp_fd->close();
    }
    else if(m_connType == COM){
        serial_fd->close();
    }

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

    if(m_connType == TCP){
        tcp_fd->write((const char*)eiodp_fd->send_buffer,eiodp_fd->send_len);
    }
    else if(m_connType == COM){
        serial_fd->write((const char*)eiodp_fd->send_buffer,eiodp_fd->send_len);
    }
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
        if(outtime > 3000000){
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


