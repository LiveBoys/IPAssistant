#ifndef IPSWITCH_SETTINGS_DIALOG_H
#define IPSWITCH_SETTINGS_DIALOG_H

#include <QDialog>
#include "ui_SettingsDialog.h"

/// Settings dialog with grouped options, descriptions, language selector, and live status preview.
class SettingsDialog : public QDialog, private Ui::SettingsDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    bool startWithWindows() const;
    bool minimizeToTray() const;

private slots:
    void updateStatusLabel();
};

#endif // IPSWITCH_SETTINGS_DIALOG_H
