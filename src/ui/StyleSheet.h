#ifndef IPSWITCH_STYLE_SHEET_H
#define IPSWITCH_STYLE_SHEET_H

#include <QString>

namespace StyleSheet
{
    inline constexpr const char* kApp = R"(
        QWidget {
            color: #2c2c2c;
            font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
            font-size: 10pt;
        }
        #CentralWidget {
            background: #ffffff;
        }

        /* ---------- top bar ---------- */
        #TopBar {
            background: #3b8ee0;
            min-height: 36px;
            max-height: 36px;
        }
        #TopBar QLabel#TitleLabel {
            color: #ffffff;
            font-weight: 600;
            font-size: 11pt;
            padding-left: 6px;
        }
        #TopBar QPushButton {
            background: transparent;
            border: none;
            color: #ffffff;
            font-size: 14pt;
            min-width: 28px;
            max-width: 28px;
            min-height: 28px;
            max-height: 28px;
        }
        #TopBar QPushButton:hover { background: rgba(255,255,255,0.18); }
        #TopBar QPushButton:pressed { background: rgba(0,0,0,0.15); }

        /* ---------- left panel ---------- */
        #LeftPanel { background: #f5f6f8; }

        /* ---------- profile list ---------- */
        QListWidget#ProfileList {
            background: #f7f7f7;
            border: 1px solid #d0d0d0;
            outline: none;
        }
        QListWidget#ProfileList::item {
            padding: 6px 8px;
            border: none;
        }
        QListWidget#ProfileList::item:selected {
            background: #cfe4f7;
            color: #1f4e7a;
        }

        /* ---------- form ---------- */
        QLabel#SectionTitle { font-weight: 600; padding: 2px 0; }
        QLabel#FieldLabel   { color: #6b6b6b; }
        QLineEdit, QComboBox {
            background: #ffffff;
            border: 1px solid #c8c8c8;
            border-radius: 3px;
            padding: 6px 8px;
            selection-background-color: #3b8ee0;
            min-height: 22px;
        }
        QLineEdit:focus, QComboBox:focus { border-color: #3b8ee0; }
        QLineEdit:disabled, QComboBox:disabled {
            background: #f0f0f0; color: #a0a0a0;
        }

        QComboBox::drop-down { border: none; width: 18px; }
        QComboBox QAbstractItemView {
            background: #ffffff;
            border: 1px solid #c8c8c8;
            selection-background-color: #3b8ee0;
            selection-color: #ffffff;
            outline: none;
        }

        /* ---------- toolbar buttons under the list ---------- */
        QPushButton#ToolButton {
            background: #ffffff;
            border: 1px solid #c8c8c8;
            border-radius: 14px;
            min-width: 28px;
            max-width: 28px;
            min-height: 28px;
            max-height: 28px;
            font-size: 13pt;
            color: #3b8ee0;
        }
        QPushButton#ToolButton:hover  { background: #eaf3fb; }
        QPushButton#ToolButton:pressed{ background: #d4e7f7; }
        QPushButton#ToolButton:disabled { color: #b0b0b0; background: #f5f5f5; }

        /* ---------- activate button ---------- */
        QPushButton#ActivateButton {
            background: #3b8ee0;
            color: #ffffff;
            border: none;
            border-radius: 3px;
            padding: 8px 0;
            font-weight: 600;
            text-align: center;
        }
        QPushButton#ActivateButton:hover    { background: #2f7ccc; }
        QPushButton#ActivateButton:pressed  { background: #266bb3; }
        QPushButton#ActivateButton:disabled { background: #b6d6f0; }

        /* ---------- dialog ---------- */
        QDialog { background: #ffffff; }
        QDialogButtonBox QPushButton {
            background: #ffffff;
            color: #3b8ee0;
            border: 1px solid #3b8ee0;
            border-radius: 3px;
            padding: 6px 18px;
            font-weight: 600;
        }
        QDialogButtonBox QPushButton:hover {
            background: #eaf3fb;
        }
        QDialogButtonBox QPushButton:pressed {
            background: #d4e7f7;
        }
        QDialogButtonBox QPushButton:disabled {
            color: #b0b0b0;
            border-color: #c8c8c8;
        }

        /* ---------- settings dialog ---------- */
        QGroupBox {
            font-weight: 600;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 14px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
            color: #3b8ee0;
        }

        /* ---------- inline IP/DNS groups (flat with title) ---------- */
        QGroupBox#IpGroup, QGroupBox#DnsGroup {
            background: #fafbfc;
            border: 1px solid #e0e6eb;
            border-radius: 4px;
            margin-top: 10px;
            padding: 16px 8px 6px 8px;
            font-weight: 600;
        }
        QGroupBox#IpGroup::title, QGroupBox#DnsGroup::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 8px;
            top: 2px;
            padding: 0 4px;
            color: #3b8ee0;
            font-size: 9pt;
            background: #fafbfc;
        }
        QGroupBox#IpGroup:!enabled, QGroupBox#DnsGroup:!enabled {
            background: #f5f5f5;
            border-color: #ebebeb;
        }
        QGroupBox#IpGroup:!enabled::title, QGroupBox#DnsGroup:!enabled::title {
            color: #aaaaaa;
            background: #f5f5f5;
        }
        QGroupBox#IpGroup:!enabled QLineEdit, QGroupBox#DnsGroup:!enabled QLineEdit {
            background: #f5f5f5;
            color: #b0b0b0;
        }
    )";
}

#endif // IPSWITCH_STYLE_SHEET_H