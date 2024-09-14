#include "framelesswindow.h"

#include "framelesswindow_p.h"

using namespace FLWidget2;

/// 设置标题栏，拖动窗口需要标题栏
FramelessWindowParams::FramelessWindowParams(QWidget *widget, int _borderRadius):
    m_framelessWidget(widget)
{
    borderRadius = _borderRadius;
}

void FramelessWindowParams::setTitleBar(QWidget* titlebar)
{
    m_titlebar = titlebar;
}
/// 设置阴影宽度，为0不绘制阴影（默认为0）
void FramelessWindowParams::setShadowWidth(int w)
{
#ifndef Q_OS_LINUX
    m_shadowWidth = w;
    m_framelessWidget->update();
#endif
}

/// 设置是否可移动
void FramelessWindowParams::setMoveEnable(bool b)
{
    m_bMoveEnable = b;
}
/// 设置是否可拖动大小
void FramelessWindowParams::setResizeEnable(bool b)
{
    m_bResizeEnable = b;
}
/// 设置双击标题栏是否最大化
void FramelessWindowParams::setDbMaxInTitleBar(bool b)
{
    m_bDbMaxInTitleBar = b;
}

/// 设置白名单，标题栏上有控件，会影响拖动（目前只在windows上有影响）
void FramelessWindowParams::setTitleWhiteList(const QVector<QWidget*> &ws)
{
    m_titleWhiteList = ws;
}
/// 设置白名单类名称，默认"QLabel"不会影响拖动（目前只在windows上有影响）
void FramelessWindowParams::setTitleWhiteClassName(const QString &objName)
{
    m_titleWhiteClassName = objName;
}

/// 设置样式
void FramelessWindowParams::setBackgroundColor(const QColor &color)
{
    backgroundColor = color;
    m_framelessWidget->update();
}
void FramelessWindowParams::setBorderColor(const QColor &color)
{
    borderColor = color;
    m_framelessWidget->update();
}
//void FramelessWindowParams::setBorderRadius(int radius)
//{
//#ifndef Q_OS_LINUX
//    borderRadius = radius;
//    m_framelessWidget->update();
//#endif
//}
void FramelessWindowParams::setBorderWidth(int width)
{
    borderWidth = width;
    m_framelessWidget->update();
}
void FramelessWindowParams::setBackgroundImg(const QString &img)
{
    backgroundImg = img;
    m_framelessWidget->update();
}

QWidget *FramelessWindowParams::titleBar() const
{
    return m_titlebar;
}
int FramelessWindowParams::shadowWidth() const
{
    return m_shadowWidth;
}

bool FramelessWindowParams::moveEnable() const
{
    return m_bMoveEnable;
}
bool FramelessWindowParams::resizeEnable() const
{
    return m_bResizeEnable;
}
bool FramelessWindowParams::dbMaxInTitleBar() const
{
    return m_bDbMaxInTitleBar;
}

QVector<QWidget*> FramelessWindowParams::titleWhiteList() const
{
    return m_titleWhiteList;
}
QString FramelessWindowParams::titleWhiteClassName() const
{
    return m_titleWhiteClassName;
}

FramelessWidget::FramelessWidget(QWidget *parent, int borderRadius):
    QWidget(parent), FramelessWindowParams(this, borderRadius)
{
    p = new FramelessWindowPrivate(this);
    connect(p, &FramelessWindowPrivate::stateChange, this, &FramelessWidget::sigWindowStateChange);
}

FramelessWidget::~FramelessWidget()
{

}

bool FramelessWidget::nativeEvent(const QByteArray &eventType, void *message, NATIVATEEVENTFUNC_3PARAM_TYPE_1 *result)
{
    if (!p)
        return QWidget::nativeEvent(eventType, message, result);

    return p->nativeEventEx(eventType, message, result,
            [=](const QByteArray &eventType, void *message, NATIVATEEVENTFUNC_3PARAM_TYPE *result) {
        return QWidget::nativeEvent(eventType, message, result);
    });
}

void FramelessWidget::paintEvent(QPaintEvent *event)
{
    if (!p)
        return QWidget::paintEvent(event);

    p->paintEventEx(event, [=](QPaintEvent *event) {
        QWidget::paintEvent(event);
    });
}

bool FramelessWidget::event(QEvent *event)
{
    if (!p)
        return QWidget::event(event);

    return p->eventEx(event, [=](QEvent *event) {
        return QWidget::event(event);
    });
}

bool FramelessWidget::eventFilter(QObject *o, QEvent *e)
{
    if (!p)
        return QWidget::eventFilter(o, e);

    return p->eventFilterEx(o, e, [=](QObject *o, QEvent *e) {
        return QWidget::eventFilter(o, e);
    });
}
