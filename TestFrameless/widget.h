#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "framelesswindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public FLWidget2::FramelessWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_pushButtonClose_clicked();

    void on_pushButtonMax_clicked();

    void on_pushButtonMin_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
