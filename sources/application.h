#pragma once
#include "touch_mpe_handler.h"
#include <QApplication>

class MainWindow;

//
class Application : public QApplication, public TouchMpeHandler {
public:
    Application(int &argc, char *argv[]);
    void init();

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
};

//
inline Application *theApp()
{
    return static_cast<Application *>(qApp);
}
