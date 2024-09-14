#include "framelesswindow_p.h"

#include <QApplication>
#include <QStyleOption>
#include <QPainter>
#include <QPainterPath>
#include <QLayout>
#include <QDebug>
#include <QAbstractNativeEventFilter>
#include <QGuiApplication>
#include <QDateTime>
#include <QCursor>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <xcb/xcbext.h>
// #include <xcb/randr.h>
// #include <xcb/shm.h>
// #include <xcb/sync.h>
// #include <xcb/xfixes.h>
// #include <xcb/render.h>
#include "framelesswindow.h"

// sudo apt-get install libxcb-xinput-dev

#ifndef _NET_WM_MOVERESIZE_SIZE_TOPLEFT
#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT 0
#endif
#ifndef _NET_WM_MOVERESIZE_SIZE_TOP
#define _NET_WM_MOVERESIZE_SIZE_TOP 1
#endif
#ifndef _NET_WM_MOVERESIZE_SIZE_TOPRIGHT
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT 2
#endif
#ifndef _NET_WM_MOVERESIZE_SIZE_RIGHT
#define _NET_WM_MOVERESIZE_SIZE_RIGHT 3
#endif
#ifndef _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#endif
#ifndef _NET_WM_MOVERESIZE_SIZE_BOTTOM
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM 5
#endif
#ifndef _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT 6
#endif
#ifndef _NET_WM_MOVERESIZE_SIZE_LEFT
#define _NET_WM_MOVERESIZE_SIZE_LEFT 7
#endif
#ifndef _NET_WM_MOVERESIZE_MOVE
#define _NET_WM_MOVERESIZE_MOVE 8/* movement only */
#endif
#ifndef _NET_WM_MOVERESIZE_SIZE_KEYBOARD
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD 9/* size via keyboard */
#endif
#ifndef _NET_WM_MOVERESIZE_MOVE_KEYBOARD
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD 10/* move via keyboard */
#endif
#ifndef _NET_WM_MOVERESIZE_CANCEL
#define _NET_WM_MOVERESIZE_CANCEL 11/* cancel operation */
#endif

// #if !defined (XCB_EVENT_RESPONSE_TYPE_MASK)
// #define XCB_EVENT_RESPONSE_TYPE_MASK (0x7f)
// #endif
// #if !defined (XCB_EVENT_RESPONSE_TYPE)
// #define XCB_EVENT_RESPONSE_TYPE(e)   (e->response_type &  XCB_EVENT_RESPONSE_TYPE_MASK)
// #endif
// #if !defined (XCB_EVENT_SENT)
// #define XCB_EVENT_SENT(e)            (e->response_type & ~XCB_EVENT_RESPONSE_TYPE_MASK)
// #endif

// extern xcb_extension_t xcb_input_id;

namespace FLWidget2 {
class X11AbstractNativeEventFilter: public QAbstractNativeEventFilter
{
public:
    enum EState
    {
        E_Not,              //
        E_MOVE,             // 移动
        E_LEFT_RESIZE,      // 左
        E_RIGHT_RESIZE,     // 右
        E_TOP_RESIZE,       // 上
        E_BOTTOM_RESIZE,    // 下
        E_TOPLEFT_RESIZE,   // 左上
        E_TOPRIGHT_RESIZE,  // 右上
        E_BOTTOMLEFT_RESIZE,// 左下
        E_BOTTOMRIGHT_RESIZE,// 右下
    };

    X11AbstractNativeEventFilter(QWidget *widget, FramelessWindowPrivate *p): QAbstractNativeEventFilter()
    {
        m_widget = widget;
        m_fwidgetp = p;

        if (!qEnvironmentVariableIsSet("QT_XCB_NO_XI2"))
            initXInput2(xcbConnect());
    }

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

    /// 发送移动时间
    void SendMove(const QWidget *widget, Qt::MouseButton qbutton, int flag);
    /// 发送按钮释放事件
    void SendButtonRelease(const QWidget *widget, const QPoint &pos, const QPoint &globalPos);

    /// 判断窗口是否是最大化
    static bool isStateMaximized(Display *display, Window window);
    /// 获取x11 Display
    static Display *x11Display();
    static xcb_connection_t *xcbConnect();

    bool enableXInput2(int opCode)
    {
        return m_xi2Enabled && m_xiOpCode == opCode;
    }

    Qt::MouseButton toQtMouseBtn(int btn)
    {
        switch (btn) {
        case Button1: return Qt::LeftButton;
        case Button3: return Qt::RightButton;
        default:
            break;
        }
        return Qt::AllButtons;
    }

protected:
    void initXInput2(xcb_connection_t *xcbConnection);

private:
    enum EResizeBefor {
        EB_Not,
        EB_NeedCalc,
        EB_Enable,
        EB_Diable
    };

    EState m_state = E_Not;
    QWidget *m_widget = nullptr;
    FramelessWindowPrivate *m_fwidgetp = nullptr;

    bool m_bLeftBtnPress = false;
    bool m_bMoveEnable = false;
    bool m_bWillResize = false;
    bool m_bWillResizeEnable = false;
    EResizeBefor m_resizeBefor = EB_Not;

    bool    m_xi2Enabled = false;
    uint8_t m_xiOpCode = 0;
};
} // end namespace

using namespace FLWidget2;

bool X11AbstractNativeEventFilter::isStateMaximized(Display *display, Window window)
{
    static Atom _NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", false);
    static Atom _NET_WM_STATE_MAXIMIZED = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
    Atom actual_type;
    int actual_format;
    unsigned long i, num_items, bytes_after;
    Atom *atoms = nullptr;
    XGetWindowProperty(display, window, _NET_WM_STATE, 0, 1024, False, XA_ATOM,
                       &actual_type, &actual_format, &num_items, &bytes_after, (unsigned char**)&atoms);
    for(i=0; i<num_items; ++i) {
        if(atoms[i]==_NET_WM_STATE_MAXIMIZED)  {
            XFree(atoms);
            return true;
        }
    }
    if(atoms)
        XFree(atoms);
    return false;
}

Display *X11AbstractNativeEventFilter::x11Display()
{
    Display *display = nullptr;
    xcb_connection_t *connection = nullptr;
    bool isPlatformX11 = false;
    if (auto *x11Application = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
        display = x11Application->display();
        connection = x11Application->connection();
        isPlatformX11 = true;
        return display;
    } else {
        return nullptr;
    }
}

xcb_connection_t *X11AbstractNativeEventFilter::xcbConnect()
{
    if (auto *x11Application = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
        return x11Application->connection();
    } else {
        return nullptr;
    }
}

void X11AbstractNativeEventFilter::SendMove(const QWidget *widget, Qt::MouseButton qbutton, int flag)
{
    // const auto screen = QX11Info::appScreen();

    Display *display = x11Display();
    if (!display)
        return;
    // or
    // isPlatformX11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();

    int xbtn = qbutton == Qt::LeftButton ? Button1 :
                   qbutton == Qt::RightButton ? Button3 :
                   AnyButton;

    // 解决电脑屏幕缩放后，拖动窗口问题
    qreal screenRadio = widget->devicePixelRatioF(); // QGuiApplication::primaryScreen()->devicePixelRatio();

    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    const Atom net_move_resize = XInternAtom(display, "_NET_WM_MOVERESIZE", false);
    xev.xclient.type = ClientMessage;
    xev.xclient.message_type = net_move_resize;
    xev.xclient.display = display;
    xev.xclient.window = widget->winId();
    xev.xclient.format = 32;

    const auto global_position = QCursor::pos();
    xev.xclient.data.l[0] = global_position.x() * screenRadio;
    xev.xclient.data.l[1] = global_position.y() * screenRadio;
    xev.xclient.data.l[2] = flag;
    xev.xclient.data.l[3] = xbtn;
    xev.xclient.data.l[4] = 0;
    XUngrabPointer(display, CurrentTime);

    // Get the root window for the current display.
    Window winRoot = XDefaultRootWindow(display);

    XSendEvent(display,
               winRoot,
               false,
               SubstructureRedirectMask | SubstructureNotifyMask,
               &xev);
    XFlush(display);
}

void X11AbstractNativeEventFilter::SendButtonRelease(const QWidget *widget, const QPoint &pos, const QPoint &globalPos)
{
    Display *display = x11Display();
    if (!display)
        return;

    XEvent xevent;
    memset(&xevent, 0, sizeof(XEvent));

    xevent.type = ButtonRelease;
    xevent.xbutton.button = Button1;
    xevent.xbutton.window = widget->effectiveWinId();
    xevent.xbutton.x = pos.x();
    xevent.xbutton.y = pos.y();
    xevent.xbutton.x_root = globalPos.x();
    xevent.xbutton.y_root = globalPos.y();
    xevent.xbutton.display = display;

    XSendEvent(display, widget->effectiveWinId(), False, ButtonReleaseMask, &xevent);
    XFlush(display);
}

void X11AbstractNativeEventFilter::initXInput2(xcb_connection_t *_xcbConnection)
{
    // xcb_extension_t xcb_shm_id, xcb_xfixes_id, xcb_randr_id, xcb_shape_id, xcb_sync_id,
    //     xcb_render_id, xcb_xkb_id, xcb_input_id, xcb_nouse_id;
    // xcb_extension_t *extensions[] = {
    //     &xcb_shm_id, &xcb_xfixes_id, &xcb_randr_id, &xcb_shape_id, &xcb_sync_id,
    //     &xcb_render_id, &xcb_xkb_id, &xcb_input_id, &xcb_nouse_id
    // };
    // for (xcb_extension_t **ext_it = extensions; *ext_it; ++ext_it)
    //     xcb_prefetch_extension_data(_xcbConnection, *ext_it);

    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(_xcbConnection, &xcb_input_id);
    if (!reply || !reply->present) {
        qWarning() << "XInput extension is not present on the X server";
        return;
    }

    // depending on whether bundled xcb is used we may support different XCB protocol versions.
    xcb_input_xi_query_version_reply_t *xinputQuery =
        xcb_input_xi_query_version_reply(_xcbConnection,
            xcb_input_xi_query_version(_xcbConnection, 2, XCB_INPUT_MINOR_VERSION), nullptr);
    if (!xinputQuery || xinputQuery->major_version != 2) {
        qWarning() << "X server does not support XInput 2";
        if (xinputQuery)
            delete xinputQuery;
        return;
    }

    qInfo() << QString("Using XInput version %1.%2")
                      .arg(xinputQuery->major_version)
                      .arg(xinputQuery->minor_version);

    m_xi2Enabled = true;
    m_xiOpCode = reply->major_opcode;
    delete xinputQuery;
}

bool X11AbstractNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result)
    if(!message)
        return false;

    Display *display = x11Display();
    if (!display)
        return false;

    auto funcMouseMove = [=](int gx, int gy)
    {
        if (m_widget->isMaximized() || m_widget->isFullScreen())
            return;

        static int resizeAreaWidth = 8;
        int shadowWidth = 0;

        int x = m_widget->x() + shadowWidth;
        int y = m_widget->y() + shadowWidth;
        int w = m_widget->width() - (shadowWidth * 2);
        int h = m_widget->height() - (shadowWidth * 2);
        QPoint posNew = QCursor::pos();

        bool bWMax = m_widget->isMaximized() || m_widget->isFullScreen();
        if (m_resizeBefor == EB_NeedCalc) {
            if (m_fwidgetp->underMouseTitleWidget())
                m_resizeBefor = EB_Diable;
            // else
            //     m_resizeBefor = EB_Enable;
        }

        bool sameCond = /*m_resizeBefor != EB_Diable &&*/ m_fwidgetp->enableResizeWidget() &&
                        !bWMax && (m_bLeftBtnPress ? m_bWillResizeEnable : true);

        bool bLeft      = sameCond && (qAbs(posNew.x() - x) <= resizeAreaWidth);
        bool bTop       = sameCond && (qAbs(posNew.y() - y) <= resizeAreaWidth);
        bool bRight     = sameCond && (qAbs(x + w - posNew.x()) <= resizeAreaWidth);
        bool bBottom    = sameCond && (qAbs(y + h - posNew.y()) <= resizeAreaWidth);

        bool bTopLeftPos = (m_state == E_Not && bTop && bLeft) || m_state == E_TOPLEFT_RESIZE;
        bool bEnableTopLeftResize = m_bLeftBtnPress && m_state == E_Not;

        bool bTopRightPos = (m_state == E_Not && bTop && bRight) || m_state == E_TOPRIGHT_RESIZE;
        bool bEnableTopRightResize = m_bLeftBtnPress && m_state == E_Not;

        bool bBottomLeftPos = (m_state == E_Not && bBottom && bLeft) || m_state == E_BOTTOMLEFT_RESIZE;
        bool bEnableBottomLeftResize = m_bLeftBtnPress && m_state == E_Not;

        bool bBottomRightPos = (m_state == E_Not && bBottom && bRight) || m_state == E_BOTTOMRIGHT_RESIZE;
        bool bEnableBottomRightResize = m_bLeftBtnPress && m_state == E_Not;

        bool bLeftPos = (m_state == E_Not && bLeft) || m_state == E_LEFT_RESIZE;
        bool bEnableLeftResize = m_bLeftBtnPress && m_state == E_Not;

        bool bRightPos = (m_state == E_Not && bRight) || m_state == E_RIGHT_RESIZE;
        bool bEnableRightResize = m_bLeftBtnPress && m_state == E_Not;

        bool bTopPos = (m_state == E_Not && bTop) || m_state == E_TOP_RESIZE;
        bool bEnableTopResize = m_bLeftBtnPress && m_state == E_Not;

        bool bBottomPos = (m_state == E_Not && bBottom) || m_state == E_BOTTOM_RESIZE;
        bool bEnableBottomResize = m_bLeftBtnPress && m_state == E_Not;

        if (bTopLeftPos) {
            m_bWillResize = true;
            m_widget->setCursor(Qt::SizeFDiagCursor);
            if (bEnableTopLeftResize) {
                m_state = E_TOPLEFT_RESIZE;
                SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_SIZE_TOPLEFT);
            }
        } else if (bTopRightPos) {
            m_bWillResize = true;
            m_widget->setCursor(Qt::SizeBDiagCursor);
            if (bEnableTopRightResize) {
                m_state = E_TOPRIGHT_RESIZE;
                SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_SIZE_TOPRIGHT);
            }
        } else if (bBottomLeftPos) {
            m_bWillResize = true;
            m_widget->setCursor(Qt::SizeBDiagCursor);
            if (bEnableBottomLeftResize) {
                m_state = E_BOTTOMLEFT_RESIZE;
                SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT);
            }
        } else if (bBottomRightPos) {
            m_bWillResize = true;
            m_widget->setCursor(Qt::SizeFDiagCursor);
            if (bEnableBottomRightResize) {
                m_state = E_BOTTOMRIGHT_RESIZE;
                SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT);
            }
        } else if (bLeftPos) {
            m_bWillResize = true;
            m_widget->setCursor(Qt::SizeHorCursor);
            if (bEnableLeftResize) {
                m_state = E_LEFT_RESIZE;
                SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_SIZE_LEFT);
            }
        } else if (bRightPos) {
            m_bWillResize = true;
            m_widget->setCursor(Qt::SizeHorCursor);
            if (bEnableRightResize) {
                m_state = E_RIGHT_RESIZE;
                SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_SIZE_RIGHT);
            }
        } else if (bTopPos) {
            m_bWillResize = true;
            m_widget->setCursor(Qt::SizeVerCursor);
            if (bEnableTopResize) {
                m_state = E_TOP_RESIZE;
                SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_SIZE_TOP);
            }
        } else if (bBottomPos) {
            m_bWillResize = true;
            m_widget->setCursor(Qt::SizeVerCursor);
            if (bEnableBottomResize) {
                m_state = E_BOTTOM_RESIZE;
                SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_SIZE_BOTTOM);
            }
        } else {
            m_bWillResize = false;
            if (m_state != E_MOVE && m_bMoveEnable) {
                m_state = E_MOVE;
                if (m_widget->isMaximized())
                    m_widget->showNormal();
                this->SendMove(m_widget, Qt::LeftButton, _NET_WM_MOVERESIZE_MOVE);
            } else if (m_state == E_Not){
                m_widget->setCursor(Qt::ArrowCursor);
            }
        }
    };
    auto funcMousePress = [=](int btn)
    {
        if (toQtMouseBtn(btn) == Qt::LeftButton) {
            m_bLeftBtnPress = true;
            m_bMoveEnable = m_fwidgetp->enableMoveWidget();
            // qWarning() << "btn press :" << m_bMoveEnable;
            if (FramelessWindowPrivate::underMouse(m_widget) && !m_fwidgetp->underMouseTitleWidget())
                m_resizeBefor = EB_NeedCalc;
            else
                m_resizeBefor = EB_Enable;

            if (m_bWillResize)
                m_bWillResizeEnable = true;
            else
                m_bWillResizeEnable = false;
        }
    };
    auto funcMouseRelease = [=](int btn)
    {
        // qWarning() << "btn release";
        if (toQtMouseBtn(btn) == Qt::LeftButton) {
            m_bLeftBtnPress = false;
            m_bMoveEnable = false;
            m_bWillResize = false;
            m_bWillResizeEnable = false;
            m_resizeBefor = EB_Not;
            if (m_state == E_MOVE) {
                SendButtonRelease(m_widget, QCursor::pos(), QCursor::pos());
            }
            m_state = E_Not;
        }
    };

    if(eventType == "xcb_generic_event_t") {
        xcb_generic_event_t* ev = static_cast<xcb_generic_event_t*>(message);
#if 0
        static int ss = 0;
        qWarning() << "ET: " << (ev->response_type & ~0x80) << " -- " << ++ss;
#endif
        switch (ev->response_type & ~0x80) {
        case XCB_PROPERTY_NOTIFY: {
#if 0
            static Atom _NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", false);
            xcb_property_notify_event_t *pnev = static_cast<xcb_property_notify_event_t*>(message);
            if(pnev->atom == _NET_WM_STATE) {

            }
#endif
            xcb_property_notify_event_t *pnev = static_cast<xcb_property_notify_event_t*>(message);
            break;
        }
        case XCB_MOTION_NOTIFY: { // 鼠标移动事件
            xcb_motion_notify_event_t *mouseev = static_cast<xcb_motion_notify_event_t*>(message);
            funcMouseMove(mouseev->root_x, mouseev->root_y);
            break;
        }
        case XCB_BUTTON_PRESS: { // 鼠标点击事件
            xcb_button_press_event_t *btnev = static_cast<xcb_button_press_event_t*>(message);
            funcMousePress(btnev->detail);
            break;
        }
        case XCB_BUTTON_RELEASE: { // 鼠标释放事件
            xcb_button_release_event_t *btnev = static_cast<xcb_button_release_event_t*>(message);
            funcMouseRelease(btnev->detail);
            break;
        }
        case XCB_GE_GENERIC: {
            xcb_ge_generic_event_t *genev = static_cast<xcb_ge_generic_event_t*>(message);
            if (!enableXInput2(genev->extension))
                break;

            switch (genev->event_type) {
            case XCB_INPUT_MOTION: {   // 鼠标移动事件
                xcb_input_motion_event_t *ximouseev = static_cast<xcb_input_motion_event_t*>(message);
                funcMouseMove(ximouseev->event_x, ximouseev->event_y);

#if 0
        static int ss = 0;
                qWarning() << "ET Move: " << (ev->response_type & ~0x80) << " - " << m_state << " -- " << ++ss;
#endif
                break;
            }
            case XCB_INPUT_BUTTON_PRESS: {    // 鼠标点击事件
                xcb_input_button_press_event_t *xibtnev = static_cast<xcb_input_button_press_event_t*>(message);
                funcMousePress(xibtnev->detail);
                break;
            }
            case XCB_INPUT_BUTTON_RELEASE: {   // 鼠标释放事件
                xcb_input_button_release_event_t *xibtnev = static_cast<xcb_input_button_release_event_t*>(message);
                funcMouseRelease(xibtnev->detail);
                break;
            }
            default:
                break;
            }
            break;
        }
        default:
            break;
        }
    }
    return false;
}

/** ################################################################################################### **/

FramelessWindowPrivate::FramelessWindowPrivate(QWidget *widget):
    QObject(widget), m_framelessWidget(widget)
{
    m_params = dynamic_cast<FramelessWindowParams*>(widget);
    m_x11EventFilter = new X11AbstractNativeEventFilter(widget, this);
    qApp->installNativeEventFilter(m_x11EventFilter);

    static bool s_initNative = false;
    if(!s_initNative)  {
        // 防止窗口 Native化
        QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        QApplication::setAttribute(Qt::AA_NativeWindows, false);
        s_initNative = true;
    }

    // 鼠标移动不受子控件影响
    m_framelessWidget->setAttribute(Qt::WA_Hover);

    if (m_params->getBorderRadius() <= 0) {
        m_framelessWidget->setWindowFlags(widget->windowFlags()
                                          | Qt::FramelessWindowHint
                                          | Qt::WindowSystemMenuHint);
    } else {
        m_framelessWidget->setWindowFlags(Qt::FramelessWindowHint);
        m_framelessWidget->setAttribute(Qt::WA_TranslucentBackground);
    }
}

FramelessWindowPrivate::~FramelessWindowPrivate()
{
    if (m_x11EventFilter)
        delete m_x11EventFilter;
    m_x11EventFilter = nullptr;

    if (m_mgs)
        delete m_mgs;
    m_mgs = nullptr;
}

bool FramelessWindowPrivate::underMouse(QWidget *w, const QPoint &pos)
{
    QPoint cpos;
    if (pos.isNull())
        cpos = QCursor::pos();
    else
        cpos = pos;
    auto tbarPos = w->mapToGlobal(QPoint(0, 0));
    QRect rt(tbarPos.x(), tbarPos.y(), w->width(), w->height());
    return rt.contains(cpos);
}

bool FramelessWindowPrivate::enableMoveWidget()
{
    if (!m_framelessWidget->isActiveWindow())
        return false;

    if (!m_params->moveEnable() || !m_params->titleBar())
        return false;

    return underMouse(m_params->titleBar());
}

bool FramelessWindowPrivate::enableResizeWidget()
{
    if (!m_framelessWidget->isActiveWindow())
        return false;

    if (m_framelessWidget->isFullScreen())
        return false;

    return m_params->resizeEnable();
}

bool FramelessWindowPrivate::underMouseTitleWidget()
{
    if (m_params->titleBar() && FramelessWindowPrivate::underMouse(m_params->titleBar()))
        return true;

    return false;
}

bool FramelessWindowPrivate::nativeEventEx(const QByteArray &eventType, void *message, NATIVATEEVENTFUNC_3PARAM_TYPE *result, CBFUNC_NativeEvent selfCBFunc)
{
    return selfCBFunc(eventType, message, result);
}

void FramelessWindowPrivate::paintEventEx(QPaintEvent *event, CBFUNC_PaintEvent selfCBFunc)
{
    // 无边框、无圆角边框，不处理
    if (m_params->getBorderWidth() <=0 && m_params->getBorderRadius() <= 0) {
        return selfCBFunc(event);
    }

    QPainter painter(m_framelessWidget);
    //    painter.setRenderHint(QPainter::Antialiasing);

    QString bkgImg = m_params->getBackgroundImg();
    int br = m_params->getBorderRadius();
    int bw = m_params->getBorderWidth();
    bool bDrawBorder = true, bMax = false;
    // 最大化、全屏时不绘制边框
    if (m_framelessWidget->isMaximized()
        || m_framelessWidget->isFullScreen()) {
        bMax = true;
        bDrawBorder = false;
        br = 0;
    }
    if (bw <= 0) {
        bDrawBorder = false;
    }

    // 阴影宽度
    int sw = bMax ? 0 : m_params->shadowWidth();
    int space = sw * 2;
    // 绘制边框矩形的大小
    QRect rtBorder(sw, sw, m_framelessWidget->width() - space, m_framelessWidget->height() - space);
    if (bDrawBorder && /*br <= 0 &&*/ sw <= 0) {
        rtBorder.setWidth(m_framelessWidget->width() - 1);
        rtBorder.setHeight(m_framelessWidget->height() - 1);
    }
    // 绘制背景矩形的大小
#if 1
    QRect rtBg = rtBorder;
#else
    QRect rtBg;
    if (bDrawBorder) {
        rtBg = QRect(sw + bw, sw + bw,
                     m_framelessWidget->width() - space - bw * 2,
                     m_framelessWidget->height() - space - bw * 2);
    } else {
        rtBg = rtBorder;
    }
#endif

    painter.save();

    painter.fillRect(m_framelessWidget->rect(), Qt::transparent);

    // 绘制背景图片
    if (!bkgImg.isEmpty()) {
        painter.drawImage(rtBg, QImage(bkgImg));
    }

    // 填充背景颜色
    QPainterPath path;
    path.addRoundedRect(rtBg, br, br);
    painter.fillPath(path, QColor(m_params->getBackgroundColor()));

    // 绘制边框
    if (bDrawBorder) {
        QPen pen = painter.pen();
        pen.setWidth(bw);
        pen.setColor(m_params->getBorderColor());
        painter.setPen(pen);
        painter.drawRoundedRect(rtBorder, br, br);
    }

    // 绘制阴影
    if (sw) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        drawShadow(&painter, sw, br);
    }

    painter.restore();
}

bool FramelessWindowPrivate::eventEx(QEvent *event, CBFUNC_Event selfCBFunc)
{
    switch (event->type())
    {
    case QEvent::Paint: {
        bool bMax = false;
        if (m_framelessWidget->isMaximized()
            || m_framelessWidget->isFullScreen()) {
            bMax = true;
        }
        upMarginsState(bMax);
        break;
    }
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate: {
        m_framelessWidget->update();
        break;
    }
    case QEvent::MouseButtonDblClick: {
        if (m_params->dbMaxInTitleBar() && m_params->titleBar() &&
                FramelessWindowPrivate::underMouse(m_params->titleBar())) {
            if (m_framelessWidget->isFullScreen())
                break;

            if (m_framelessWidget->isMaximized())
                m_framelessWidget->showNormal();
            else
                m_framelessWidget->showMaximized();
        }
        break;
    }
    default:
        break;
    }
    return selfCBFunc(event);
}

/// event事件处理
bool FramelessWindowPrivate::eventFilterEx(QObject *o, QEvent *e, CBFUNC_EventFilter selfCBFunc)
{
    return selfCBFunc(o, e);
}

void FramelessWindowPrivate::upMarginsState(bool bMax)
{
    if (m_bMaxOrFullscreen != bMax) {
        m_framelessWidget->update();
        emit stateChange(m_bMaxOrFullscreen);
    }

    m_bMaxOrFullscreen = bMax;
    if (!bMax) {
        // 非最大化时，需要预留边框和阴影的宽度
        if (!m_bMgBorder) {
            m_bMgBorder = true;
            drawWidgetMgs(m_params->getBorderWidth());
        }
    } else if (m_bMgBorder) {
        // 最大化时恢复margin
        m_bMgBorder = false;
        restoreMgs();
    }
}

void FramelessWindowPrivate::drawShadow(QPainter *painter, int sw, int br)
{
    if (sw <= 0)
        return;

    int ww = m_framelessWidget->width();
    int wh = m_framelessWidget->height();
    QColor color(Qt::darkGray);

    static double bufaa[] = {
        0.4, 0.26, 0.20, 0.16,
        0.12, 0.09, 0.06, 0.02
    };
    static double bufa[] = {
        0.20, 0.16, 0.12, 0.09,
        0.0, 0.0, 0.0, 0.0
    };

    for(int i = 0; i < sw; i++) {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        // 阴影颜色逐渐变浅色
        qreal x = sw - i;
        qreal y = sw -i;
        qreal w = ww - (sw - i) * 2;
        qreal h = wh - (sw - i) * 2;
        //        QRectF rtF(x, y, w, h);
        path.addRoundedRect(x, y, w, h, br, br);
        double na = 0.0;
        if (m_framelessWidget->isActiveWindow()) {
            if (i >= sizeof (bufaa))
                na = 0.0;
            else
                na = bufaa[i];
        } else {
            if (i >= sizeof (bufa))
                na = 0.0;
            else
                na = bufa[i];
            // na = (136 - sqrt(i) * 55) * 0.6;
        }
        color.setAlphaF(na/* < 0 ? 0 : na*/);
        painter->setPen(color);
        painter->drawPath(path);
    }
}

void FramelessWindowPrivate::drawWidgetMgs(int val)
{
    auto *layout = m_framelessWidget->layout();
    if (!layout)
        return;

    int sw = m_params->shadowWidth() ? m_params->shadowWidth() : 0;

    if (!m_mgs) {
        m_mgs = new QMargins;
        *m_mgs = layout->contentsMargins();
    }

    auto mgs = *m_mgs; // layout->contentsMargins();
    if (val >= 0) {
        mgs.setBottom(mgs.bottom() + sw + val);
        mgs.setTop(mgs.top() + sw + val);
        mgs.setLeft(mgs.left() + sw + val);
        mgs.setRight(mgs.right() + sw + val);
    } else {
        mgs.setBottom(mgs.bottom() - sw + val);
        mgs.setTop(mgs.top() - sw + val);
        mgs.setLeft(mgs.left() - sw + val);
        mgs.setRight(mgs.right() - sw + val);
    }
    layout->setContentsMargins(mgs);
}

void FramelessWindowPrivate::restoreMgs()
{
    if (!m_mgs)
        return;

    auto *layout = m_framelessWidget->layout();
    if (!layout)
        return;

    layout->setContentsMargins(*m_mgs);
}
