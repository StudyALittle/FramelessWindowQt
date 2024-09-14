#include "widget.h"
#include "ui_widget.h"

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
    else
        this->showMaximized();
}

void Widget::on_pushButtonMin_clicked()
{
    this->showMinimized();
}

void Widget::on_pushButton_2_clicked()
{

}

