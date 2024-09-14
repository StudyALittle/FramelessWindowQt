#ifndef WIDGET2_H
#define WIDGET2_H

#include <QWidget>
#include "framelesswindow.h"

namespace Ui {
class Widget2;
}

class Widget2 : public FLWidget2::FramelessWidget
{
    Q_OBJECT

public:
    explicit Widget2(QWidget *parent = nullptr);
    ~Widget2();

private slots:
    void on_pushButtonMin_clicked();

    void on_pushButtonMax_clicked();

    void on_pushButtonClose_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::Widget2 *ui;
};

#endif // WIDGET2_H
