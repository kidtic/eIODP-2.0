#include <qiodp.h>



Qiodp::Qiodp(QWidget *parent, Mode_t mode, ConnType_t connType)
{

}
Qiodp::~Qiodp()
{

}

//返回连接状态
bool Qiodp::isConnect(void)
{

}

//如果是TCP客户端，调用连接
qint32 Qiodp::tcpConn(QString ip, quint16 port)
{

}

//get请求
QArrayData Qiodp::requestGET(quint32 cmd, QArrayData data)
{

}
