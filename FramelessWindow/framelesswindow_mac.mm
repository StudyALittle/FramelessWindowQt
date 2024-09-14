#include "framelesswindow.h"

#import <Cocoa/Cocoa.h>

using namespace FLWidget2;

FramelessWindowPrivate::FramelessWindowPrivate(QWidget *widget):
    QObject(widget), m_framelessWidget(widget)
{
    m_params = dynamic_cast<FramelessWindowParams*>(widget);

    NSView *view = (NSView*)m_framelessWidget->winId();
    NSWindow *window = [view window];

    // 设置TitleBar 透明
    window.titlebarAppearsTransparent = YES;
    // 隐藏标题
    window.titleVisibility = NSWindowTitleHidden;
    // 设置按钮融合进页面
    // NSWindowStyleMaskFullSizeContentView 设置TitleBar不额外占用空间
    window.styleMask = NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
                       | NSWindowStyleMaskResizable | NSWindowStyleMaskTitled
                       | NSWindowStyleMaskFullSizeContentView;

    // 设置window透明背景
    // window.opaque = NO;
    // window.backgroundColor = [NSColor clearColor];

    // 修改按钮禁用
    // NSButton *button = [window standardWindowButton:NSWindowZoomButton];
    // [button setEnabled:NO];
    // [button setHidden:YES];

    // 隐藏系统标题栏按钮
    [[window standardWindowButton:NSWindowCloseButton] setHidden:YES];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[window standardWindowButton:NSWindowZoomButton] setHidden:YES];

    this->installEventFilter(this);
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

    return selfCBFunc(event);
}

bool FramelessWindowPrivate::eventEx(QEvent *event, CBFUNC_Event selfCBFunc)
{
    return selfCBFunc(event);
}

/// event事件处理
bool FramelessWindowPrivate::eventFilterEx(QObject *o, QEvent *e, CBFUNC_EventFilter selfCBFunc)
{
    // 隐藏系统标题栏按钮
    if (o == this && e->type() == QEvent::Show) {
        NSView *view = (NSView*)this->winId();
        NSWindow *window = [view window];

        [[window standardWindowButton:NSWindowCloseButton] setHidden:YES];
        [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
        [[window standardWindowButton:NSWindowZoomButton] setHidden:YES];
    }
    return selfCBFunc(o, e);
}
