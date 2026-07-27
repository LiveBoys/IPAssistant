#ifndef IPSWITCH_ACTIVE_INDICATOR_DELEGATE_H
#define IPSWITCH_ACTIVE_INDICATOR_DELEGATE_H

#include <QStyledItemDelegate>

class ActiveIndicatorDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ActiveIndicatorDelegate(QObject* parent = nullptr);

    void setActiveName(const QString& name);
    QString activeName() const;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    QString m_activeName;
};

#endif // IPSWITCH_ACTIVE_INDICATOR_DELEGATE_H
