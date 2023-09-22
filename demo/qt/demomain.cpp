#include "demomain.h"
#include "ui_demomain.h"

demoMain::demoMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::demoMain)
{
    ui->setupUi(this);

    //查看type
    QString conStr = ui->cb_connType->currentText();
    if(conStr.compare("TCP")==0){
        ui->cb_serialName->setDisabled(true);
        ui->cb_baudrate->setDisabled(true);
        ui->label_serialName->setDisabled(true);
        ui->label_baud->setDisabled(true);
    }
    else if(conStr.compare("Serial")==0)
    {
        ui->le_ip->setDisabled(true);
        ui->le_port->setDisabled(true);
        ui->label_ip->setDisabled(true);
        ui->label_port->setDisabled(true);
    }

    ui->btn_send->setDisabled(true);
    ui->btn_alltest->setDisabled(true);
    ui->btn_alltest->setDisabled(true);


    updateSerialCB();

    connect(&m_testTimer, &QTimer::timeout, this, &demoMain::on_allTest);
    srand(time(NULL));

    //
    errorcnt = 0;
    okcnt = 0;
}

demoMain::~demoMain()
{
    delete ui;
}


void demoMain::updateSerialCB(void)
{
    QStringList serialportinfo;
    foreach(QSerialPortInfo info,QSerialPortInfo::availablePorts())
    {
        serialportinfo<<info.portName();
    }
    // ui->comboBox->addItems(serialportinfo);
    ui->cb_serialName->addItems(serialportinfo);

}

void demoMain::new_eiodpfd(void)
{
    QString conStr = ui->cb_connType->currentText();


    if(conStr.compare("TCP")==0){
        qiodp_fd = new Qiodp(this, Qiodp::CLIENT, Qiodp::TCP);
    }
    else if(conStr.compare("Serial")==0)
    {
        qiodp_fd = new Qiodp(this, Qiodp::CLIENT, Qiodp::COM);
    }
    connect(qiodp_fd, &Qiodp::uilog, this, [&](QString str){
        ui->tb_log->append(str);
    });
}
void demoMain::delete_eiodpfd(void)
{
    delete qiodp_fd;
    qiodp_fd == NULL;
}



void demoMain::on_btn_conn_clicked()
{
    if(ui->btn_conn->text().compare("断开")==0)
    {
        qiodp_fd->Close();
        delete_eiodpfd();
        ui->btn_conn->setText("连接");
        ui->btn_send->setDisabled(true);
        ui->btn_alltest->setDisabled(true);
        ui->btn_alltest->setDisabled(true);
        return;
    }
    QString conStr = ui->cb_connType->currentText();
    new_eiodpfd();
    if(conStr.compare("TCP")==0){
        QString ipstr = ui->le_ip->text();
        uint16_t port = ui->le_port->text().toUInt();
        qiodp_fd->tcpConn(ipstr,port);
        int outtime = 0;
        ui->btn_conn->setDisabled(true);
        while (1) {

            if(qiodp_fd->isConnect()){
                ui->tb_log->append("连接成功");
                ui->btn_conn->setText("断开");
                ui->btn_send->setDisabled(false);
                ui->btn_alltest->setDisabled(false);
                ui->btn_alltest->setDisabled(false);
                ui->btn_conn->setDisabled(false);
                break;
            }
            qApp->processEvents();
            outtime++;
            if(outtime>1000000){
                ui->tb_log->append("连接超时");
                delete_eiodpfd();
                ui->btn_conn->setDisabled(false);
                return;
            }
        }
    }
    else if(conStr.compare("Serial")==0){
        QString serialName = ui->cb_serialName->currentText();
        quint32 serialBaud = ui->cb_baudrate->currentText().toUInt();
        qint32 ret = qiodp_fd->serialOpen(serialName, serialBaud,QSerialPort::NoParity);
        if(ret==0){
            ui->tb_log->append("连接成功");
            ui->btn_send->setDisabled(false);
            ui->btn_alltest->setDisabled(false);
            ui->btn_alltest->setDisabled(false);
            ui->btn_conn->setText("断开");
        }
        else{
            delete_eiodpfd();
        }
    }
}

void demoMain::on_btn_send_clicked()
{
    uint32_t cmd = ui->le_cmd->text().toInt(nullptr,16);
    QByteArray sbuf,rbuf;
    QString sdataStr = ui->te_sdata->toPlainText();
    QStringList strList = sdataStr.split(" ");

    ui->btn_send->setDisabled(true);

    for(auto e:strList){
        sbuf.push_back(e.toUInt(nullptr,16));
    }
    rbuf = qiodp_fd->requestGET(cmd, sbuf);
    ui->tb_log->append("recv:");
    QString logout;
    for(auto e: rbuf)
    {
        logout+= QString::number(e,16);
        logout += " ";
    }
    ui->tb_log->append(logout);
    ui->btn_send->setDisabled(false);
}

void demoMain::on_btn_clean_clicked()
{
    ui->tb_log->clear();
}

//压力测试
void demoMain::on_btn_alltest_clicked()
{
    QString btnstr = ui->btn_alltest->text();
    if(btnstr.compare("压力测试")==0){
        m_testTimer.start(10);
        ui->btn_alltest->setText("停止测试");
    }
    else{
        m_testTimer.stop();
        ui->btn_alltest->setText("压力测试");
    }

}

void demoMain::on_cb_connType_currentTextChanged(const QString &arg1)
{
    QString conStr = arg1;
    if(conStr.compare("TCP")==0){
        ui->cb_serialName->setDisabled(true);
        ui->cb_baudrate->setDisabled(true);
        ui->label_serialName->setDisabled(true);
        ui->label_baud->setDisabled(true);

        ui->le_ip->setDisabled(false);
        ui->le_port->setDisabled(false);
        ui->label_ip->setDisabled(false);
        ui->label_port->setDisabled(false);
    }
    else if(conStr.compare("Serial")==0)
    {
        ui->le_ip->setDisabled(true);
        ui->le_port->setDisabled(true);
        ui->label_ip->setDisabled(true);
        ui->label_port->setDisabled(true);

        ui->cb_serialName->setDisabled(false);
        ui->cb_baudrate->setDisabled(false);
        ui->label_serialName->setDisabled(false);
        ui->label_baud->setDisabled(false);
    }
}


void demoMain::on_allTest(void)
{
    QByteArray sdata;
    QByteArray rdata;
    int testlen = 512;
    //随机测试长度
    testlen = rand()%600+100;
    //随机数组
    sdata.clear();
    rdata.clear();
    for(int i=0; i<testlen; i++){
        sdata.append((unsigned char)(rand()%254));
    }
    rdata = qiodp_fd->requestGET(0x10,sdata);
    //check
    if(rdata.size()!=testlen){
        QString logstr = "size="+QString::number(rdata.size());
        ui->tb_log->append(logstr);
        qDebug()<<logstr;
        errorcnt++;
    }
    else{
        int i=0;
        for(i=0; i<testlen; i++){
            if(((uint8_t)sdata[i]+1)!=(uint8_t)rdata[i]){
                QString logstr = "i="+QString::number(i)+" sdata="+QString::number(sdata[i]+1)+" rdata="+QString::number(rdata[i]);
                ui->tb_log->append(logstr);
                qDebug()<<logstr;
                errorcnt++;
                break;
            }
        }
        if(i==testlen){
            okcnt++;
            if(okcnt%200==0){
                QString logstr = "okcnt="+QString::number(okcnt)+"  errorcnt="+QString::number(errorcnt);
                ui->tb_log->append(logstr);
                qDebug()<<logstr;
            }
        }
    }
}

