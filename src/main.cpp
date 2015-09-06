#include "cookiesmanager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CookiesManager w;
    w.show();
    return a.exec();
}
