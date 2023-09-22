#ifndef DEMOMAIN_H
#define DEMOMAIN_H

#include <QWidget>
#include "qiodp.h"
#include "QTimer"

QT_BEGIN_NAMESPACE
namespace Ui { class demoMain; }
QT_END_NAMESPACE

class demoMain : public QWidget
{
    Q_OBJECT

public:
    demoMain(QWidget *parent = nullptr);
    ~demoMain();

    void updateSerialCB(void);

    void new_eiodpfd(void);
    void delete_eiodpfd(void);

    Qiodp* qiodp_fd;

private slots:
    void on_btn_conn_clicked();

    void on_btn_send_clicked();


    void on_btn_clean_clicked();

    void on_btn_alltest_clicked();

    void on_cb_connType_currentTextChanged(const QString &arg1);

    void on_allTest(void);

private:
    Ui::demoMain *ui;

    QTimer m_testTimer;

    int errorcnt;
    int okcnt;

};
#endif // DEMOMAIN_H
