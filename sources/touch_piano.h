#pragma once
#include "mpe_definitions.h"
#include <QWidget>
#include <QPixmap>
#include <unordered_map>
#include <memory>

class TouchPianoListener;
class QTouchEvent;

//
class TouchPiano : public QWidget {
    Q_OBJECT

public:
    explicit TouchPiano(QWidget *parent = nullptr);
    ~TouchPiano();

    void setScaleValue(qreal scale);

    void setListener(TouchPianoListener *listener);

    MpeZone mpeZone() const { return mpeZone_; }
    void setMpeZone(MpeZone mpeZone) { mpeZone_ = mpeZone; }

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool releaseTouchPoint(ulong timestamp, int id);
    bool releaseAllTouchPoints(ulong timestamp);
    bool moveTouchPoint(ulong timestamp, int id, QPointF pos, qreal pressure);

    qreal scaledKeyWidth() const;
    qreal scaledOctaveWidth() const;
    qreal scaledFullWidth() const;

    void updateSizesAndPositions();
    void paintBackground(QPainter &p);

    QPointF mapToPiano(QPointF pos);

    Point3d xyzFromPos(QPointF pos, qreal pressure) const;
    QPointF posFromXyz(Point3d xyz);
    static Point3d clampXyz(Point3d xyz);

private:
    int topMargin_ = 40;
    int bottomMargin_ = 20;
    int keyWidth_ = 30;
    qreal scaleValue_ = 0;
    qreal xScrollPosition_ = 0.0;

    enum { NumKeys = 128, NumWhiteKeys = 10 * 7 + 5, };

    std::unique_ptr<QColor[]> keyColor_;
    std::unique_ptr<QRectF[]> keyRect_;

    struct TouchState {
        int id = -1;
        Point3d xyz;
    };

    long slideFingerId_ = -1;
    QPointF slideOrigin_;
    qreal slideInitialXScrollPosition_ = 0.0;

    std::unordered_map<int, TouchState> keyTouch_;

    TouchPianoListener *listener_ = nullptr;
    MpeZone mpeZone_ = MpeZone::Lower;

#if defined(Q_OS_ANDROID)
    enum { HasSlowDrawing = true };
#else
    enum { HasSlowDrawing = false };
#endif
};
