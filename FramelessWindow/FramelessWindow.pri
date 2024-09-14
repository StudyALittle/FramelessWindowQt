LIB_NAME = FramelessWindow

CONFIG(debug, debug|release) {
     unix: LIB_NAME = $$join(LIB_NAME,,,_debug)
     else: LIB_NAME = $$join(LIB_NAME,,,d)
}

LIBS += -L$$PWD/../../FramelessWQt_bin -l$$LIB_NAME

INCLUDEPATH += $$PWD
