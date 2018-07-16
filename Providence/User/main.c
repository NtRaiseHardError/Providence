#include <Windows.h>
#include <stdlib.h>
#include <fltUser.h>
#include "common.h"
#include "scanner.h"

#define NUM_SCAN_THREADS 10

typedef struct _PORT_DATA_THREAD_CONTEXT {
	HANDLE hFilterPort;
	HANDLE hCompletionPort;
} PORT_DATA_THREAD_CONTEXT, *PPORT_DATA_THREAD_CONTEXT;

typedef struct _USER_SCAN_DATA_MESSAGE {
	FILTER_MESSAGE_HEADER header;
	SCAN_DATA_MESSAGE data;
	OVERLAPPED ovlp;
} USER_SCAN_DATA_MESSAGE, *PUSER_SCAN_DATA_MESSAGE;

// Pack it or size calculation in FilterReplyMessage/FltSendMessage will complain about buffer overflow.
#pragma pack(1)
typedef struct _USER_SCAN_REPLY_MESSAGE {
	FILTER_REPLY_HEADER header;
	SCAN_REPLY_MESSAGE reply;
} USER_SCAN_REPLY_MESSAGE, *PUSER_SCAN_REPLY_MESSAGE;

VOID ScanThread(PPORT_DATA_THREAD_CONTEXT pContext) {
	DWORD dwBytes;
	ULONG_PTR ulKey;
	LPOVERLAPPED pOvlp;
	PUSER_SCAN_DATA_MESSAGE message = NULL;

	while (TRUE) {
		// Block until we get a message from the driver's FltSendMessage.
		BOOL bRet = GetQueuedCompletionStatus(pContext->hCompletionPort, &dwBytes, &ulKey, &pOvlp, INFINITE);
		if (bRet == FALSE) {
			DBG_PRINT("GetQueuedCompletionStatus error: <0x%08x>.\n", GetLastError());
			break;
		}

		// Since the overlapped parameter is passed in as an address from FilterGetMessage, 
		// we can retrieve the message structure via the CONTAINING_RECORD macro.
		message = CONTAINING_RECORD(pOvlp, USER_SCAN_DATA_MESSAGE, ovlp);

		USER_SCAN_REPLY_MESSAGE reply;
		reply.header.Status = 0;
		// Set the reply's message ID to correspond with the original message's.
		reply.header.MessageId = message->header.MessageId;

		// Scan the file and set the infection status.
		SCAN_CALLBACK_USER_DATA scanData;
		scanData.bIsDetected = FALSE;

		// Convert wide string to ASCII.
		CHAR szFilePath[MAX_PATH + 1];
		ZeroMemory(szFilePath, MAX_PATH + 1);
		sprintf_s(szFilePath, MAX_PATH, "%ws", message->data.FilePath);

		if (ScanFile(szFilePath, &scanData) == FALSE) {
			DBG_PRINT("ScanFile failed.\n");
		}
		reply.reply.Infected = (BOOLEAN)scanData.bIsDetected;

		if (reply.reply.Infected) {
			DBG_PRINT("File is infected!\n");
		}

		HRESULT hr = FilterReplyMessage(pContext->hFilterPort, (PFILTER_REPLY_HEADER)&reply, sizeof(USER_SCAN_REPLY_MESSAGE));
		if (!SUCCEEDED(hr)) {
			DBG_PRINT("Failed to reply to message: <0x%08x>.\n", hr);
			//break;
		}

		// Reset the overlapped data for another asynchronous message.
		ZeroMemory(&message->ovlp, sizeof(OVERLAPPED));

		// Request for new message asynchronously.
		hr = FilterGetMessage(pContext->hFilterPort, &message->header, sizeof(FILTER_MESSAGE_HEADER) + sizeof(SCAN_DATA_MESSAGE), &message->ovlp);
		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)) {
				DBG_PRINT("Driver port disconnected.\n");
			} else {
				DBG_PRINT("Failed to get new message: <0x%08x>.\n", hr);
			}
			break;
		}
	}

	// Something wrong with this, I don't know why it BSODs. :'(
	HeapFree(GetProcessHeap(), 0, message);
}

int __cdecl main(int argc, char *argv[]) {
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	// Seed infection status.
	srand((__rdtsc() << 32) >> 32);

	PORT_DATA_THREAD_CONTEXT ctx = { 0 };

	// Connect to the driver's port.
	HANDLE hPort;
	HRESULT hr;
	while (IS_ERROR(hr = FilterConnectCommunicationPort(PORT_NAME, 0, NULL, 0, NULL, &hPort)));
	if (IS_ERROR(hr)) {
		DBG_PRINT("Failed to connect to communication port: <0x%08x>.\n", hr);
		return 1;
	}

	HANDLE hCompletion = CreateIoCompletionPort(hPort, NULL, 0, NUM_SCAN_THREADS);
	if (hCompletion == NULL) {
		DBG_PRINT("Failed to create completion port: <0x%08x>.\n", GetLastError());
		goto cleanup;
	}

	DBG_PRINT("Filter port: 0x%p; Completion port: 0x%p\n", hPort, hCompletion);

	ctx.hFilterPort = hPort;
	ctx.hCompletionPort = hCompletion;

	DBG_PRINT("Creating threads.\n");
	// Create threads to wait for messages from driver.
	HANDLE hThreadPool[NUM_SCAN_THREADS];
	for (size_t i = 0; i < NUM_SCAN_THREADS; i++) {
		hThreadPool[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ScanThread, &ctx, 0, NULL);

		if (hThreadPool[i] == NULL) {
			DBG_PRINT("Failed to create thread %d: <0x%08x>.\n", i, GetLastError());
			goto cleanup;
		}

		// We free this in the thread worker.
		PUSER_SCAN_DATA_MESSAGE message = (PUSER_SCAN_DATA_MESSAGE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(USER_SCAN_DATA_MESSAGE));
		if (message == NULL) {
			DBG_PRINT("Failed to create message in heap: <0X%08X>.\n", GetLastError());
			goto cleanup;
		}

		ZeroMemory(&message->ovlp, sizeof(OVERLAPPED));
			
		// Get messages from driver asynchronously.
		hr = FilterGetMessage(hPort, &message->header, sizeof(FILTER_MESSAGE_HEADER) + sizeof(SCAN_DATA_MESSAGE), &message->ovlp);
		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			DBG_PRINT("FitlerGetMessage failed overlapped: <0x%08x>.\n", GetLastError());
			goto cleanup;
		}
	}

	DBG_PRINT("Waiting for threads to complete.\n");

	// Block until all threads have completed.
	WaitForMultipleObjects(NUM_SCAN_THREADS, hThreadPool, TRUE, INFINITE);

cleanup:
	for (size_t i = 0; i < NUM_SCAN_THREADS; i++) {
		if (hThreadPool[i] != NULL) {
			CloseHandle(hThreadPool[i]);
		}
	}

	if (hCompletion != NULL) {
		CloseHandle(hCompletion);
	}

	if (hPort != NULL) {
		CloseHandle(hPort);
	}

	return 0;
}