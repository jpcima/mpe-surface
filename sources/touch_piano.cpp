#include "touch_piano.h"
#include "touch_piano_listener.h"
#include <QTouchEvent>
#include <QPainter>
#include <QEvent>
#include <QDebug>

static constexpr bool key_octave_color[12] = {0, 1, 0, 1, 0,
                                              0, 1, 0, 1, 0, 1, 0};

TouchPiano::TouchPiano(QWidget *parent)
    : QWidget(parent),
      keyColor_(new QColor[NumKeys]),
      keyRect_(new QRectF[NumKeys])
{
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAttribute(Qt::WA_NoBackground);

    setFocusPolicy(Qt::StrongFocus);

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
                moveTouchPoint(touchEvent->timestamp(), tp.id(), mapToPiano(tp.pos()), tp.pressure());
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
                moveTouchPoint(touchEvent->timestamp(), tp.id(), mapToPiano(tp.pos()), tp.pressure());
            return true;
        }
        else {
            const QTouchEvent::TouchPoint *tp = nullptr;
            for (const QTouchEvent::TouchPoint &curTp : touchPoints) {
                tp = &curTp;
                if (tp) break;
            }
            if (tp) {
                qreal delta = (qreal)(tp->pos().x() - slideOrigin_.x()) / width();
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

    return QWidget::event(event);
}

void TouchPiano::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    updateSizesAndPositions();
}

void TouchPiano::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    qreal kw = scaledKeyWidth();
    qreal scrollWidth = scaledFullWidth() - width();
    qreal xoff = -xScrollPosition_ * scrollWidth;

    QPainter p(this);
    paintBackground(p);

    p.setPen(QPen(Qt::black, 1.0));
    p.setBrush(QColor(0x72, 0x9f, 0xcf, 0x80));

    for (auto &keyTouch : keyTouch_) {
        const TouchState &ts = keyTouch.second;
        QPointF pos = posFromXyz(ts.xyz);
        qreal pressure = ts.xyz.z;
        qreal b = kw * 0.5;
        qreal r = b + b * pressure;

        if (!HasSlowDrawing) {
            p.drawEllipse(xoff + pos.x() - r, pos.y() - r, 2 * r, 2 * r);
            // p.drawLine(xoff + pos.x() - r, pos.y(), xoff + pos.x() + r, pos.y());
            // p.drawLine(xoff + pos.x(), pos.y() - r, xoff + pos.x(), pos.y() + r);
        }
    }
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

    if (!HasSlowDrawing)
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

    if (!HasSlowDrawing)
        repaint();

    return true;
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
    qreal wh = height();

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
        if (white_key_index != -1) {
            sx = ow * (i / 12) + white_key_index * white_key_width;
            sw = white_key_width;
            if (i + 1 == NumKeys)
                sw = kw;
        }

        keyRect_[i] = QRectF(sx, sy, sw, sh);
    }

    repaint();
}

void TouchPiano::paintBackground(QPainter &p)
{
    qreal tm = topMargin_;
    qreal kw = scaledKeyWidth();
    QSize size(scaledFullWidth(), height());
    qreal scrollWidth = size.width() - width();
    qreal xoff = -xScrollPosition_ * scrollWidth;

    p.fillRect(QRect(QPoint(), size), Qt::black);

    p.setPen(QPen(Qt::white, 1.0));

    for (unsigned key = 0; key < NumKeys; key += 12) {
        QRectF keyRect = keyRect_[key];

        p.drawLine(QPointF(xoff + keyRect.x(), 0).toPoint(),
                   QPointF(xoff + keyRect.x(), topMargin_).toPoint());

        QFontMetrics fm = p.fontMetrics();
        QString text = QString::number((int)key / 12 - 1);

        qreal textw = fm.horizontalAdvance(text);
        qreal texth = fm.height();
        qreal textx = keyRect.x() + 0.5 * (kw - textw);
        qreal texty = keyRect.y() + 0.5 * (texth - tm);

        p.drawText(QPointF(xoff + textx,texty).toPoint(), text);
    }

    p.setPen(QPen(Qt::black, 1.0));

    for (unsigned a = 0; a < 2; ++a) {
        for (unsigned key = 0; key < NumKeys; ++key) {
            QColor keyColor = keyColor_[key];
            QRectF keyRect = keyRect_[key];

            if (a != key_octave_color[key % 12])
                continue;

            p.setBrush(keyColor);
            p.drawRect(keyRect.adjusted(xoff, 0, xoff, 0).toAlignedRect());
        }
    }
}

QPointF TouchPiano::mapToPiano(QPointF pos)
{
    qreal scrollWidth = scaledFullWidth() - width();
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
