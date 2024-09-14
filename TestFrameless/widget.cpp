#include "widget.h"
#include "ui_widget.h"

#include <QScreen>
#ifdef Q_OS_WIN
#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <objidl.h>
#include <gdiplus.h>

#pragma comment (lib,"Dwmapi.lib")
#pragma comment (lib,"user32.lib")
#endif
#include <QDebug>

Widget::Widget(QWidget *parent)
    : FramelessWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    this->setTitleBar(ui->widget);
    this->setMoveEnable(true);
    this->setResizeEnable(true);
    this->setDbMaxInTitleBar(true);
    this->setBorderWidth(1);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButtonClose_clicked()
{
    this->close();
}

void Widget::on_pushButtonMax_clicked()
{
    if (this->isMaximized())
        this->showNormal();
    else {
        // HWND hwnd = (HWND)this->winId();
        // ShowWindow(hwnd, SW_MAXIMIZE);
        this->showMaximized();
    }
}

void Widget::on_pushButtonMin_clicked()
{
    this->showMinimized();
}


void Widget::on_pushButton_2_clicked()
{
//    qDebug() << "SS: " << this->screen()->size();
//    qDebug() << "WS: " << this->rect();
//    qDebug() << "WS2: " << ui->widget->size();
}

