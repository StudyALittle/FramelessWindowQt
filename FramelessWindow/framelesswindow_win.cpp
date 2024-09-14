#include "framelesswindow_p.h"

#include <QApplication>
#include <QStyleOption>
#include <QPainter>
#include <QPainterPath>
#include <QLayout>
#include <QDebug>
#include "framelesswindow.h"

using namespace FLWidget2;

FramelessWindowPrivate::FramelessWindowPrivate(QWidget *widget):
    QObject(widget), m_framelessWidget(widget)
{
    m_params = dynamic_cast<FramelessWindowParams*>(widget);

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

        m_hwnd = (HWND)m_framelessWidget->winId();

        // 设置窗口类型
        SetWindowLongPtr(m_hwnd, GWL_STYLE, GetWindowLong(m_hwnd, GWL_STYLE) | WS_CAPTION);
        SetWindowLongPtr(m_hwnd, GWL_STYLE, GetWindowLong(m_hwnd, GWL_STYLE) | WS_THICKFRAME);
        SetWindowLongPtr(m_hwnd, GWL_STYLE, GetWindowLong(m_hwnd, GWL_STYLE) | WS_MAXIMIZE);

        // 刷新位置状态
        SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        // 留一个距离绘制阴影
        MARGINS margins = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea(m_hwnd, &margins);
    } else {
        m_framelessWidget->setWindowFlags(Qt::FramelessWindowHint);
        m_framelessWidget->setAttribute(Qt::WA_TranslucentBackground);

        m_hwnd = (HWND)m_framelessWidget->winId();
    }
}

FramelessWindowPrivate::~FramelessWindowPrivate()
{
    if (m_mgs)
        delete m_mgs;
    m_mgs = nullptr;
}

bool FramelessWindowPrivate::nativeEventEx(const QByteArray &eventType, void *message, NATIVATEEVENTFUNC_3PARAM_TYPE *result, CBFUNC_NativeEvent selfCBFunc)
{
#if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
    MSG* msg = *reinterpret_cast<MSG**>(message);
#else
    MSG* msg = reinterpret_cast<MSG*>(message);
#endif

    auto composition_enabled = []() -> bool {
        BOOL composition_enabled = FALSE;
        bool success = ::DwmIsCompositionEnabled(&composition_enabled) == S_OK;
        return composition_enabled && success;
    };

    switch (msg->message)
    {
    case WM_NCACTIVATE: {
        if (m_params->getBorderRadius() > 0)
            break;

        if (!composition_enabled()) {
            *result = 1;
            return true;
        }
        break;
    }
    case WM_NCCALCSIZE: {
        if (m_params->getBorderRadius() > 0)
            break;

        if (!maximized(msg->hwnd)) {
            *result = WVR_REDRAW;
            return true;
        }

        // 修正最大化时内容超出屏幕问题
        NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
        auto monitor = ::MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONULL);
        if (!monitor)
            break;

        MONITORINFO monitor_info{};
        monitor_info.cbSize = sizeof(monitor_info);
        if (!::GetMonitorInfoW(monitor, &monitor_info))
            break;

        auto workRect = monitor_info.rcWork;
        params.rgrc[0].left = qMax(params.rgrc[0].left, long(workRect.left));
        params.rgrc[0].top = qMax(params.rgrc[0].top, long(workRect.top));
        params.rgrc[0].right = qMin(params.rgrc[0].right, long(workRect.right));
        params.rgrc[0].bottom = qMin(params.rgrc[0].bottom, long(workRect.bottom));
        *result = WVR_REDRAW;
        return true;
    }
    case WM_NCHITTEST: {
        POINT cursor {
            GET_X_LPARAM(msg->lParam),
            GET_Y_LPARAM(msg->lParam)
        };
        auto x = cursor.x;
        auto y = cursor.y;

        if (m_params->getBorderRadius() > 0 && maximized(msg->hwnd))
            break;

        // 拖动大小
        RECT winrect;
        if (m_params->resizeEnable() && ::GetWindowRect(msg->hwnd, &winrect)) {
            *result = 0;

            const POINT border{
                ::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
                ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
            };

            bool resizeWidth = m_framelessWidget->minimumWidth() != m_framelessWidget->maximumWidth();
            bool resizeHeight = m_framelessWidget->minimumHeight() != m_framelessWidget->maximumHeight();

            winrect.left += m_params->shadowWidth();
            winrect.top += m_params->shadowWidth();
            winrect.right -= m_params->shadowWidth();
            winrect.bottom -= m_params->shadowWidth();

            if(resizeWidth) {
                //left border
                if (x >= winrect.left && x < winrect.left + border.x) {
                    *result = HTLEFT;
                }
                //right border
                if (x < winrect.right && x >= winrect.right - border.x) {
                    *result = HTRIGHT;
                }
            }
            if(resizeHeight) {
                //bottom border
                if (y < winrect.bottom && y >= winrect.bottom - border.y) {
                    *result = HTBOTTOM;
                }
                //top border
                if (y >= winrect.top && y < winrect.top + border.y) {
                    *result = HTTOP;
                }
            }
            if(resizeWidth && resizeHeight) {
                //bottom left corner
                if (x >= winrect.left && x < winrect.left + border.x &&
                    y < winrect.bottom && y >= winrect.bottom - border.y) {
                    *result = HTBOTTOMLEFT;
                }
                //bottom right corner
                if (x < winrect.right && x >= winrect.right - border.x &&
                    y < winrect.bottom && y >= winrect.bottom - border.y) {
                    *result = HTBOTTOMRIGHT;
                }
                //top left corner
                if (x >= winrect.left && x < winrect.left + border.x &&
                    y >= winrect.top && y < winrect.top + border.y) {
                    *result = HTTOPLEFT;
                }
                //top right corner
                if (x < winrect.right && x >= winrect.right - border.x &&
                    y >= winrect.top && y < winrect.top + border.y) {
                    *result = HTTOPRIGHT;
                }
            }

            if (*result != 0)
                return true;
        }

        // 移动窗口
        if (!m_params->titleBar() || !m_params->moveEnable())
            break;

        //support highdpi
        double dpr = m_framelessWidget->devicePixelRatioF();
        QPoint pos = m_params->titleBar()->mapFromGlobal(QPoint(cursor.x/dpr, cursor.y/dpr));

        if (!m_params->titleBar()->rect().contains(pos))
            break;

        QWidget* child = m_params->titleBar()->childAt(pos);
        // 防止鼠标在标题栏其它控件上时拖动窗口无效果
        if (!child || m_params->titleWhiteList().contains(child) ||
                (!m_params->titleWhiteClassName().isEmpty() &&
                    child->inherits(m_params->titleWhiteClassName().toStdString().c_str()))) {
            *result = HTCAPTION;
            upMarginsState(maximized(msg->hwnd));
            return true;
        }
        break;
    }
    case WM_GETMINMAXINFO: {
        upMarginsState(maximized(msg->hwnd));
        break;
    }
    default:
        break;
    }

    return selfCBFunc(eventType, message, result);
}

bool FramelessWindowPrivate::maximized(HWND hwnd)
{
    WINDOWPLACEMENT placement;
    if (!::GetWindowPlacement(hwnd, &placement)) {
        return false;
    }

    return placement.showCmd == SW_MAXIMIZE;
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
    selfCBFunc(event);
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

#if 0
    for(int i = 0; i < sw; i++) {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);

        // 阴影颜色逐渐变浅色
        qreal x = sw - i;
        qreal y = sw -i;
        qreal w = ww - (sw - i) * 2;
        qreal h = wh - (sw - i) * 2;

        if (i < 4)
            continue;

        path.addRoundedRect(x, y, w, h, br, br);
        qreal af = 0.0;
        switch (i) {
            case 0: af = 0.30; break;
            case 1: af = 0.1; break;
            case 2: af = 0.1; break;
            case 3: af = 0.1; break;
            case 4: af = 0.1; break;
            case 5: af = 0.1; break;
            case 6: af = 0.1; break;
            case 7: af = 0.1; break;
        }
        color.setAlphaF(af);
        painter->setPen(color);
        painter->drawPath(path);
    }
#else
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
//        {
//            painter->drawLine(QLineF(
//                                  QPointF(rtF.topLeft().x())),
//                              QLineF());
//        }
    }
#endif
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
