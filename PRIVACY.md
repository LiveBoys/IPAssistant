# IP Assistant Privacy Policy

Effective date: July 27, 2026

IP Assistant is an open-source, local Windows network configuration utility.
The application does not require an account and does not include analytics,
advertising, telemetry, crash reporting, or cloud synchronization.

## Data processed locally

IP Assistant processes the following information only on the user's computer:

- saved network profiles, including adapter identifiers, IP addresses,
  gateways, DNS servers, and profile names;
- application preferences, including language, startup, and system tray
  settings;
- current Windows network adapter information required to display and apply a
  selected profile;
- local diagnostic error messages produced while applying or restoring a
  network configuration.

Network profiles are stored in the application's local `data` directory.
Preferences are stored in the current user's Windows registry through Qt
`QSettings`. Privileged network changes are sent only between
`IPAssistant.exe` and the local `IPAssistantService.exe` Windows service.

## Data transmission

IP Assistant does not transmit personal information, network profiles, adapter
identifiers, or usage information to the developer or to any external server.
The application does not contact a developer-operated service.

Applying a profile changes the Windows network configuration. Windows and
other applications may subsequently use the configured gateway or DNS
servers; that operating-system network activity is outside IP Assistant's
control.

## Data retention and deletion

Saved profiles remain on the user's computer until they are deleted in the
application or the local data files are removed. The traditional installer
keeps the `data` directory during uninstall so profiles can survive an upgrade
or reinstall.

To remove all IP Assistant data:

1. Uninstall IP Assistant.
2. Delete `%LocalAppData%\Programs\IPAssistant\data` if it remains.
3. Delete `HKEY_CURRENT_USER\Software\IPAssistant\IP Assistant` to remove
   application preferences.
4. Remove the `IPAssistant` value under
   `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run` if it
   remains.

## Third-party components

IP Assistant is built with Qt. The distributed Qt runtime libraries operate
locally as part of the application. Any website, source-code host, or download
provider used to obtain IP Assistant may have its own privacy policy.

## Changes to this policy

Material privacy changes will be documented in this file and in the project's
release notes. The effective date above will be updated.

## Contact

Privacy questions may be submitted through the
[GitHub issue tracker](https://github.com/LiveBoys/IPAssistant/issues). Do not
include passwords, private network details, or other sensitive information in
a public issue.

---

## 中文摘要

IP Assistant 默认不收集遥测、崩溃报告或使用统计，不要求账号，也不会把
Profile、网卡标识、IP、网关或 DNS 配置上传到开发者服务器。配置和设置仅保存在
用户本机。卸载程序默认保留 `data` 目录；如需彻底删除数据，请按上文“Data
retention and deletion”步骤清理本地目录和注册表。
