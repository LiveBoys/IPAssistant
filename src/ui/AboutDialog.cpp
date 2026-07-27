#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QPixmap>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    Ui::AboutDialog ui;
    ui.setupUi(this);

    QIcon appIcon;
#ifdef Q_OS_WIN
    HICON h = (HICON)LoadImageW(GetModuleHandleW(nullptr),
                                MAKEINTRESOURCEW(1),
                                IMAGE_ICON, 80, 80, 0);
    if (h) {
        appIcon = QPixmap::fromImage(QImage::fromHICON(h));
        DestroyIcon(h);
    }
#else
    appIcon = QApplication::windowIcon();
#endif
    if (!appIcon.isNull())
        ui.iconLabel->setPixmap(appIcon.pixmap(80, 80));

    ui.versionLabel->setText(
        tr("Version %1").arg(QApplication::applicationVersion()));

    ui.opensourceLabel->setText(
        tr("This project is open source software.\n"
           "Built with Qt 6 and C++ on Windows."));

    ui.copyrightLabel->setText(
        tr("Copyright \u00a9 2026 Jiugang He"));
}
