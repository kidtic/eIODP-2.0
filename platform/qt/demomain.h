#ifndef DEMOMAIN_H
#define DEMOMAIN_H

#include <QWidget>
#include "Qiodp/qiodp.h"

QT_BEGIN_NAMESPACE
namespace Ui { class demoMain; }
QT_END_NAMESPACE

class demoMain : public QWidget
{
    Q_OBJECT

public:
    demoMain(QWidget *parent = nullptr);
    ~demoMain();

private slots:
    void on_btn_conn_clicked();

    void on_btn_send_clicked();


    void on_btn_clean_clicked();

    void on_btn_alltest_clicked();

private:
    Ui::demoMain *ui;
    Qiodp* qiodp_fd;
};
#endif // DEMOMAIN_H
