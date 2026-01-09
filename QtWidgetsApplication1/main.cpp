#include <QApplication>
#include "mainwindow.h"

//Aplikacja do wyostrzania obrazu z wykorzystaniem C++ oraz ASM
//Semestr zimowy 2025/2026
//Wojciech Pêdziwiatr


int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
