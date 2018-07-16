#include <Windows.h>
#include <stdlib.h>
#include "scanner.h"

BOOL ScanFile(LPCSTR lpFileName, PSCAN_CALLBACK_USER_DATA pScanData) {
	DBG_PRINT("Scanning file: %s\n", lpFileName);

	// Simulate infection status (1/100 infected rate).
	pScanData->bIsDetected = rand() % 100 > 0 ? FALSE : TRUE;

	return TRUE;
}