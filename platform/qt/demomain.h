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

private:
    Ui::demoMain *ui;
};
#endif // DEMOMAIN_H
