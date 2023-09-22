#ifndef DEMOSERVER_H
#define DEMOSERVER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class demoServer; }
QT_END_NAMESPACE

class demoServer : public QWidget
{
    Q_OBJECT

public:
    demoServer(QWidget *parent = nullptr);
    ~demoServer();

private:
    Ui::demoServer *ui;
};
#endif // DEMOSERVER_H
