#include "demomain.h"
#include "ui_demomain.h"

demoMain::demoMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::demoMain)
{
    ui->setupUi(this);
    qiodp_fd = new Qiodp(this, Qiodp::CLIENT, Qiodp::TCP);
    connect(qiodp_fd, &Qiodp::uilog, this, [&](QString str){
        ui->tb_log->append(str);
    });
}

demoMain::~demoMain()
{
    delete ui;
}


void demoMain::on_btn_conn_clicked()
{
    if(ui->btn_conn->text().compare("断开")==0)
    {
        qiodp_fd->tcpClose();
        ui->btn_conn->setText("连接");
        return;
    }
    qiodp_fd->tcpConn("127.0.0.1",6666);
    int outtime = 0;
    ui->btn_conn->setDisabled(true);
    while (1) {

        if(qiodp_fd->isConnect()){
            ui->tb_log->append("连接成功");
            ui->btn_conn->setText("断开");
            ui->btn_conn->setDisabled(false);
            break;
        }
        qApp->processEvents();
        outtime++;
        if(outtime>1000000){
            ui->tb_log->append("连接超时");
            ui->btn_conn->setDisabled(false);
            return;
        }
    }
}

void demoMain::on_btn_send_clicked()
{
    uint32_t cmd = ui->le_cmd->text().toInt(nullptr,16);
    QByteArray sbuf,rbuf;
    QString sdataStr = ui->te_sdata->toPlainText();
    QStringList strList = sdataStr.split(" ");

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
}
