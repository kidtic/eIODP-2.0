#include "demoserver.h"
#include "ui_demoserver.h"

demoServer::demoServer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::demoServer)
{
    ui->setupUi(this);
}

demoServer::~demoServer()
{
    delete ui;
}

