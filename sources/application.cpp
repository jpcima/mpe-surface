#include "application.h"
#include "main_window.h"
#include "style/qmidictlActionBarStyle.h"
#include <QStyleFactory>
#include <QTouchDevice>
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras>
#endif
#include <QUrl>

#if !defined(Q_OS_ANDROID)
static const char *rtmidiOutPortName = "output";
static const char *rtmidiVirtualPortName = "Virtual port";
#endif

Application::Application(int &argc, char *argv[])
    : QApplication(argc, argv)
{
    setApplicationName("MPE surface");

    setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents);

#if !defined(Q_OS_ANDROID)
    RtMidiOut *out = new RtMidiOut(RtMidi::UNSPECIFIED, applicationName().toStdString());
    rtmidiOut_.reset(out);
    out->openVirtualPort(rtmidiOutPortName);
#endif
}

void Application::init()
{
#if defined(Q_OS_ANDROID)
    QStyle *androidStyle = QStyleFactory::create("Android");
    setStyle(new qmidictlActionBarStyle(androidStyle));
#endif

    MainWindow *win = mainWindow_ = new MainWindow(this);
#if defined(Q_OS_ANDROID)
    win->showMaximized();
#else
    win->show();
#endif
}

#if !defined(Q_OS_ANDROID)
QStringList Application::getMidiPorts()
{
    QStringList ports;

    std::vector<RtMidi::Api> apis;
    RtMidi::getCompiledApi(apis);

    for (RtMidi::Api api : apis) {
        std::unique_ptr<RtMidiOut> out(new RtMidiOut(api));
        unsigned count = out->getPortCount();

        if (api != RtMidi::WINDOWS_MM) { // supports virtual port
            QUrl url;
            url.setScheme(QString::fromStdString(RtMidi::getApiName(api)));
            url.setPath(QString::fromStdString(rtmidiVirtualPortName));
            ports.append(url.toString());
        }

        for (unsigned i = 0; i < count; ++i) {
            std::string portName = out->getPortName(i);
            if (portName.empty())
                break;

            QUrl url;
            url.setScheme(QString::fromStdString(RtMidi::getApiName(api)));
            url.setPath(QString::fromStdString(portName));
            ports.append(url.toString());
        }
    }

    return ports;
}

QString Application::getMidiPortDisplayName(const QString &uri) const
{
    QUrl url(uri);
    return url.scheme() + ": " + url.path();
}

void Application::useMidiPort(const QString &uri)
{
    QUrl url(uri);

    rtmidiOut_.reset();

    RtMidi::Api api = RtMidi::getCompiledApiByName(url.scheme().toStdString());
    if (api == RtMidi::UNSPECIFIED)
        return;

    std::unique_ptr<RtMidiOut> out(new RtMidiOut(api, applicationName().toStdString()));
    if (!out)
        return;

    std::string portName = url.path().toStdString();

    bool portFound = false;
    if (portName == rtmidiVirtualPortName) {
        portFound = true;
        out->openVirtualPort(rtmidiOutPortName);
    }
    else {
        unsigned portCount = out->getPortCount();

        for (unsigned i = 0; !portFound && i < portCount; ++i) {
            std::string thisPortName = out->getPortName(i);
            if (thisPortName.empty())
                break;

            if (thisPortName == portName) {
                portFound = true;
                out->openPort(i, rtmidiOutPortName);
            }
        }
    }

    if (!portFound)
        return;

    rtmidiOut_ = std::move(out);
}
#else
QStringList Application::getMidiPorts()
{
    QStringList ports;

    QAndroidJniEnvironment env;
    QAndroidJniObject context = QtAndroid::androidContext();

    QAndroidJniObject midiInterface = QAndroidJniObject::callStaticObjectMethod("org/sdf1/jpcima/MainActivity", "getMidiDeviceInterface", "()Lorg/sdf1/jpcima/MidiDeviceInterface;");
    QAndroidJniObject midiPorts = midiInterface.callObjectMethod("listInputPorts", "()[Ljava/lang/String;");

    unsigned count = (unsigned)env->GetArrayLength(midiPorts.object<jobjectArray>());

    for (unsigned i = 0; i < count; ++i) {
        QString uri = QAndroidJniObject(env->GetObjectArrayElement(midiPorts.object<jobjectArray>(), i)).toString();
        ports.append(uri);
    }

    return ports;
}

QString Application::getMidiPortDisplayName(const QString &uri) const
{
    QUrl url(uri);

    QStringList segments;
    for (const QString &segment : url.path().split('/', QString::SkipEmptyParts))
        segments.append(QUrl::fromPercentEncoding(segment.toUtf8()));

    return segments.join(": ");
}

void Application::useMidiPort(const QString &uri)
{
    QAndroidJniObject context = QtAndroid::androidContext();

    QAndroidJniObject midiInterface = QAndroidJniObject::callStaticObjectMethod("org/sdf1/jpcima/MainActivity", "getMidiDeviceInterface", "()Lorg/sdf1/jpcima/MidiDeviceInterface;");
    midiInterface.callMethod<void>("openInputPort", "(Ljava/lang/String;)V", QAndroidJniObject::fromString(uri).object<jobject>());
}
#endif

void Application::sendMidi(const quint8 *data, size_t length, ulong timestamp)
{
    double timedelta = 0;
    if (havePreviousMidiTimestamp_)
        timedelta = 1e-3 * (timestamp - previousMidiTimestamp_);
    else
        havePreviousMidiTimestamp_ = true;
    previousMidiTimestamp_ = timestamp;

    //TODO: MIDI time delta

#ifdef Q_OS_ANDROID
    QAndroidJniEnvironment env;
    QAndroidJniObject midiInterface = QAndroidJniObject::callStaticObjectMethod("org/sdf1/jpcima/MainActivity", "getMidiDeviceInterface", "()Lorg/sdf1/jpcima/MidiDeviceInterface;");
    QAndroidJniObject dataArray = env->NewByteArray(length);
    env->SetByteArrayRegion(dataArray.object<jbyteArray>(), 0, length, (const jbyte *)data);
    midiInterface.callMethod<void>("sendMidiMessage", "([B)V", dataArray.object<jbyteArray>());
#else
    if (RtMidiOut *out = rtmidiOut_.get())
        out->sendMessage(data, length);

    if (0) {
        fprintf(stderr, "Send MIDI:");
        for (size_t i = 0; i < length; ++i)
            fprintf(stderr, " %02X", data[i]);
        fprintf(stderr, "\n");
    }
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
    return false;
#endif
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
    return false;
#endif
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
