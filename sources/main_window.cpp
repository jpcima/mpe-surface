#include "main_window.h"
#include "touch_mpe_handler.h"
#include "application.h"
#include "mpe_definitions.h"
#include "forms/ui_main_window.h"
#include "forms/ui_key_scale_chooser.h"
#include "forms/ui_mpe_zone_chooser.h"
#include "forms/ui_bend_range_chooser.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QWidgetAction>
#include <QToolButton>
#include <QSlider>
#include <QLabel>
#include <QMenu>
#include <QTimer>
#include <QDebug>

MainWindow::MainWindow(TouchMpeHandler *handler, QWidget *parent)
    : QMainWindow(parent),
      handler_(handler)
{
    Ui::MainWindow *ui = new Ui::MainWindow;
    ui_.reset(ui);
    ui->setupUi(this);

    //
    QTimer *sendConfigurationTimer = sendConfigurationTimer_ = new QTimer(this);
    sendConfigurationTimer->setInterval(0);
    sendConfigurationTimer->setSingleShot(true);
    connect(sendConfigurationTimer, &QTimer::timeout, this, &MainWindow::sendConfiguration);

    //
    ui->lowerTouchPiano->setMpeZone(MpeZone::Lower);
    ui->upperTouchPiano->setMpeZone(MpeZone::Upper);
    ui->lowerTouchPiano->setListener(handler);
    ui->upperTouchPiano->setListener(handler);

    //
    qobject_cast<QToolButton *>(ui->toolBar->widgetForAction(ui->actionSettingsMenu))->setPopupMode(QToolButton::InstantPopup);
    QMenu *menu = new QMenu(this);
    setupSettingsMenu(menu);
    ui->actionSettingsMenu->setMenu(menu);

    //
    connect(ui->actionSend_configuration, &QAction::triggered, this, &MainWindow::sendConfiguration);
    connect(ui->actionDeviceInfo, &QAction::triggered, this, &MainWindow::execDeviceInfoDialog);
    connect(ui->actionChoose_MIDI_port, &QAction::triggered, this, &MainWindow::chooseMidiPort);
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::execHelpDialog);
    connect(keyScaleSlider_, &QSlider::valueChanged, this, &MainWindow::updateKeyScale);
    connect(mpeZoneSlider_, &QSlider::valueChanged, this, &MainWindow::updateMpeZones);
    connect(bendRangeSlider_, &QSlider::valueChanged, this, &MainWindow::updateBendRange);

    //
    updateMpeZones();
    updateBendRange();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupSettingsMenu(QMenu *menu)
{
    QWidget *w;
    QWidgetAction *wa;

    wa = new QWidgetAction(this);
    w = new QWidget;
    wa->setDefaultWidget(w);
    Ui::KeyScaleChooser uiScale;
    uiScale.setupUi(w);
    menu->addAction(wa);

    keyScaleSlider_ = uiScale.horizontalSlider;

    ///
    {
        menu->addSeparator();

        wa = new QWidgetAction(this);
        w = new QWidget;
        wa->setDefaultWidget(w);
        Ui::MPEZoneChooser uiZone;
        uiZone.setupUi(w);
        menu->addAction(wa);

        mpeZoneSlider_ = uiZone.horizontalSlider;
        mpeLowerZoneLabel_ = uiZone.lowerZoneLabel;
        mpeUpperZoneLabel_ = uiZone.upperZoneLabel;

        uiZone.horizontalSlider->setRange(0, 14);
        uiZone.horizontalSlider->setValue(7);

        int mpeZoneLabelWidth = mpeLowerZoneLabel_->fontMetrics().horizontalAdvance("WW");
        mpeLowerZoneLabel_->setFixedWidth(mpeZoneLabelWidth);
        mpeUpperZoneLabel_->setFixedWidth(mpeZoneLabelWidth);
    }

    ///
    {
        menu->addSeparator();

        wa = new QWidgetAction(this);
        w = new QWidget;
        wa->setDefaultWidget(w);
        Ui::BendRangeChooser uiBendRange;
        uiBendRange.setupUi(w);
        menu->addAction(wa);

        bendRangeSlider_ = uiBendRange.horizontalSlider;
        bendRangeLabel_ = uiBendRange.bendRangeLabel;

        uiBendRange.horizontalSlider->setRange(12, 96);
        uiBendRange.horizontalSlider->setValue(48);

        int bendRangeLabelWidth = bendRangeLabel_->fontMetrics().horizontalAdvance("WW");
        bendRangeLabel_->setFixedWidth(bendRangeLabelWidth);
        bendRangeLabel_->setFixedWidth(bendRangeLabelWidth);
    }
}

void MainWindow::execDeviceInfoDialog()
{
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Device information"));

    QGridLayout *grid = new QGridLayout;
    dlg->setLayout(grid);

    QPixmap yesPic = QPixmap(":/images/noto/emoji_u2714.png").scaledToHeight(32, Qt::SmoothTransformation);
    QPixmap noPic = QPixmap(":/images/noto/emoji_u274c.png").scaledToHeight(32, Qt::SmoothTransformation);
    QPixmap maybePic = QPixmap(":/images/noto/emoji_u2753.png").scaledToHeight(32, Qt::SmoothTransformation);

    int row = 0;

    auto insertStatus =
        [grid, &row]
        (const QString &title, bool test, const QString &yesText, const QString &noText, const QPixmap &yesPic, const QPixmap &noPic) {
            QLabel *iconLabel = new QLabel;
            grid->addWidget(iconLabel, row, 0);
            iconLabel->setPixmap(test ? yesPic : noPic);
            QLabel *textLabel = new QLabel;
            grid->addWidget(textLabel, row, 1);
            QString text;
            text += "<p><b>" + title.toHtmlEscaped() + "</b></p>";
            text += "<p>" + (test ? yesText : noText) + "</p>";
            textLabel->setTextFormat(Qt::RichText);
            textLabel->setText(text);
            ++row;
        };

    insertStatus(tr("MIDI ports"), theApp()->deviceSupportsMidiPorts(),
                 tr("This device permits to communicate MIDI using ports."),
                 tr("This device does not permit to communicate MIDI using ports."),
                 yesPic, noPic);

    insertStatus(tr("Touch interface"), theApp()->deviceSupportsMultiTouch(),
                 tr("This device support multi-touch input."),
                 tr("This device does not support multi-touch input."),
                 yesPic, noPic);

    insertStatus(tr("Advanced touch interface"), theApp()->deviceSupportsAdvancedMultiTouch(),
                 tr("This device is able to track up to 5 fingers simultaneously."),
                 tr("This device is unable to track up to 5 fingers simultaneously."),
                 yesPic, noPic);

    insertStatus(tr("Touch pressure"), theApp()->deviceSupportsTouchPressure(),
                 tr("This device may be capable of sending touch pressure information."),
                 tr("This device is not capable of sending touch pressure information."),
                 maybePic, noPic);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    grid->addWidget(buttons, row, 0, 1, 2);
    ++row;

    connect(buttons, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    dlg->exec();
    dlg->deleteLater();
}

void MainWindow::execHelpDialog()
{
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Help"));

    QGridLayout *grid = new QGridLayout;
    dlg->setLayout(grid);

    int row = 0;

    QPixmap rocketPic = QPixmap(":/images/noto/emoji_u1f680.png").scaledToHeight(32, Qt::SmoothTransformation);
    QPixmap wrenchPic = QPixmap(":/images/noto/emoji_u1f527.png").scaledToHeight(32, Qt::SmoothTransformation);
    QPixmap keyboardPic = QPixmap(":/images/noto/emoji_u1f3b9.png").scaledToHeight(32, Qt::SmoothTransformation);
    QPixmap infoPic = QPixmap(":/images/noto/emoji_u2139.png").scaledToHeight(32, Qt::SmoothTransformation);
#ifdef Q_OS_ANDROID
    QPixmap robotPic = QPixmap(":/images/noto/emoji_u1f916.png").scaledToHeight(32, Qt::SmoothTransformation);
#endif

    auto insertSeparator =
        [grid, &row]
        () {
            QFrame *frame = new QFrame;
            grid->addWidget(frame, row, 0, 1, 2);
            frame->setFrameShape(QFrame::HLine);
            ++row;
        };

    auto insertPictureText =
        [grid, &row]
        (const QString &title, const QString &contentText, const QPixmap &pic) {
            QLabel *iconLabel = new QLabel;
            grid->addWidget(iconLabel, row, 0);
            iconLabel->setPixmap(pic);
            QLabel *textLabel = new QLabel;
            grid->addWidget(textLabel, row, 1);
            QString text;
            text += "<p><b>" + title.toHtmlEscaped() + "</b></p>";
            text += "<p>" + contentText + "</p>";
            textLabel->setTextFormat(Qt::RichText);
            textLabel->setText(text);
            ++row;
        };

    insertPictureText(tr("Send configuration"),
                      tr("Tells the connected MPE-compatible synthesizer to enter Polyphonic Expression mode.<br />"
                         "By default, synthesizers are not generally preconfigured to use Polyphonic Expression."),
                      rocketPic);

    insertSeparator();

    insertPictureText(tr("Settings"),
                      tr("The <b>MPE zone</b> setting can adjust the extent of the lower zone and the upper zone.<br />"
                         " The zone division allows to control distinct instruments using each hand.<br />"
                         " 14 available channels can be allocated as desired between the zones.<br />"
                         "The <b>bend range</b> indicates the limit of pitch variation after the initial press.<br />"
                          " A greater variation is allowed at higher values, at the expense of resolution."),
                      wrenchPic);

    insertSeparator();

    insertPictureText(tr("MIDI port"),
                      tr("Selects a MIDI port which is used to transmit data to the instrument."),
                      keyboardPic);

    insertSeparator();

    insertPictureText(tr("Device information"),
                      tr("Verifies presence of device features which guarantee ideal usage experience."),
                      infoPic);

#ifdef Q_OS_ANDROID
    insertSeparator();

    insertPictureText(tr("Android MIDI"),
                      tr("To enable MIDI output via USB port, switch the USB connection to MIDI mode.<br />"
                         " Some devices are not compatible with MIDI mode."),
                      robotPic);
#endif

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    grid->addWidget(buttons, row, 0, 1, 2);
    ++row;

    connect(buttons, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    dlg->exec();
    dlg->deleteLater();
}

void MainWindow::chooseMidiPort()
{
    Ui::MainWindow &ui = *ui_;

    QWidget *button = ui.toolBar->widgetForAction(ui.actionChoose_MIDI_port);
    QMenu menu;
    QAction *action;

    QStringList ports = theApp()->getMidiPorts();

    QIcon musicIcon = QIcon(":/images/noto/emoji_u1f3bc.png");

    if (ports.empty()) {
        action = menu.addAction(tr("No MIDI ports"));
        action->setDisabled(true);
    }
    else {
        for (unsigned i = 0, n = ports.size(); i < n; ++i) {
            QString portName = ports[i];
            QString portDisplayName = theApp()->getMidiPortDisplayName(portName);
            action = menu.addAction(portDisplayName);
            action->setData(portName);
            action->setIcon(musicIcon);
        }
    }

    QAction *choice = menu.exec(button->mapToGlobal(button->rect().bottomLeft()));
    if (choice) {
        QString portName = choice->data().toString();
        theApp()->useMidiPort(portName);
    }
}

void MainWindow::updateMpeZones()
{
    Ui::MainWindow &ui = *ui_;

    unsigned mpeLowerValue = mpeZoneSlider_->value();
    unsigned mpeUpperValue = 14 - mpeLowerValue;

    mpeLowerZoneLabel_->setText(QString::number(mpeLowerValue));
    mpeUpperZoneLabel_->setText(QString::number(mpeUpperValue));

    ui.lowerFrame->setVisible(mpeLowerValue > 0);
    ui.upperFrame->setVisible(mpeUpperValue > 0);

    sendConfigurationTimer_->start();
}

void MainWindow::updateBendRange()
{
    unsigned bendRangeValue = bendRangeSlider_->value();
    bendRangeLabel_->setText(QString::number(bendRangeValue));

    sendConfigurationTimer_->start();
}

void MainWindow::updateKeyScale()
{
    Ui::MainWindow &ui = *ui_;
    QSlider *sl = keyScaleSlider_;
    qreal scale = (qreal)sl->value();
    ui.lowerTouchPiano->setScaleValue(scale);
    ui.upperTouchPiano->setScaleValue(scale);
}

void MainWindow::sendConfiguration()
{
    unsigned mpeLowerValue = mpeZoneSlider_->value();
    unsigned bendRangeValue = bendRangeSlider_->value();

    TouchMpeHandler *handler = handler_;
    handler->setMpeLowerValue(mpeLowerValue);
    handler->setBendRangeValue(bendRangeValue);
    handler->sendConfiguration();
}
