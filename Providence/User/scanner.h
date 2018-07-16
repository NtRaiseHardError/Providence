#pragma once
#ifndef __SCANNER_H__
#define __SCANNER_H__

#include <stdio.h>

#define DEBUG
#ifdef DEBUG
#define DBG_PRINT(_fmt, ...) printf(_fmt, __VA_ARGS__)
#else
#define DBG_PRINT(_fmt, ...) { NOTHING; }
#endif // DEBUG

typedef struct _SCAN_CALLBACK_USER_DATA {
	BOOL bIsDetected;
} SCAN_CALLBACK_USER_DATA, *PSCAN_CALLBACK_USER_DATA;

BOOL ScanFile(LPCSTR lpFileName, PSCAN_CALLBACK_USER_DATA pScanData);

#endif // !__SCANNER_H__
