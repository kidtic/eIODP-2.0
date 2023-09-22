#include "demoserver.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    demoServer w;
    w.show();
    return a.exec();
}
