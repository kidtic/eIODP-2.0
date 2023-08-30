#include "demomain.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    demoMain w;
    w.show();
    return a.exec();
}
