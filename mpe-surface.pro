#-------------------------------------------------
#
# Project created by QtCreator 2019-07-14T00:52:28
#
#-------------------------------------------------

QT       += core gui
android { QT += androidextras }

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mpe-surface
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11
CONFIG += thread

UI_DIR = forms

SOURCES += \
    sources/application.cpp \
    sources/main.cpp \
    sources/main_window.cpp \
    sources/touch_mpe_handler.cpp \
    sources/touch_piano.cpp

HEADERS += \
    sources/application.h \
    sources/main_window.h \
    sources/touch_mpe_handler.h \
    sources/touch_piano.h \
    sources/touch_piano_listener.h \
    sources/mpe_definitions.h

FORMS += \
    sources/forms/bend_range_chooser.ui \
    sources/forms/key_scale_chooser.ui \
    sources/forms/main_window.ui \
    sources/forms/mpe_zone_chooser.ui

RESOURCES += \
    resources/resources.qrc

INCLUDEPATH += \
    $$PWD/sources

CONFIG += mobility
MOBILITY =


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/src/java/org/sdf1/jpcima/FeatureChecks.java \
    android/src/java/org/sdf1/jpcima/MainActivity.java \
    android/src/java/org/sdf1/jpcima/MidiDeviceInterface.java \
    android/src/java/org/sdf1/jpcima/MidiService.java \
    android/res/values/libs.xml

ANDROID_PACKAGE_SOURCE_DIR = \
    $$PWD/android

include("thirdparty/RtMidi/RtMidi.pri")
