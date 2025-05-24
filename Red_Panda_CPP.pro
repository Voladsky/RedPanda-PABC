TEMPLATE = subdirs

SUBDIRS += \
    RedPandaIDE \
    consolepauser \
    redpanda_qt_utils \
    qsynedit \
    lua

consolepauser.subdir = tools/consolepauser
redpanda_qt_utils.subdir = libs/redpanda_qt_utils
qsynedit.subdir = libs/qsynedit
lua.subdir = libs/lua

# Add the dependencies so that the RedPandaIDE project can add the depended programs
# into the main app bundle
RedPandaIDE.depends = consolepauser qsynedit lua
qsynedit.depends = redpanda_qt_utils

APP_NAME = RedPandaCPP
include(version.inc)

!isEmpty(APP_VERSION_SUFFIX): {
    APP_VERSION = "$${APP_VERSION}$${APP_VERSION_SUFFIX}"
}

# win32: {
# SUBDIRS += \
#     redpanda-win-git-askpass
# redpanda-win-git-askpass.subdir = tools/redpanda-win-git-askpass
# RedPandaIDE.depends += redpanda-win-git-askpass
# }

# unix: {
# SUBDIRS += \
#     redpanda-git-askpass
#     redpanda-git-askpass.subdir = tools/redpanda-git-askpass
#     RedPandaIDE.depends += redpanda-git-askpass
# }

unix:!macos: {
    isEmpty(PREFIX) {
        PREFIX = /usr
    }
    isEmpty(LIBEXECDIR) {
        LIBEXECDIR = $${PREFIX}/libexec
    }

    QMAKE_SUBSTITUTES += platform/linux/RedPandaIDE.desktop.in

    resources.path = $${PREFIX}/share/$${APP_NAME}
    resources.files += platform/linux/templates
    INSTALLS += resources

    docs.path = $${PREFIX}/share/doc/$${APP_NAME}
    docs.files += README.md
    docs.files += NEWS.md
    docs.files += LICENSE
    INSTALLS += docs

    xdgicons.path = $${PREFIX}/share/icons/hicolor/scalable/apps/
    xdgicons.files += platform/linux/redpandaide.svg
    INSTALLS += xdgicons

    desktop.path = $${PREFIX}/share/applications
    desktop.files += platform/linux/RedPandaIDE.desktop
    INSTALLS += desktop

    mime.path = $${PREFIX}/share/mime/packages
    mime.files = platform/linux/redpandaide.xml
    INSTALLS += mime
}

win32: {
    !isEmpty(PREFIX) {
        target.path = $${PREFIX}

        resources.path = $${PREFIX}

        resources.files += platform/windows/templates
        resources.files += platform/windows/qt.conf
        resources.files += README.md
        resources.files += NEWS.md
        resources.files += LICENSE
        resources.files += RedPandaIDE/images/devcpp.ico

        INSTALLS += resources

        equals(X86_64, "ON") {
            extra_templates.path = $${PREFIX}/templates
            extra_templates.files += platform/windows/templates-win64/*
            INSTALLS += extra_templates
        }
    }
}

# Base PascalABCNETLinux installation
pabcnet.path = $${PREFIX}/share/$${APP_NAME}/PascalABCNETLinux
pabcnet.files = PascalABCNETLinux/*
INSTALLS += pabcnet

# Function to recursively add all subdirectories
defineTest(addPabcnetDir) {
    dir = $$1
    base = PascalABCNETLinux
    varname = $$replace(dir, /, _)
    eval(pabcnet_$${varname}.path = $${PREFIX}/share/$${APP_NAME}/$${base}/$$dir)
    eval(pabcnet_$${varname}.files = $${base}/$$dir/*)
    eval(INSTALLS += pabcnet_$${varname})
    return(true)
}


addPabcnetDir(Highlighting)
addPabcnetDir(IntelliSense)
addPabcnetDir(IntelliSense/Highlighting)
addPabcnetDir(IntelliSense/tr)
addPabcnetDir(IntelliSense/pt-BR)
addPabcnetDir(IntelliSense/ko)
addPabcnetDir(IntelliSense/it)
addPabcnetDir(IntelliSense/Ico)
addPabcnetDir(IntelliSense/fr)
addPabcnetDir(IntelliSense/zh-Hans)
addPabcnetDir(IntelliSense/es)
addPabcnetDir(IntelliSense/x86)
addPabcnetDir(IntelliSense/cs)
addPabcnetDir(IntelliSense/PT4)
addPabcnetDir(IntelliSense/PT4/Lib)
addPabcnetDir(IntelliSense/PT4/Lib/Graph)
addPabcnetDir(IntelliSense/zh-Hant)
addPabcnetDir(IntelliSense/Lib)
addPabcnetDir(IntelliSense/Lib/LibForVB)
addPabcnetDir(IntelliSense/Lib/LibForVB/ABCObjects)
addPabcnetDir(IntelliSense/Lib/LibForVB/GraphABC)
addPabcnetDir(IntelliSense/Lib/en)
addPabcnetDir(IntelliSense/LanguageKits)
addPabcnetDir(IntelliSense/ru)
addPabcnetDir(IntelliSense/pl)
addPabcnetDir(IntelliSense/runtimes)
addPabcnetDir(IntelliSense/runtimes/browser)
addPabcnetDir(IntelliSense/runtimes/browser/lib)
addPabcnetDir(IntelliSense/runtimes/browser/lib/net8.0)
addPabcnetDir(IntelliSense/Lng)
addPabcnetDir(IntelliSense/Lng/Ukr)
addPabcnetDir(IntelliSense/Lng/Rus)
addPabcnetDir(IntelliSense/Lng/zh_CN)
addPabcnetDir(IntelliSense/Lng/Eng)
addPabcnetDir(IntelliSense/ja)
addPabcnetDir(IntelliSense/de)
addPabcnetDir(LanguageKits)
addPabcnetDir(Lib)
addPabcnetDir(LibSource)
addPabcnetDir(Lng)
addPabcnetDir(Lng/Eng)
addPabcnetDir(Lng/Rus)
addPabcnetDir(LSPProxy)

win32: {
INCLUDEPATH += "C:/zmq/include"

LIBS += "C:/zmq/lib/libzmq-v142-mt-4_3_5.lib"
}
