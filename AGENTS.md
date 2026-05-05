# AGENTS.md — Sandboxie

## Repository Overview

Two products in one repo:

- **Sandboxie Classic** (`Sandboxie/`) — C/C++ kernel driver + service + injection DLL + legacy UI. MSBuild + WDK.
- **Sandboxie-Plus** (`SandboxiePlus/`) — Qt-based GUI (SandMan) extending Classic. MSBuild + qmake/jom.
- **SandboxieTools** (`SandboxieTools/`) — Supplementary tools (ImBox, UpdUtil).
- **Installer/** — Inno Setup (Plus) and NSIS (Classic) installer infrastructure.

## Build Prerequisites

- Visual Studio 2022 (CI uses `windows-2022`; toolset v142 for compatibility)
- Windows 10 SDK 10.0.19041
- MFC for v142 build tools
- Windows Driver Kit (WDK) 10.0.19041
- Qt 6.8.3 (version set in `Installer\buildVariables.cmd`)
- jom (downloaded by `SandboxiePlus\install_jom.cmd`)
- Inno Setup 6.3.3 (for Plus installer)
- NSIS 2.5 (for Classic installer)

## Build Commands (x64)

Order matters. Win32 DLLs must build before x64 because the x64 service needs 32-bit WoW64 DLLs.

```cmd
:: 1. Classic core (Win32 DLLs first, then x64 all, then x64 driver)
msbuild /t:build Sandboxie\SandboxDll.sln /p:Configuration="SbieRelease" /p:Platform=Win32 -maxcpucount:8
msbuild /t:build Sandboxie\Sandbox.sln /p:Configuration="SbieRelease" /p:Platform=x64 -maxcpucount:8
msbuild /t:build Sandboxie\SandboxDrv.sln /p:Configuration="SbieRelease" /p:Platform=x64 -maxcpucount:8

:: 2. Qt framework setup
SandboxiePlus\install_qt.cmd x64
SandboxiePlus\install_jom.cmd

:: 3. Plus UI (qmake_plus builds UGlobalHotkey -> QtSingleApp -> MiscHelpers -> QSbieAPI -> SandMan)
SandboxiePlus\qmake_plus.cmd x64 build_qt6

:: 4. Shell extension
msbuild /t:restore,build -p:RestorePackagesConfig=true SandboxiePlus\SbieShell\SbieShell.sln /p:Configuration="Release" /p:Platform=x64

:: 5. Tools
msbuild /t:build SandboxieTools\SandboxieTools.sln /p:Configuration="Release" /p:Platform=x64 -maxcpucount:8

:: 6. Merge/packaging
Installer\fix_qt5_languages.cmd x64 build_qt6
Installer\get_openssl.cmd
Installer\get_7zip.cmd
Installer\copy_build.cmd x64 build_qt6
```

## Build Configurations

| Solution | Debug | Release |
|---|---|---|
| Sandboxie Classic (`Sandboxie/`) | `SbieDebug` | `SbieRelease` |
| Sandboxie Plus (`SandboxiePlus/`) | `Debug` | `Release` |
| SandboxieTools | `Debug` | `Release` |

Platforms: Win32, x64, ARM64, ARM64EC (Classic supports all four).

## Critical Build Constraints

1. **Win32 DLLs before x64**: The x64 `SbieSvc` loads 32-bit `SbieDll`/`SbieSvc` for WoW64. Always build `SandboxDll.sln` for Win32 first.
2. **qmake_plus.cmd strict order**: Each library must succeed before the next. The script checks output DLL existence and aborts on failure.
3. **Qt5 vs Qt6**: `SandMan.qc.pro` = Qt5, `SandMan-Qt6.qc.pro` = Qt6. The `qmake_plus.cmd` auto-selects based on `qt_version` prefix.
4. **Private Qt headers**: `qmake_plus.cmd` copies private QtCore headers from Qt's include subdirectory — required for build.
5. **ARM64 builds**: Need both x64 Qt (for qmake host tools) and ARM64 Qt. The script generates `my_target_qt.conf` because the GitHub Actions runner's `target_qt.conf` is broken.

## Build Output Locations

- Classic: `Sandboxie/Bin/{Platform}/{Configuration}/`
- Plus Qt: `SandboxiePlus/Bin/{Platform}/Release/`
- Plus MSBuild: `SandboxiePlus/{Platform}/Release/`
- Tools: `SandboxieTools/{Platform}/Release/`
- Installer staging: `Installer/SbiePlus_x64/`, `Installer/SbiePlus_a64/`
- Debug merge: `SandboxiePlus/x64/Debug/` (via `MergeDbg.cmd`)

## Version Files

- `Sandboxie/common/my_version.h` — Classic version (currently 5.72.5)
- `SandboxiePlus/version.h` — Plus version (currently 1.17.5)
- `Installer/buildVariables.cmd` — Central dependency versions (Qt, OpenSSL)

## Testing

**No automated test suite exists.** `TestCI.cmd` is a CI environment diagnostic, not a test runner. Verification is manual: enable test signing (`bcdedit /set testsigning on`), install, and run. CodeQL provides static analysis.

## Code Style

- **C/C++**: tabs (`.editorconfig` enforced)
- **YAML/RC/XML/props**: spaces, indent 2
- **.sln files**: UTF-8-BOM, tabs (required or VS won't load)
- **Templates.ini**: UTF-8-BOM, spaces, indent 4
- **No linter/formatter/typecheck** beyond `.editorconfig` and codespell CI
- **Compiler**: TreatWarningsAsError=true, StdCall, static CRT, no exceptions, no buffer security check

## Key Entry Points

| Component | Source |
|---|---|
| SandMan.exe (Plus GUI) | `SandboxiePlus/SandMan/main.cpp` |
| SbieSvc.exe (service) | `Sandboxie/core/svc/main.cpp` |
| SbieDrv.sys (driver) | `Sandboxie/core/drv/driver.c` |
| SbieDll.dll (injection) | `Sandboxie/core/dll/dllmain.c` (117 source files) |
| Templates.ini | `Sandboxie/install/Templates.ini` (4500+ lines) |

## Localization

Three separate systems:
1. Classic messages: `Sandboxie/msgs/Text-{Language}-{LCID}.txt` → compiled by `Parse.vcxproj` into `SbieMsg.dll`
2. Plus UI: `SandboxiePlus/SandMan/sandman_*.ts` (Qt `.ts` files) → `.qm` by `lrelease`
3. Troubleshooting: `SandboxiePlus/SandMan/Troubleshooting/lang_*.json` + JS scripts

## Compiler Settings (Classic)

Preprocessor: `VISUAL_STUDIO_BUILD`, `UNICODE`, `WINVER=0x0502`, `_WIN32_WINNT=0x0502`
Runtime: static (`MultiThreaded`), Exceptions disabled, Buffer security disabled

## CI Workflows

- `main.yml` — Primary build (x64 + ARM64, Qt6). 45min timeout. Triggers on push/PR to master/experimental.
- `codeql.yml` — Weekly + PR static analysis
- `codespell.yml` — Spell checking with custom dictionary
- `lupdate.yml` — Nightly Qt translation string sync

## External Dependencies (downloaded at build time)

- Qt 6.8.3 from `xanasoft/qt-builds`
- OpenSSL 3.4.0 from `xanasoft/openssl-builds`
- 7-Zip from `DavidXanatos/7z`
- Detours (vendored in `Sandboxie/common/Detours/`)
- NuGet (only for SbieShellExt)

## Debug Workflow

`MergeDbg.cmd` copies Classic + Tools debug outputs into `SandboxiePlus/x64/Debug/` for combined debugging since Classic and Plus are separate solutions.
