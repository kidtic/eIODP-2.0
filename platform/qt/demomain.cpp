#include "demomain.h"
#include "ui_demomain.h"

demoMain::demoMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::demoMain)
{
    ui->setupUi(this);
}

demoMain::~demoMain()
{
    delete ui;
}

