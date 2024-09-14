#include "widget2.h"
#include "ui_widget2.h"

Widget2::Widget2(QWidget *parent)
    : FramelessWidget(parent,
#ifdef Q_OS_LINUX
                      0  // linux下不考虑圆角窗口
#else
                      8
#endif
                      )
    , ui(new Ui::Widget2)
{
    ui->setupUi(this);

    this->setTitleBar(ui->widget);
    this->setMoveEnable(true);
    this->setResizeEnable(true);
    this->setDbMaxInTitleBar(true);
    this->setBackgroundColor(Qt::white);

#ifdef Q_OS_WIN
    // window下，圆角窗口，需要绘制背景阴影
    this->setShadowWidth(8);
    // windows下圆角窗口设置边框效果差，不建议设置圆角窗口边框
    this->setBorderWidth(0);
#elif defined(Q_OS_MAC)
    // mac下系统默认带回系统阴影
    // this->setShadowWidth(0);
    // mac下不建议设置圆角窗口边框
    this->setBorderWidth(0);
#elif defined(Q_OS_LINUX)
    // linux下不考虑背景阴影
    // this->setShadowWidth(8);
    // linux下无背景阴影，建议设置窗口边框
    this->setBorderWidth(1);
#endif
}

Widget2::~Widget2()
{
    delete ui;
}

void Widget2::on_pushButtonMin_clicked()
{
    this->showMinimized();
}

void Widget2::on_pushButtonMax_clicked()
{
    if (this->isMaximized())
        this->showNormal();
    else {
        this->showMaximized();
    }
}

void Widget2::on_pushButtonClose_clicked()
{
    this->close();
}

void Widget2::on_pushButton_2_clicked()
{

}

