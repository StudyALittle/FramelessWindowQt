#include "widget.h"

#include <QApplication>
#include "widget2.h"

int main(int argc, char *argv[])
{

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
    // 开启高分辨率支持，qt6默认开启
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication a(argc, argv);

    Widget w;
    w.show();

    Widget2 w2;
    w2.show();

    return a.exec();
}
