#ifndef IPSWITCH_ABOUT_DIALOG_H
#define IPSWITCH_ABOUT_DIALOG_H

#include <QDialog>

/// Simple "About" dialog matching the design mock.
class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);
};

#endif // IPSWITCH_ABOUT_DIALOG_H