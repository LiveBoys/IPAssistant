#ifndef IPSWITCH_TOGGLE_SWITCH_H
#define IPSWITCH_TOGGLE_SWITCH_H

#include <QCheckBox>

class QPropertyAnimation;

/// Apple-style toggle switch with sliding knob animation.
class ToggleSwitch : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(float knobPos READ knobPos WRITE setKnobPos)
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);
    explicit ToggleSwitch(const QString& text, QWidget* parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    float knobPos() const { return m_knobPos; }
    void  setKnobPos(float pos);

    float m_knobPos = 0.0f;   // 0.0 = off (left), 1.0 = on (right)
    int   m_trackHeight = 22;
    int   m_trackWidth  = 40;
    int   m_knobDiameter = 18;
};

#endif // IPSWITCH_TOGGLE_SWITCH_H
