#ifndef FRAMELESSWINDOW_H
#define FRAMELESSWINDOW_H

#include <QWidget>
#include <QDialog>
#include "FramelessWindow_global.h"

namespace FLWidget2 {

/*! *******************************************************
 * @File       : framelesswindow.h
 * @Author     : wmz
 * @Date       : 2024-08-22  22:06
 * @Version    : 0.1.0
 * @Copyright  : Copyright By Wmz, All Rights Reserved
 * @Brief      : 无边框窗口，可自定义标题栏，支持windows（10、11），mac（12，13，14），linux
 * @Description: [详细描述]
 ************** 无边框表现形式：
 ************** 形状：
 **************     windows：
 **************         -- 矩形（默认）
 **************         -- 圆角矩形（需要设置）：效果较差，不建议使用
 **************     linux：
 **************         -- 矩形（只支持矩形）
 **************     mac：
 **************         -- 矩形（默认）
 **************         -- 圆角矩形（需要设置）：示例
 **************             this->setBorderRadius(16);
 **************             this->setWindowFlags(Qt::FramelessWindowHint);
 **************             this->setAttribute(Qt::WA_TranslucentBackground, true);
 ************** 阴影：
 **************     windows：支持
 **************     linux：不支持
 **************     mac：支持（底层框架默认带回阴影效果）
 ************** 缩放、移动：
 **************     windows：支持
 **************     linux：支持
 **************     mac：支持
 **********************************************************/

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#define NATIVATEEVENTFUNC_3PARAM_TYPE_1 long
#else
#define NATIVATEEVENTFUNC_3PARAM_TYPE_1 qintptr
#endif

#define Q_PROPERTY_FRAMELESSWIDGET_PARAM \
    Q_PROPERTY(QString backgroundImg READ getBackgroundImg WRITE setBackgroundImg USER true) \
    Q_PROPERTY(QColor backgroundColor READ getBackgroundColor WRITE setBackgroundColor USER true) \
    Q_PROPERTY(QColor borderColor READ getBorderColor WRITE setBorderColor USER true) \
    Q_PROPERTY(int borderWidth READ getBorderWidth WRITE setBorderWidth USER true) \
//    Q_PROPERTY(int borderRadius READ getBorderRadius WRITE setBorderRadius USER true)

class FramelessWindowPrivate;

class FRAMELESSWINDOW_EXPORT FramelessWindowParams
{
public:
    /// borderRadius：圆角窗口
    ///     --linux：目前不支持
    ///     --window：支持；圆角配合背景阴影（setShadowWidth）有瑕疵，borderRadius建议不要大于8
    ///     --mac：支持
    FramelessWindowParams(QWidget *widget, int borderRadius = 0);

    /// 设置标题栏，拖动窗口需要标题栏
    void setTitleBar(QWidget* titlebar);

    /// 设置阴影宽度，为0不绘制阴影（默认为0）
    /// --linux：不支持（linux开启系统特效自带阴影）
    /// --window：设置窗口为圆角时建议设置
    /// --mac：默认自带阴影
    void setShadowWidth(int w);

    /// 设置是否可移动
    void setMoveEnable(bool b);
    /// 设置是否可拖动大小
    void setResizeEnable(bool b);
    /// 设置双击标题栏是否最大化
    void setDbMaxInTitleBar(bool b);

    /// 设置白名单，标题栏上有控件，会影响拖动（目前只在windows上有影响）
    void setTitleWhiteList(const QVector<QWidget*> &ws);
    /// 设置白名单类名称，默认"QLabel"不会影响拖动（目前只在windows上有影响）
    void setTitleWhiteClassName(const QString &objName);

    /// 设置样式
    void setBackgroundColor(const QColor &color);
    void setBorderColor(const QColor &color);
    void setBorderWidth(int width);
    void setBackgroundImg(const QString &img);
    // void setBorderRadius(int radius);

    QString getBackgroundImg() { return backgroundImg; }
    QColor getBackgroundColor() { return backgroundColor; }
    QColor getBorderColor() { return borderColor; }
    int getBorderRadius() { return borderRadius; }
    int getBorderWidth() { return borderWidth; }

    QWidget *titleBar() const;
    int shadowWidth() const;

    bool moveEnable() const;
    bool resizeEnable() const;
    bool dbMaxInTitleBar() const;

    QVector<QWidget*> titleWhiteList() const;
    QString titleWhiteClassName() const;

private:
    // 标题栏
    QWidget *m_titlebar = nullptr;
    // 需要无边框化的窗口
    QWidget *m_framelessWidget = nullptr;

    // 标题栏上有控件，会影响拖动（目前只在windows上有影响）
    QVector<QWidget*> m_titleWhiteList;
    QString m_titleWhiteClassName = "QLabel";

    // 控制属性
    bool m_bMoveEnable = true;      // 是否可拖动（必须设置标题栏）
    bool m_bResizeEnable = false;   // 是否可拖动大小
    bool m_bDbMaxInTitleBar = false;// 双击标题栏是否最大化

    // 绘制阴影宽度
    int m_shadowWidth = 0;

    // 样式，需要通过qss控制
    int borderRadius = 0;               // border-radius
    int borderWidth = 1;                // 边框宽度
    QColor borderColor = "black";        // 背景颜色
    QColor backgroundColor = "#F0F0F0"; // 背景颜色
    QString backgroundImg;              // 背景图片
};

class FRAMELESSWINDOW_EXPORT FramelessWidget: public QWidget, public FramelessWindowParams
{
    Q_OBJECT

    /// 可通过qss控制样式 qproperty-backgroundColor、qproperty-borderColor等
    Q_PROPERTY_FRAMELESSWIDGET_PARAM

public:
    FramelessWidget(QWidget *parent = nullptr, int borderRadius = 0);
    virtual ~FramelessWidget() override;

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, NATIVATEEVENTFUNC_3PARAM_TYPE_1 *result) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *o, QEvent *e) override;

signals:
    /// 窗口状态变化（待实现）
    void sigWindowStateChange(bool bMax);

private:
    FramelessWindowPrivate *p = nullptr;
};

} // end namespace

#endif // FRAMELESSWINDOW_H
