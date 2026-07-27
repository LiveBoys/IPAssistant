#include "ActiveIndicatorDelegate.h"

#include <QPainter>

static constexpr int kBarWidth = 4;
static constexpr int kBarMargin = 4;

ActiveIndicatorDelegate::ActiveIndicatorDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void ActiveIndicatorDelegate::setActiveName(const QString& name)
{
    m_activeName = name;
}

QString ActiveIndicatorDelegate::activeName() const
{
    return m_activeName;
}

void ActiveIndicatorDelegate::paint(QPainter* painter,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    // Let base class draw everything first (selection highlight, text, etc.)
    QStyledItemDelegate::paint(painter, option, index);

    if (m_activeName.isEmpty()) return;

    QString itemText = index.data(Qt::DisplayRole).toString();
    if (itemText != m_activeName) return;

    // Draw a colored vertical bar on the left edge
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    QColor barColor(0x3b, 0x8e, 0xe0); // primary blue
    painter->setPen(Qt::NoPen);
    painter->setBrush(barColor);

    QRect barRect(option.rect.left() + kBarMargin,
                  option.rect.top() + 2,
                  kBarWidth,
                  option.rect.height() - 4);
    painter->drawRect(barRect);

    painter->restore();
}

QSize ActiveIndicatorDelegate::sizeHint(const QStyleOptionViewItem& option,
                                         const QModelIndex& index) const
{
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    // Add left margin for the indicator bar
    return QSize(sz.width() + kBarWidth + kBarMargin * 2, sz.height());
}
