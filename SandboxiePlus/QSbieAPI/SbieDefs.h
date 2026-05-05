#pragma once

#define SANDBOXIE		L"Sandboxie"

#define REQUEST_LEN     4096

#ifndef CONF_LINE_LEN
#define CONF_LINE_LEN   2000
#endif

#ifndef BOXNAME_MAX_LEN
#define BOXNAME_MAX_LEN (CONF_LINE_LEN - 1)
#endif

#define SBIESVC_PORT	L"\\RPC Control\\SbieSvcPort"

//#define SANDBOXIE_INI   L"Sandboxie.ini"

#define SBIEDRV         L"SbieDrv"
#define SBIEDRV_SYS     L"SbieDrv.sys"

#define SBIESVC         L"SbieSvc"
#define SBIESVC_EXE     L"SbieSvc.exe"

#define SBIELDR_EXE     L"SbieLdr.exe"

#define SBIESTART_EXE   L"Start.exe"

#define SBIEMSG_DLL     L"SbieMsg.dll"

#define MAX_REQUEST_LENGTH      (2048 * 1024)