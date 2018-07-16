#pragma once
#ifndef __COMMON_H__
#define __COMMON_H__

#pragma warning(disable:4005)
#undef MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH 500
#endif // !MAX_PATH

#define PORT_NAME L"\\ProvidencePort"

#pragma warning(disable:4200)
typedef struct _SCAN_DATA_MESSAGE {
	WCHAR FilePath[MAX_PATH + 1];	// Path of the file name to scan.
} SCAN_DATA_MESSAGE, *PSCAN_DATA_MESSAGE;

typedef struct _SCAN_REPLY_MESSAGE {
	BOOLEAN Infected;		// Infection state of the file.
} SCAN_REPLY_MESSAGE, *PSCAN_REPLY_MESSAGE;

#endif // !__COMMON_H__
