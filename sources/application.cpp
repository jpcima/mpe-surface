#include "application.h"
#include "main_window.h"
#include <QTouchDevice>
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras>
#endif

Application::Application(int &argc, char *argv[])
    : QApplication(argc, argv)
{
    setApplicationName("MPE surface");

    setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents);
}

void Application::init()
{
    MainWindow *win = mainWindow_ = new MainWindow(this);
    win->show();
}

void Application::sendMidi(const quint8 *data, size_t length, ulong timestamp)
{
    double timedelta = 0;
    if (havePreviousMidiTimestamp_)
        timedelta = 1e-3 * (timestamp - previousMidiTimestamp_);
    else
        havePreviousMidiTimestamp_ = true;
    previousMidiTimestamp_ = timestamp;

#ifdef Q_OS_ANDROID
    QAndroidJniObject context = QtAndroid::androidContext();

    QAndroidJniObject intent("android/content/Intent", "()V");
    QAndroidJniEnvironment env;
    intent.callObjectMethod(
        "setClassName", "(Landroid/content/Context;Ljava/lang/String;)Landroid/content/Intent;",
        context.object<jobject>(),
        QAndroidJniObject::fromString("org.sdf1.jpcima.MidiService").object<jobject>());
    intent.callObjectMethod(
        "putExtra", "(Ljava/lang/String;I)Landroid/content/Intent;",
        QAndroidJniObject::fromString("type").object<jstring>(),
        (jint)1/*SEND_MIDI*/);
    intent.callObjectMethod(
        "putExtra", "(Ljava/lang/String;D)Landroid/content/Intent;",
        QAndroidJniObject::fromString("time").object<jstring>(),
        (jdouble)timedelta);
    jbyteArray dataArray = env->NewByteArray(length);
    env->SetByteArrayRegion(dataArray, 0, length, (const jbyte *)data);
    intent.callObjectMethod(
        "putExtra", "(Ljava/lang/String;[B)Landroid/content/Intent;",
        QAndroidJniObject::fromString("data").object<jstring>(),
        dataArray);
    QAndroidJniObject::callStaticMethod<void>(
        "androidx/core/app/JobIntentService",
        "enqueueWork", "(Landroid/content/Context;Ljava/lang/Class;ILandroid/content/Intent;)V",
        context.object<jobject>(),
        env.findClass("org/sdf1/jpcima/MidiService"),
        (jint)(1000 /*JOB_ID*/),
        intent.object<jobject>());
#else

    #pragma message("TODO: implement MIDI out on desktop")

    fprintf(stderr, "Send MIDI:");
    for (size_t i = 0; i < length; ++i)
        fprintf(stderr, " %02X", data[i]);
    fprintf(stderr, "\n");

#endif
}

bool Application::deviceSupportsMidiPorts() const
{
#ifdef Q_OS_ANDROID
    QAndroidJniObject context = QtAndroid::androidContext();
    return QAndroidJniObject::callStaticMethod<jboolean>(
        "org/sdf1/jpcima/FeatureChecks",
        "deviceSupportsMidiPorts", "(Landroid/content/Context;)Z",
        context.object<jobject>());
#else
    // desktop computer
    return true;
#endif
}

bool Application::deviceSupportsMultiTouch() const
{
#ifdef Q_OS_ANDROID
    QAndroidJniObject context = QtAndroid::androidContext();
    return QAndroidJniObject::callStaticMethod<jboolean>(
        "org/sdf1/jpcima/FeatureChecks",
        "deviceSupportsMultiTouchDistinct", "(Landroid/content/Context;)Z",
        context.object<jobject>());
#else
    for (const QTouchDevice *device : QTouchDevice::devices()) {
        if (device->capabilities() & QTouchDevice::MouseEmulation)
            continue;
        if ((device->capabilities() & QTouchDevice::Position) && device->maximumTouchPoints() >= 2)
            return true;
    }
#endif
    return false;
}

bool Application::deviceSupportsAdvancedMultiTouch() const
{
#ifdef Q_OS_ANDROID
    QAndroidJniObject context = QtAndroid::androidContext();
    return QAndroidJniObject::callStaticMethod<jboolean>(
        "org/sdf1/jpcima/FeatureChecks",
        "deviceSupportsMultiTouchJazzHand", "(Landroid/content/Context;)Z",
        context.object<jobject>());
#else
    for (const QTouchDevice *device : QTouchDevice::devices()) {
        if (device->capabilities() & QTouchDevice::MouseEmulation)
            continue;
        if ((device->capabilities() & QTouchDevice::Position) && device->maximumTouchPoints() >= 5)
            return true;
    }
#endif
    return false;
}

bool Application::deviceSupportsTouchPressure() const
{
// #ifdef Q_OS_ANDROID
//     QAndroidJniObject context = QtAndroid::androidContext();
//     return QAndroidJniObject::callStaticMethod<jboolean>(
//         "org/sdf1/jpcima/FeatureChecks",
//         "deviceSupportsTouchPressure", "(Landroid/content/Context;)Z",
//         context.object<jobject>());
// #else
    for (const QTouchDevice *device : QTouchDevice::devices()) {
        if (device->capabilities() & QTouchDevice::Pressure)
            return true;
    }
// #endif
    return false;
}
