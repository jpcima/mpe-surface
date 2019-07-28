#pragma once
#include "touch_mpe_handler.h"
#include <QApplication>
#if !defined(Q_OS_ANDROID)
#include <RtMidi.h>
#endif
#include <memory>

class MainWindow;

//
class Application : public QApplication, public TouchMpeHandler {
public:
    Application(int &argc, char *argv[]);
    void init();

    static void initGL();

    QStringList getMidiPorts();
    QString getMidiPortDisplayName(const QString &uri) const;
    void useMidiPort(const QString &uri);
    void sendMidi(const quint8 *data, size_t length, ulong timestamp);

    //
    bool deviceSupportsMidiPorts() const;
    bool deviceSupportsMultiTouch() const;
    bool deviceSupportsAdvancedMultiTouch() const;
    bool deviceSupportsTouchPressure() const;

private:
    MainWindow *mainWindow_ = nullptr;

    //
    bool havePreviousMidiTimestamp_ = false;
    ulong previousMidiTimestamp_ = 0;

    //
#if !defined(Q_OS_ANDROID)
    std::unique_ptr<RtMidiOut> rtmidiOut_;
#endif
};

//
inline Application *theApp()
{
    return static_cast<Application *>(qApp);
}
