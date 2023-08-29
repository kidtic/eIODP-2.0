#ifndef QIODP_H
#define QIODP_H

extern "C"{
    #include "eiodp.h"
};


#include <QThread>
#include <QTcpSocket>
#include <QTcpServer>
#include <QNetworkInterface>
#include <QHostInfo>

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

    /**
     * @brief 如果是TCP客户端，调用连接（非阻塞，调用后使用isConnect检查）
     * 
     * @param ip 
     * @param port 
     * @return qint32 
     */
    qint32 tcpConn(QString ip, quint16 port);

    /**
     * @brief 调用get请求
     * @param cmd 命令字
     * @param data 输入数据
     * @return QByteArray 返回数据
     */
    QByteArray requestGET(quint32 cmd, QByteArray data);

    /**
     * @brief 断开TCP连接
     * 
     */
    void tcpClose(void);


private: //data
    bool connStatus;    //连接状态
    Mode_t m_mode;
    ConnType_t m_connType;
    QTcpSocket* tcp_fd;
    eIODP_TYPE* eiodp_fd;
    uint8_t recvBuf[2048];



private slots:  //槽，模块输入

signals:        //信号，模块输出
    //用于向UI输出打印信息
    void uilog(QString str);

};


#endif
