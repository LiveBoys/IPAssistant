# IP Assistant

[简体中文](README_CN.md) | English

IP Assistant is a Qt 6 network configuration switcher for Windows 10 and
Windows 11. It stores multiple IPv4, IPv6, and DNS configurations for each
network adapter and lets users activate them quickly from the main window or
the system tray.

Current version: **1.0.0**

License: [MIT](LICENSE)

Privacy and security:

- [Privacy Policy](PRIVACY.md)
- [Security Policy and Vulnerability Reporting](SECURITY.md)
- [Code Signing Policy](.signpath/README.md)

## Features

- Create multiple network profiles for different adapters.
- Configure IPv4 DHCP, static addresses, subnet masks, default gateways, and
  DNS servers.
- Configure IPv6 automatic addressing, static addresses, prefix lengths,
  default gateways, and DNS servers.
- Bind profiles to stable adapter GUIDs so renaming an adapter does not break
  profile activation.
- Fall back to the adapter name when a GUID is no longer available; preserve
  the profile and show an invalid-adapter warning when neither identifier can
  be resolved.
- Save the current network state before applying changes, attempt to restore
  it if any step fails, and return the actual operation result.
- Support Windows startup, the system tray, and 18 interface languages.

## Supported Languages

The application includes the following languages and selects one on first
launch according to the Windows regional settings:

| Language | Locale |
| --- | --- |
| English | `en_US` |
| 简体中文 | `zh_CN` |
| 繁體中文 | `zh_TW` |
| 日本語 | `ja_JP` |
| 한국어 | `ko_KR` |
| Deutsch | `de_DE` |
| Français | `fr_FR` |
| Español | `es_ES` |
| Português (Brasil) | `pt_BR` |
| Русский | `ru_RU` |
| Italiano | `it_IT` |
| Türkçe | `tr_TR` |
| Polski | `pl_PL` |
| Bahasa Indonesia | `id_ID` |
| Tiếng Việt | `vi_VN` |
| ไทย | `th_TH` |
| العربية | `ar_SA` |
| हिन्दी | `hi_IN` |

The language can be changed immediately in Settings. Arabic automatically
uses a right-to-left layout. Translation sources are stored in
`src/translations`; Qt Linguist compiles and embeds them during the CMake
build.

## Architecture

The application consists of two processes:

- `IPAssistant.exe`: the graphical interface, running with standard user
  privileges.
- `IPAssistantService.exe`: the `IPAssistantSvc` Windows service, responsible
  for network configuration operations that require administrator privileges.

Administrator privileges are required to install the service during setup or
first use. Activating a profile afterward does not repeatedly trigger UAC.

IPv4 addresses and DNS servers are configured through Windows
`netsh interface` commands. Manual IPv6 addresses and routes are configured
through the Windows IP Helper API. Before making changes, the service captures
a snapshot of the current adapter configuration and uses it for rollback if
the operation fails.

## Profile and Adapter Binding

Each profile stores:

- `deviceGuid`: the stable binding identifier.
- `deviceName`: the current display name and a fallback for legacy data.

Legacy `profiles.json` data is upgraded automatically to version 2 at startup.
Before upgrading, the application creates a one-time `profiles.json.bak` in
the same directory. Existing profiles are not deleted.

Windows may assign a new GUID after an adapter driver is uninstalled and
reinstalled. IP Assistant then attempts to rebind the profile by its previous
adapter name. If the name has also changed, edit the profile and select the
adapter again.

## Data Location

Profiles are stored under the application directory:

```text
data/profiles.json
data/profiles.json.bak
```

Interface preferences are stored for the current user through `QSettings` in
the Windows registry. The uninstaller preserves the `data` directory by
default so profiles remain available after an upgrade or reinstall.

## Building

Requirements:

- Windows 10/11 x64
- Qt 6.5 or later (Core, Gui, Widgets, Network, and Test)
- CMake 3.16 or later
- MinGW 13.x or a compatible MSVC toolchain

Example:

```powershell
cmake -S . -B build `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\mingw_64" `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_TESTING=ON

cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

The build invokes Qt Linguist `lrelease` automatically, compiles the 17
non-English `.ts` files to `.qm`, and embeds them in `IPAssistant.exe`. After
adding or changing translatable source text, update a translation with:

```powershell
& "C:\Qt\6.9.1\mingw_64\bin\lupdate.exe" `
  src `
  -no-obsolete `
  -locations none `
  -ts src\translations\ipassistant_zh_CN.ts
```

Main outputs:

```text
build/IPAssistant.exe
build/IPAssistantService.exe
build/ProfileBindingTests.exe
```

Before packaging, deploy the Qt DLLs and plugins into `build` with
`windeployqt`:

```powershell
& "C:\Qt\6.9.1\mingw_64\bin\windeployqt.exe" `
  --release `
  --compiler-runtime `
  build\IPAssistant.exe
```

## Building the Installer

The installer uses Inno Setup 6:

```powershell
ISCC.exe installer\IPAssistant.iss
```

Output:

```text
installer/Output/IPAssistantSetup-1.0.0.exe
```

The installer:

1. Stops the previous `IPAssistantSvc`.
2. Updates the GUI, service, and Qt runtime.
3. Removes the old service registration, then installs and starts the new
   service.
4. Creates a desktop shortcut when selected by the user.

During uninstall, it closes the GUI before stopping and removing the service.

The installer wizard supports English, Simplified Chinese, Japanese, Korean,
German, French, Spanish, Brazilian Portuguese, Russian, Italian, Turkish,
Polish, Thai, and Arabic. The wizard falls back to English on Traditional
Chinese, Indonesian, Vietnamese, and Hindi systems, while the installed
application still uses the corresponding interface language.

## Project Structure

```text
.
├── CMakeLists.txt
├── IPAssistant.pro          # Optional qmake project for the GUI
├── LICENSE                  # MIT License
├── PRIVACY.md               # Privacy Policy
├── SECURITY.md              # Vulnerability reporting policy
├── README.md                # English README
├── README_CN.md             # Simplified Chinese README
├── .github/workflows/       # Automated GitHub Release build
├── .signpath/               # SignPath policies and artifact constraints
├── installer/               # Inno Setup installer script
├── src/
│   ├── core/                # Profiles, persistence, adapters, and core calls
│   ├── service/             # Windows service, network changes, and rollback
│   ├── ui/                  # Main window, profile editor, settings, and About
│   ├── translations/        # 17 non-English Qt Linguist source files
│   └── resources/           # Icons and Windows resources
└── tests/                   # GUID binding and compatibility tests
```

## Important Notes

- Switching network configurations may briefly interrupt the current
  connection.
- Static IP, gateway, and DNS values must be valid for the target network.
- If rollback also fails, the application explicitly reports the rollback
  failure. Check the adapter state in Windows network settings.

## License

Copyright © 2026 Jiugang He.

IP Assistant is licensed under the [MIT License](LICENSE).
