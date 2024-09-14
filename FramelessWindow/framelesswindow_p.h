#ifndef FRAMELESSWINDOW_P_H
#define FRAMELESSWINDOW_P_H

#include <QWidget>
#include <QDialog>
#include <functional>
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

namespace FLWidget2 {

class FramelessWindowParams;
class X11AbstractNativeEventFilter;

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#define NATIVATEEVENTFUNC_3PARAM_TYPE long
#else
#define NATIVATEEVENTFUNC_3PARAM_TYPE qintptr
#endif

typedef std::function<bool(const QByteArray&, void*, NATIVATEEVENTFUNC_3PARAM_TYPE*)> CBFUNC_NativeEvent;
typedef std::function<void(QPaintEvent*)> CBFUNC_PaintEvent;
typedef std::function<bool(QEvent*)> CBFUNC_Event;
typedef std::function<bool(QObject*, QEvent*)> CBFUNC_EventFilter;

class FramelessWindowPrivate: public QObject
{
    Q_OBJECT

public:
    FramelessWindowPrivate(QWidget *widget);
    virtual ~FramelessWindowPrivate();

    /// nativeEvent事件处理
    bool nativeEventEx(const QByteArray &eventType, void *message, NATIVATEEVENTFUNC_3PARAM_TYPE *result, CBFUNC_NativeEvent selfCBFunc);
    /// paintEvent事件处理
    void paintEventEx(QPaintEvent *event, CBFUNC_PaintEvent selfCBFunc);
    /// event事件处理
    bool eventEx(QEvent *event, CBFUNC_Event selfCBFunc);
    /// event事件处理
    bool eventFilterEx(QObject *o, QEvent *e, CBFUNC_EventFilter selfCBFunc);

    bool enableMoveWidget();
    bool enableResizeWidget();
    bool underMouseTitleWidget();

    static bool underMouse(QWidget *w, const QPoint &pos = QPoint());

signals:
    void stateChange(bool bMax);

protected:
#ifdef Q_OS_WIN
    bool maximized(HWND hwnd);
#endif

    void upMarginsState(bool bMax);
    /// 绘制阴影
    void drawShadow(QPainter *painter, int sw, int br);
    void drawWidgetMgs(int val);
    void restoreMgs();

private:
#ifdef Q_OS_WIN
    HWND m_hwnd = nullptr;
#endif
    QWidget *m_framelessWidget = nullptr;
    FramelessWindowParams *m_params;
    X11AbstractNativeEventFilter *m_x11EventFilter = nullptr;

    bool m_bMaxOrFullscreen = false;
    bool m_bMgBorder = false;
    QMargins *m_mgs = nullptr;
};

}// end namespace
#endif // FRAMELESSWINDOW_P_H
