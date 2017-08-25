#include <QCoreApplication>
#include <QtCore>
#include "si570.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Si570 si570;

    if(qApp->arguments().size() > 1)
        si570.set_frequency(QString(qApp->arguments().at(1)).toDouble());

    return 0;
}
