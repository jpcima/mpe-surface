#include "touch_piano.h"
#include "touch_piano_listener.h"
#include "my_nanovg.h"
#include <QTouchEvent>
#include <QPainter>
#include <QEvent>
#include <QTimer>
#include <QDebug>

static constexpr bool key_octave_color[12] = {0, 1, 0, 1, 0,
                                              0, 1, 0, 1, 0, 1, 0};

TouchPiano::TouchPiano(QWidget *parent)
    : QOpenGLWidget(parent),
      pixelRatio_(devicePixelRatioF()),
      keyColor_(new QColor[NumKeys]),
      keyRect_(new QRectF[NumKeys])
{
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAttribute(Qt::WA_NoBackground);

    setFocusPolicy(Qt::StrongFocus);

    topMargin_ = 40 * pixelRatio_;
    bottomMargin_ = 20 * pixelRatio_;
    keyWidth_ = 30 * pixelRatio_;

    for (unsigned i = 0; i < NumKeys; ++i) {
        qreal hue = i * 0.1;
        hue -= (int)hue;
        keyColor_[i] = QColor::fromHsvF(hue, 0.5, 0.5).toRgb();
    }

    updateSizesAndPositions();
}

TouchPiano::~TouchPiano()
{
}

void TouchPiano::setScaleValue(qreal scale)
{
    if (scaleValue_ == scale)
        return;

    scaleValue_ = scale;
    updateSizesAndPositions();
}

void TouchPiano::setListener(TouchPianoListener *listener)
{
    listener_ = listener;
}

bool TouchPiano::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);

        slideFingerId_ = -1;
        if (releaseAllTouchPoints(touchEvent->timestamp()))
            return true;

        break;
    }

    case QEvent::TouchBegin: {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);

        slideFingerId_ = -1;
        releaseAllTouchPoints(touchEvent->timestamp());

        const QList<QTouchEvent::TouchPoint> &touchPoints = touchEvent->touchPoints();
        if (touchPoints.empty())
            break;

        bool pointsAreIn = true;
        for (const QTouchEvent::TouchPoint &tp : touchPoints) {
            Point3d xyz = xyzFromPos(tp.pos(), tp.pressure());
            if (xyz != clampXyz(xyz)) { pointsAreIn = false; break; }
        }

        if (pointsAreIn) {
            for (const QTouchEvent::TouchPoint &tp : touchPoints)
                moveTouchPoint(touchEvent->timestamp(), tp.id(), mapToPiano(tp.pos() * pixelRatio_), tp.pressure());
            return true;
        }
        else {
            slideOrigin_ = touchPoints.front().pos();
            slideFingerId_ = touchPoints.front().id();
            slideInitialXScrollPosition_ = xScrollPosition_;
            return true;
        }

        break;
    }

    case QEvent::TouchUpdate: {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        const QList<QTouchEvent::TouchPoint> &touchPoints = touchEvent->touchPoints();

        if (slideFingerId_ == -1) {
            for (auto it = keyTouch_.begin(), end = keyTouch_.end(); it != end;) {
                int id = (it++)->first;
                auto search = std::find_if(
                    touchPoints.begin(), touchPoints.end(),
                    [id](const QTouchEvent::TouchPoint &tp) -> bool { return tp.id() == id; });
                if (search == touchPoints.end())
                    releaseTouchPoint(touchEvent->timestamp(), id);
            }
            for (const QTouchEvent::TouchPoint &tp : touchPoints)
                moveTouchPoint(touchEvent->timestamp(), tp.id(), mapToPiano(tp.pos() * pixelRatio_), tp.pressure());
            return true;
        }
        else {
            const QTouchEvent::TouchPoint *tp = nullptr;
            for (const QTouchEvent::TouchPoint &curTp : touchPoints) {
                tp = &curTp;
                if (tp) break;
            }
            if (tp) {
                qreal delta = (qreal)(tp->pos().x() - slideOrigin_.x()) / viewportWidth();
                delta *= -1.0;

                qreal scroll = slideInitialXScrollPosition_ + delta;
                scroll = (scroll > 0.0) ? scroll : 0.0;
                scroll = (scroll < 1.0) ? scroll : 1.0;
                xScrollPosition_ = scroll;

                repaint();
                return true;
            }
        }
        break;
    }

    default:
        break;
    }

    return QOpenGLWidget::event(event);
}

void TouchPiano::resizeEvent(QResizeEvent *event)
{
    QOpenGLWidget::resizeEvent(event);

    updateSizesAndPositions();
}

static int nvgCreateFontQtResource(NVGcontext *vg, const char *name, const QString &path)
{
    QFile file(path);
    file.open(QFile::ReadOnly);

    qint64 size = file.size();
    if (size <= 0)
        return -1;

    unsigned char *ptr = (unsigned char *)malloc((size_t)size);
    if (file.read((char *)ptr, size) != size) {
        free(ptr);
        return -1;
    }

    int font = nvgCreateFontMem(vg, name, ptr, size, true);
    if (font == -1) {
        free(ptr);
        return -1;
    }

    return font;
}

void TouchPiano::initializeGL()
{
    initializeOpenGLFunctions();

    NVGcontext *vg = nvgCreateGL(NVG_ANTIALIAS|NVG_STENCIL_STROKES);
    vg_.reset(vg);

    nvgCreateFontQtResource(vg, "Regular", ":/fonts/fonts/DejaVuSans.ttf");
    nvgCreateFontQtResource(vg, "Bold", ":/fonts/DejaVuSans-Bold.ttf");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    qreal vw = viewportWidth();
    qreal vh = viewportHeight();

    glViewport(0, 0, vw, vh);
}

void TouchPiano::resizeGL(int w, int h)
{
    qreal vw = viewportWidth();
    qreal vh = viewportHeight();

    glViewport(0, 0, vw, vh);
}

void TouchPiano::paintGL()
{
    qreal tm = topMargin_;
    qreal kw = scaledKeyWidth();
    QSize size(scaledFullWidth(), viewportHeight());
    qreal scrollWidth = size.width() - viewportWidth();
    qreal xoff = -xScrollPosition_ * scrollWidth;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    NVGcontext *vg = vg_.get();
    if (!vg)
        return;

    nvgBeginFrame(vg, viewportWidth(), viewportHeight(), pixelRatio_);

    // Background
    {
        for (unsigned key = 0; key < NumKeys; key += 12) {
            QRectF keyRect = keyRect_[key];

            nvgBeginPath(vg);
            nvgMoveTo(vg, xoff + keyRect.x(), 0);
            nvgLineTo(vg, xoff + keyRect.x(), tm);
            nvgStrokeWidth(vg, 1.0);
            nvgStrokeColor(vg, nvgRGB(0xff, 0xff, 0xff));
            nvgStroke(vg);

            float bounds[4];

            std::string text = std::to_string((int)key / 12 - 1);
            nvgFontFace(vg, "Bold");
            nvgFontSize(vg, 20.0);
            nvgFillColor(vg, nvgRGB(0xff, 0xff, 0xff));
            nvgTextBounds(vg, 0, 0, text.c_str(), nullptr, bounds);
            qreal textw = bounds[2] - bounds[0];
            qreal texth = bounds[3] - bounds[1];
            qreal textx = keyRect.x() + 0.5 * (kw - textw);
            qreal texty = keyRect.y() + 0.5 * (texth - tm);
            nvgText(vg, xoff + textx, texty, text.c_str(), nullptr);
        }

        for (unsigned a = 0; a < 2; ++a) {
            for (unsigned key = 0; key < NumKeys; ++key) {
                QColor keyColor = keyColor_[key];
                QRectF keyRect = keyRect_[key];

                if (a != key_octave_color[key % 12])
                    continue;

                nvgBeginPath(vg);
                nvgRect(vg, xoff + keyRect.x(), keyRect.y(), keyRect.width(), keyRect.height());
                nvgFillColor(vg, nvgRGB(keyColor.red(), keyColor.green(), keyColor.blue()));
                nvgFill(vg);
                nvgStrokeWidth(vg, 1.0);
                nvgStrokeColor(vg, nvgRGB(0x00, 0x00, 0x00));
                nvgStroke(vg);
            }
        }
    }

    // Foreground
    {
        for (auto &keyTouch : keyTouch_) {
            const TouchState &ts = keyTouch.second;
            QPointF pos = posFromXyz(ts.xyz);
            qreal pressure = ts.xyz.z;
            qreal b = kw * 0.5;
            qreal r = b + b * pressure;

            nvgBeginPath(vg);
            nvgEllipse(vg, xoff + pos.x(), pos.y(), r, r);
            nvgFillColor(vg, nvgRGBA(0x72, 0x9f, 0xcf, 0x80));
            nvgFill(vg);
            nvgStrokeWidth(vg, 1.0);
            nvgStrokeColor(vg, nvgRGB(0x00, 0x00, 0x00));
            nvgStroke(vg);
        }
    }

    nvgEndFrame(vg);
}

bool TouchPiano::releaseTouchPoint(ulong timestamp, int id)
{
    auto it = keyTouch_.find(id);
    if (it == keyTouch_.end())
        return false;

    // TouchState &ts = it->second;
    // Point3d xyz = ts.xyz;
    // qDebug() << "Release" << id << xyz;

    if (TouchPianoListener *listener = listener_)
        listener->touchEnd(mpeZone_, id, timestamp);

    keyTouch_.erase(it);

    repaint();
    return true;
}

bool TouchPiano::releaseAllTouchPoints(ulong timestamp)
{
    if (keyTouch_.empty())
        return false;

    do {
        releaseTouchPoint(timestamp, keyTouch_.begin()->first);
    } while (!keyTouch_.empty());

    return true;
}

bool TouchPiano::moveTouchPoint(ulong timestamp, int id, QPointF pos, qreal pressure)
{
    TouchState *ts = nullptr;
    bool create = false;

    auto it = keyTouch_.find(id);
    if (it != keyTouch_.end())
        ts = &it->second;
    else {
        ts = &keyTouch_[id];
        ts->id = id;
        create = true;
    }

    Point3d xyz = clampXyz(xyzFromPos(pos, pressure));
    ts->xyz = xyz;

    // qDebug() << "Move" << id << xyz;

    if (TouchPianoListener *listener = listener_) {
        if (create)
            listener->touchBegin(mpeZone_, id, xyz, timestamp);
        else
            listener->touchUpdate(mpeZone_, id, xyz, timestamp);
    }

    repaint();
    return true;
}

qreal TouchPiano::viewportWidth() const
{
    return pixelRatio_ * width();
}

qreal TouchPiano::viewportHeight() const
{
    return pixelRatio_ * height();
}

qreal TouchPiano::scaledKeyWidth() const
{
    return (1.0 + scaleValue_ * 0.075) * keyWidth_;
}

qreal TouchPiano::scaledOctaveWidth() const
{
    return 12 * scaledKeyWidth();
}

qreal TouchPiano::scaledFullWidth() const
{
    QRectF rightmostRect = keyRect_[NumKeys - 1];
    return rightmostRect.x() + rightmostRect.width();
}

void TouchPiano::updateSizesAndPositions()
{
    qreal wh = viewportHeight();

    qreal tm = topMargin_;
    qreal bm = bottomMargin_;

    qreal kw = scaledKeyWidth();
    qreal ow = scaledOctaveWidth();
    qreal kh = wh - (tm + bm);

    const int white_key_indices[12] =
        {0, -1, 1, -1, 2,
         3, -1, 4, -1, 5, -1, 6};
    qreal white_key_width = kw * (12.0 / 7.0);

    for (unsigned i = 0; i < NumKeys; ++i) {
        qreal sx = i * kw;
        qreal sy = tm;
        qreal sw = kw;
        qreal sh = kh;

        if (key_octave_color[i % 12])
            sh *= 0.6;

        int white_key_index = white_key_indices[i % 12];

        if (0) {
            if (white_key_index != -1) {
                sx = ow * (i / 12) + white_key_index * white_key_width;
                sw = white_key_width;
                if (i + 1 == NumKeys)
                    sw = kw;
            }
        }
        else {
            switch (white_key_index) {
            case 0:
            case 3:
                sw += 0.5 * sw;
                break;
            case 1:
            case 4:
            case 5:
                sx -= 0.5 * sw;
                sw += sw;
                break;
            case 2:
            case 6:
                sx -= 0.5 * sw;
                sw += 0.5 * sw;
                break;
            }
        }

        keyRect_[i] = QRectF(sx, sy, sw, sh);
    }

    repaint();
}

QPointF TouchPiano::mapToPiano(QPointF pos)
{
    qreal scrollWidth = scaledFullWidth() - viewportWidth();
    pos.setX(pos.x() + xScrollPosition_ * scrollWidth);
    return pos;
}

Point3d TouchPiano::xyzFromPos(QPointF pos, qreal pressure) const
{
    Point3d xyz;

    QRectF rect = keyRect_[0];
    qreal kw = scaledKeyWidth();

    xyz.x = (pos.x() - rect.x()) / kw - 0.5;
    xyz.y = 1.0 - (pos.y() - rect.y()) / rect.height();
    xyz.z = pressure;

    return xyz;
}

QPointF TouchPiano::posFromXyz(Point3d xyz)
{
    QPointF pos;

    QRectF rect = keyRect_[0];
    qreal kw = scaledKeyWidth();

    pos.setX((xyz.x + 0.5) * kw + rect.x());
    pos.setY((1.0 - xyz.y) * rect.height() + rect.y());

    return pos;
}

Point3d TouchPiano::clampXyz(Point3d xyz)
{
    xyz.x = (xyz.x > -0.5) ? xyz.x : -0.5;
    xyz.x = (xyz.x < (NumKeys - 0.5)) ? xyz.x : (NumKeys - 0.5);

    xyz.y = (xyz.y > 0.0) ? xyz.y : 0.0;
    xyz.y = (xyz.y < 1.0) ? xyz.y : 1.0;

    xyz.z = (xyz.z > 0.0) ? xyz.z : 0.0;
    xyz.z = (xyz.z < 1.0) ? xyz.z : 1.0;

    return xyz;
}

void TouchPiano::NanoVGDeleter::operator()(NVGcontext *x)
{
    nvgDeleteGL(x);
};
