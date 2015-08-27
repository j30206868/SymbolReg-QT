#include "mainwindow.h"
#include <QApplication>

#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Window window;
    window.show();

    return a.exec();
}
