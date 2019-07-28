#pragma once
#include "mpe_definitions.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPixmap>
#include <unordered_map>
#include <memory>

class TouchPianoListener;
class QTouchEvent;
struct NVGcontext;

//
class TouchPiano : public QOpenGLWidget, protected QOpenGLFunctions {
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

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    bool releaseTouchPoint(ulong timestamp, int id);
    bool releaseAllTouchPoints(ulong timestamp);
    bool moveTouchPoint(ulong timestamp, int id, QPointF pos, qreal pressure);

    qreal viewportWidth() const;
    qreal viewportHeight() const;

    qreal scaledKeyWidth() const;
    qreal scaledOctaveWidth() const;
    qreal scaledFullWidth() const;

    void updateSizesAndPositions();

    QPointF mapToPiano(QPointF pos);

    Point3d xyzFromPos(QPointF pos, qreal pressure) const;
    QPointF posFromXyz(Point3d xyz);
    static Point3d clampXyz(Point3d xyz);

private:
    qreal pixelRatio_ = 1.0;

    qreal topMargin_ = 0;
    qreal bottomMargin_ = 0;
    qreal keyWidth_ = 0;
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

    struct NanoVGDeleter { void operator()(NVGcontext *x); };
    std::unique_ptr<NVGcontext, NanoVGDeleter> vg_;
};
