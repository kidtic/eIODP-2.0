#ifndef QIODP_H
#define QIODP_H

extern "C"{
    #include "eiodp.h"
};


#include <QThread>
#include <QTcpSocket>
#include <QTcpServer>

class Qiodp : public QThread
{
    Q_OBJECT
public:
    enum Mode_t{
        CLIENT,
        SERVER
    };
    enum ConnType_t{
        TCP,
        COM
    };


public:
    Qiodp(QWidget *parent, Mode_t mode, ConnType_t connType);
    ~Qiodp();

    //返回连接状态
    bool isConnect(void);

    //如果是TCP客户端，调用连接
    qint32 tcpConn(QString ip, quint16 port);

    //get请求
    QArrayData requestGET(quint32 cmd, QArrayData data);


private: //data
    bool connStatus;



private slots:  //槽，模块输入


signals:        //信号，模块输出

};


#endif
