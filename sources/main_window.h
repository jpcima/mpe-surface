#pragma once
#include <QMainWindow>
#include <memory>

class TouchMpeHandler;
namespace Ui { class MainWindow; }
class QMenu;
class QSlider;
class QLabel;

//
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(TouchMpeHandler *handler, QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupSettingsMenu(QMenu *menu);

    void execDeviceInfoDialog();
    void execHelpDialog();

    void updateMpeZones();
    void updateBendRange();
    void updateKeyScale();
    void sendConfiguration();

private:
    std::unique_ptr<Ui::MainWindow> ui_;
    QSlider *keyScaleSlider_ = nullptr;
    QSlider *mpeZoneSlider_ = nullptr;
    QLabel *mpeLowerZoneLabel_ = nullptr;
    QLabel *mpeUpperZoneLabel_ = nullptr;
    QSlider *bendRangeSlider_ = nullptr;
    QLabel *bendRangeLabel_ = nullptr;
    TouchMpeHandler *handler_ = nullptr;
    QTimer *sendConfigurationTimer_ = nullptr;
};
