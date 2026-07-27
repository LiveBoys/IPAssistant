#include "ToggleSwitch.h"

#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOption>
#include <QDebug>

ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QCheckBox(parent)
{
    setCursor(Qt::PointingHandCursor);
    connect(this, &QCheckBox::toggled, this, [this](bool checked) {
        auto* anim = new QPropertyAnimation(this, "knobPos", this);
        anim->setDuration(180);
        anim->setEasingCurve(QEasingCurve::InOutCubic);
        anim->setStartValue(m_knobPos);
        anim->setEndValue(checked ? 1.0f : 0.0f);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

ToggleSwitch::ToggleSwitch(const QString& text, QWidget* parent)
    : ToggleSwitch(parent)
{
    setText(text);
}

QSize ToggleSwitch::sizeHint() const
{
    // The track + text spacing
    int textW = fontMetrics().horizontalAdvance(text());
    int w = m_trackWidth + (textW > 0 ? 8 + textW : 0) + 4;
    int h = qMax(m_trackHeight, fontMetrics().height());
    return QSize(w, h);
}

QSize ToggleSwitch::minimumSizeHint() const
{
    return sizeHint();
}

void ToggleSwitch::setKnobPos(float pos)
{
    m_knobPos = pos;
    update();
}

void ToggleSwitch::resizeEvent(QResizeEvent* e)
{
    QCheckBox::resizeEvent(e);
    // On first show, snap to current checked state
    if (!isVisible() && !underMouse()) {
        m_knobPos = isChecked() ? 1.0f : 0.0f;
    }
}

void ToggleSwitch::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // --- Draw track ---
    int trackX = 0;
    int trackY = (height() - m_trackHeight) / 2;
    QRectF trackRect(trackX, trackY, m_trackWidth, m_trackHeight);

    // Interpolate track color
    QColor offTrack(0xd1, 0xd1, 0xd1);
    QColor onTrack(0x34, 0xc7, 0x59);   // Apple green
    float t = m_knobPos;
    int r = int(offTrack.red()   + (onTrack.red()   - offTrack.red())   * t);
    int g = int(offTrack.green() + (onTrack.green() - offTrack.green()) * t);
    int b = int(offTrack.blue()  + (onTrack.blue()  - offTrack.blue())  * t);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(r, g, b));
    p.drawRoundedRect(trackRect, m_trackHeight / 2.0, m_trackHeight / 2.0);

    // --- Draw knob ---
    int padding = 2;
    int maxTravel = m_trackWidth - m_knobDiameter - padding * 2;
    int knobX = trackX + padding + int(maxTravel * m_knobPos);
    int knobY = trackY + (m_trackHeight - m_knobDiameter) / 2;
    QRectF knobRect(knobX, knobY, m_knobDiameter, m_knobDiameter);

    // Knob shadow
    p.setBrush(QColor(0, 0, 0, 30));
    p.drawEllipse(knobRect.translated(0, 1));

    // Knob body
    p.setBrush(Qt::white);
    p.drawEllipse(knobRect);

    // --- Draw text ---
    if (!text().isEmpty()) {
        QStyleOptionButton opt;
        opt.initFrom(this);
        opt.text = text();
        opt.rect = QRect(m_trackWidth + 8, 0,
                         width() - m_trackWidth - 8, height());
        style()->drawControl(QStyle::CE_CheckBoxLabel, &opt, &p, this);
    }
}
