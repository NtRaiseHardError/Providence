#include <fltKernel.h>
#include "common.h"

#define DEBUG
#ifdef DEBUG
#define DBG_PRINT(_fmt, ...) DbgPrint(_fmt, __VA_ARGS__)
#else
#define DBG_PRINT(_fmt, ...) { NOTHING; }
#endif // DEBUG

#define MAX_PATH 260

DRIVER_INITIALIZE DriverEntry;

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
NTSTATUS Unload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS InstanceSetup(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFileSystemType);
NTSTATUS InstanceQueryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);
NTSTATUS GetFilePath(_In_ PFLT_CALLBACK_DATA Data, _Inout_ PUNICODE_STRING FilePath);
BOOLEAN IsProtectedFileName(PFLT_CALLBACK_DATA Data);
FLT_PREOP_CALLBACK_STATUS PreCreate(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext);
FLT_POSTOP_CALLBACK_STATUS PostCreate(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS PreCleanup(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext);
FLT_PREOP_CALLBACK_STATUS PreSetInformation(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext);
NTSTATUS PortConnect(_In_ PFLT_PORT ClientPort, _In_ PVOID ServerPortCookie, _In_ PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Out_ PVOID *ConnectionPortCookie);
VOID PortDisconnect(_In_ PVOID ConnectionCookie);
NTSTATUS ScanFileInUserMode(_In_ PFLT_CALLBACK_DATA Data, _Out_ PBOOLEAN Infected);

typedef struct _STREAM_HANDLE_CONTEXT {
	BOOLEAN Scan;	// Perform scan or not.
} STREAM_HANDLE_CONTEXT, *PSTREAM_HANDLE_CONTEXT;

typedef struct _GLOBAL_DATA {
	PFLT_FILTER FilterHandle;
	PFLT_PORT ServerPort;
	PFLT_PORT ClientPort;
	PEPROCESS UserProcess;
	UNICODE_STRING DriverPath;
	UNICODE_STRING ApplicationPath;
} GLOBAL_DATA, *PGLOBAL_DATA;

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_CREATE, 0, PreCreate, PostCreate },				// We want to scan files when created. This routine also holds anti-deletion of critical files.
	{ IRP_MJ_CLEANUP, 0, PreCleanup, NULL },					// Rescan the file if the file was created with write access.
	/* { IRP_MJ_WRITE, 0, PreWrite, PostWrite }, */				// Don't need this since we will scan on clean up? 
	{ IRP_MJ_SET_INFORMATION, 0, PreSetInformation, NULL },		// Anti-deletion of critical files.
	{ IRP_MJ_OPERATION_END }
};

CONST FLT_CONTEXT_REGISTRATION Contexts[] = {
	{ FLT_STREAMHANDLE_CONTEXT, 0, NULL, sizeof(STREAM_HANDLE_CONTEXT), 'vorP' },	// For rescanning of files created with write access.
	{ FLT_CONTEXT_END }
};

CONST FLT_REGISTRATION FilterRegistration = {
	sizeof(FLT_REGISTRATION),				// Size
	FLT_REGISTRATION_VERSION,				// Version
	0,										// Flags
	Contexts,								// ContextRegistration
	Callbacks,								// OperationRegistration
	Unload,									// FilterUnloadCallback
	InstanceSetup,							// InstanceSetupCallback
	InstanceQueryTeardown,					// InstanceQueryTeardownCallback
	NULL,									// InstanceTeardownStartCallback
	NULL,									// InstanceTeardownCompleteCallback
	NULL,									// GenerateFileNameCallback
	NULL,									// NormalizeNameComponentCallback
	NULL									// NormalizeContextCleanupCallback
};

GLOBAL_DATA Globals;

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	DBG_PRINT("DriverEntry called.\n");

	RtlZeroMemory(&Globals, sizeof(GLOBAL_DATA));
	RtlInitUnicodeString(&Globals.DriverPath, L"C:\\Users\\Arcadia\\Desktop\\Filter.sys");
	RtlInitUnicodeString(&Globals.ApplicationPath, L"C:\\Users\\Arcadia\\Desktop\\Providence.exe");

	// Register with filter manager.
	NTSTATUS status = FltRegisterFilter(DriverObject, &FilterRegistration, &Globals.FilterHandle);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	// Initialise the port name for communication with the user application.
	UNICODE_STRING usPortName;
	RtlInitUnicodeString(&usPortName, PORT_NAME);

	// Security descriptor for communication port creation.
	PSECURITY_DESCRIPTOR sd = NULL;
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (NT_SUCCESS(status)) {
		// Object handle for communication port creation.
		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &usPortName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, sd);

		// Create a port for the user application to connect for receiving scan requests.
		status = FltCreateCommunicationPort(Globals.FilterHandle, &Globals.ServerPort, &oa, NULL, PortConnect, PortDisconnect, NULL, 1);
		
		FltFreeSecurityDescriptor(sd);

		if (NT_SUCCESS(status)) {
			// Start filtering I/O.
			status = FltStartFiltering(Globals.FilterHandle);
			if (NT_SUCCESS(status)) {
				return STATUS_SUCCESS;
			}

			FltCloseCommunicationPort(Globals.ServerPort);
		}
	}

	FltUnregisterFilter(Globals.FilterHandle);

	DBG_PRINT("Oops! Something went wrong: <0x%08x>.\n", status);

	return status;
}

NTSTATUS Unload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags) {
	UNREFERENCED_PARAMETER(Flags);

	DBG_PRINT("Unload called.\n");

	FltCloseCommunicationPort(Globals.ServerPort);

	FltUnregisterFilter(Globals.FilterHandle);

	return STATUS_SUCCESS;
}

/*
 * Callback for when the minifilter is called to set up for another volume.
 * Reject connection to a networked volume.
 */
NTSTATUS InstanceSetup(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFileSystemType) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeFileSystemType);

	PAGED_CODE();

	// Do not attach to networked file systems.
	if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	return STATUS_SUCCESS;
}

/*
 * Callback for when the volume is requesting manual detach.
 * Probably want to stop any jobs that are active with the volume.
 */
NTSTATUS InstanceQueryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	// Something something here.

	return STATUS_SUCCESS;
}

/*
 * Translate the NT namespace convention to a standard, non-UNC file name.
 *
 * Returns STATUS_SUCCESS on success else, an appropriate error code.
 */
NTSTATUS GetFilePath(_In_ PFLT_CALLBACK_DATA Data, _Inout_ PUNICODE_STRING FilePath) {
	UNICODE_STRING DosName;
	NTSTATUS status = IoVolumeDeviceToDosName(Data->Iopb->TargetFileObject->DeviceObject, &DosName);
	if (NT_SUCCESS(status)) {
		// Set up variables for the UNICODE_STRING to hold the full path name.
		// Max UNICODE_STRING length.
		USHORT MaxLength = DosName.MaximumLength + Data->Iopb->TargetFileObject->FileName.MaximumLength;
		// Allocate a buffer in paged memory.
		PVOID Buffer = ExAllocatePool(PagedPool, MaxLength);
		if (Buffer != NULL) {
			// Initialise UNICODE_STRING members to hold full path name.
			FilePath->Length = 0;
			FilePath->MaximumLength = MaxLength;
			FilePath->Buffer = Buffer;
			// Copy the DOS device drive letter.
			RtlCopyUnicodeString(FilePath, &DosName);
			// Free up this DOS device name since we do not need it anymore.
			//ExFreePool(DosName.Buffer);
			// Append the file name's path.
			RtlAppendUnicodeStringToString(FilePath, &Data->Iopb->TargetFileObject->FileName);

			// Free up the buffer allocated previously in paged memory.
			//ExFreePool(Buffer);
		} else {
			DBG_PRINT("ExAllocatePool failed.");
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}

	return status;
}

/*
 * Compares provided file name in FLT_CALLBACK_DATA to protected, critical files.
 *
 * Returns TRUE if the given file name is the same as a protected file else, FALSE.
 */
BOOLEAN IsProtectedFileName(PFLT_CALLBACK_DATA Data) {
	BOOLEAN ret = FALSE;

	// Attempt to translate volume device name to DOS device name.
	UNICODE_STRING FilePath;
	if (NT_SUCCESS(GetFilePath(Data, &FilePath))) {
		DBG_PRINT("Attempt to delete file \"%wZ\".\n", &FilePath);
		// Compare file name and reject entry they are our files.
		if (RtlCompareUnicodeString(&Globals.DriverPath, &FilePath, FALSE) == 0 || RtlCompareUnicodeString(&Globals.ApplicationPath, &FilePath, FALSE) == 0) {
			DBG_PRINT("File deletion rejected.\n", Data->Iopb->MajorFunction, &FilePath);
			ret = TRUE;
		} else {
			DBG_PRINT("File deletion allowed.\n");
		}

		ExFreePool(FilePath.Buffer);
	} else {
		DBG_PRINT("GetFilePath failed: <0x%08x>.\n");
	}

	return ret;
}

/*
 * We allow file creation for the user-mode application. 
 * We also check for any DELETE_ON_CLOSE options to reject deletion of protected, critical files.
 */
FLT_PREOP_CALLBACK_STATUS PreCreate(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext) {
	UNREFERENCED_PARAMETER(CompletionContext);

	PAGED_CODE();

	FLT_PREOP_CALLBACK_STATUS ret = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	PFLT_FILE_NAME_INFORMATION FileNameInfo = NULL;

	// Allow our user-mode application through.
	if (IoThreadToProcess(Data->Thread) == Globals.UserProcess) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// To deny deletion of our protected files, we must find and compare the names of files selected for deletion.
	// Check if the I/O request is file deletion.
	if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION && (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation ||
		Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformationEx) ||
		(Data->Iopb->MajorFunction == IRP_MJ_CREATE && FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE))) {
		if (FltObjects->FileObject != NULL) {
			NTSTATUS status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);
			if (NT_SUCCESS(status)) {
				// Parse name information.
				FltParseFileNameInformation(FileNameInfo);

				// Compare the received file name with protected files.
				if (IsProtectedFileName(Data)) {
					// Set status to access denied.
					Data->IoStatus.Status = STATUS_ACCESS_DENIED;
					Data->IoStatus.Information = 0;
					// Complete I/O request.
					ret = FLT_PREOP_COMPLETE;
				}
			}
		}
	}

	// Cleanup.
	if (FileNameInfo != NULL) {
		FltReleaseFileNameInformation(FileNameInfo);
	}

	// Pass on the I/O request.
	return ret;
}

/*
 * If successful file creation, we want to scan the file.
 * If the file was created with write access, we want to scan it again on cleanup.
 */
FLT_POSTOP_CALLBACK_STATUS PostCreate(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags) {
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	// Ignore failed creates and symbolic links.
	if (!NT_SUCCESS(Data->IoStatus.Status) || Data->IoStatus.Status == STATUS_REPARSE) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// We don't care about directories.
	BOOLEAN IsDirectory = FALSE;
	NTSTATUS status = FltIsDirectory(FltObjects->FileObject, FltObjects->Instance, &IsDirectory);
	if (NT_SUCCESS(status) && IsDirectory == FALSE) {
		// Scan the file.
		BOOLEAN Infected = FALSE;
		ScanFileInUserMode(Data, &Infected);

		if (Infected == TRUE) {
			DBG_PRINT("File is infected!\n");
		}

		// If the file is opened with write permissions, we want to scan it again in the cleanup.
		if (FltObjects->FileObject->WriteAccess || FlagOn(Data->Iopb->Parameters.Create.Options, GENERIC_WRITE)) {
			DBG_PRINT("File created with write access.\n");
			// Allocate a scan context.
			PSTREAM_HANDLE_CONTEXT Context;
			status = FltAllocateContext(Globals.FilterHandle, FLT_STREAMHANDLE_CONTEXT, sizeof(STREAM_HANDLE_CONTEXT), PagedPool, &Context);
			if (NT_SUCCESS(status)) {
				// Set context to indicate a rescan.
				Context->Scan = TRUE;

				FltSetStreamHandleContext(FltObjects->Instance, FltObjects->FileObject, FLT_SET_CONTEXT_REPLACE_IF_EXISTS, Context, NULL);

				// Decrement the reference count on success or failure.
				FltReleaseContext(Context);
			}
		}
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}

/*
 * Check if we need to scan the file again after file creation with write access. Scan if required.
 */
FLT_PREOP_CALLBACK_STATUS PreCleanup(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext) {
	UNREFERENCED_PARAMETER(CompletionContext);

	PAGED_CODE();

	// Get the scan context to check if we need to rescan the file (after opening with write access).
	PSTREAM_HANDLE_CONTEXT Context;
	NTSTATUS status = FltGetStreamHandleContext(FltObjects->Instance, FltObjects->FileObject, &Context);
	if (NT_SUCCESS(status)) {
		if (Context->Scan) {
			BOOLEAN Infected = FALSE;
			ScanFileInUserMode(Data, &Infected);

			if (Infected) {
				DBG_PRINT("File is infected!\n", &FltObjects->FileObject->FileName);
			}
		}

		FltReleaseContext(Context);
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

/*
 * Detect and reject deletion of protected, critical files.
 */
FLT_PREOP_CALLBACK_STATUS PreSetInformation(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext) {
	UNREFERENCED_PARAMETER(CompletionContext);

	PAGED_CODE();

	FLT_PREOP_CALLBACK_STATUS ret = FLT_PREOP_SUCCESS_NO_CALLBACK;
	PFLT_FILE_NAME_INFORMATION FileNameInfo = NULL;

	// To deny deletion of our protected files, we must find and compare the names of files selected for deletion.
	// Check if the I/O request is file deletion.
	if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION && (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation ||
		Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformationEx) ||
		(Data->Iopb->MajorFunction == IRP_MJ_CREATE && FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE))) {
		if (FltObjects->FileObject != NULL) {

			NTSTATUS status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);
			if (NT_SUCCESS(status)) {
				// Parse name information.
				FltParseFileNameInformation(FileNameInfo);

				// Compare the received file name with protected files.
				if (IsProtectedFileName(Data)) {
					// Set status to access denied.
					Data->IoStatus.Status = STATUS_ACCESS_DENIED;
					Data->IoStatus.Information = 0;
					// Complete I/O request.
					ret = FLT_PREOP_COMPLETE;
				}
			}
		}
	}

	// Cleanup.
	if (FileNameInfo != NULL) {
		FltReleaseFileNameInformation(FileNameInfo);
	}

	// Pass on the I/O request.
	return ret;
}

/*
 * Save the context of the user-mode application.
 * Application's process to bypass file creation.
 * Client port for disconnect routine.
 */
NTSTATUS PortConnect(_In_ PFLT_PORT ClientPort, _In_ PVOID ServerPortCookie, _In_ PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Out_ PVOID *ConnectionPortCookie) {
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ClientPort);
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionPortCookie);

	DBG_PRINT("User application connected.\n");

	// Save the user-mode application's process so that we allow it through IRP_MJ_CREATE.
	Globals.UserProcess = PsGetCurrentProcess();

	// Save the client port so we can close it on disconnect.
	Globals.ClientPort = ClientPort;

	return STATUS_SUCCESS;
}

/*
 * Close the saved client port and unset it.
 */
VOID PortDisconnect( _In_ PVOID ConnectionCookie) {
	UNREFERENCED_PARAMETER(ConnectionCookie);

	PAGED_CODE();

	DBG_PRINT("User application disconnected.\n");

	// Clean up the client's port so we can allow another connection (max: 1)
	FltCloseClientPort(Globals.FilterHandle, &Globals.ClientPort);

	// Unset the user process.
	Globals.UserProcess = NULL;
}

/*
 * Get the file name that is being created and send it to the user-mode application for scanning.
 */
NTSTATUS ScanFileInUserMode(_In_ PFLT_CALLBACK_DATA Data, _Out_ PBOOLEAN Infected) {
	// We need to check if the client did not disconnect.
	if (Globals.ClientPort == NULL) {
		return STATUS_SUCCESS;
	}

	UNICODE_STRING FilePath;
	NTSTATUS status = GetFilePath(Data, &FilePath);
	if (!NT_SUCCESS(status)) {
		DBG_PRINT("Failed to get file path: <0x%08x>.\n", status);
		return status;
	}

	// Check if the buffer can fit inside the message buffer including the null-terminating byte. Ideally we should have a max path length of 32,767.
	if (FilePath.Length > MAX_PATH) {
		DBG_PRINT("File path exceeds max size of %d (%hu).\n", MAX_PATH, FilePath.Length);
		goto cleanup;
	}

	// Allocate some space to store the file path to be sent to the user-mode application.
	//Message = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(&Message->FilePath), 'nacS');
	//if (Message == NULL) {
	//	DBG_PRINT("Failed to allocate data for SCAN_DATA_MESSAGE.\n");
	//	status = STATUS_INSUFFICIENT_RESOURCES;
	//	goto cleanup;
	//}

	SCAN_DATA_MESSAGE Message;

	// Convert the UNICODE_STRING to a WCHAR type.
	// Zero the buffer so we don't need to null-terminate after copy.
	RtlZeroMemory(Message.FilePath, MAX_PATH);
	// We already checked previously if FilePath.Length > MAX_PATH so we can just copy the bytes in.
	RtlCopyMemory(Message.FilePath, FilePath.Buffer, FilePath.Length);

	DBG_PRINT("Sending file path: %ws\n", Message.FilePath);

	// Send the file path to the user-mode application for scanning.
	SCAN_REPLY_MESSAGE Reply;
	ULONG ReplyLength = sizeof(SCAN_REPLY_MESSAGE);
	status = FltSendMessage(Globals.FilterHandle, &Globals.ClientPort, &Message, sizeof(SCAN_DATA_MESSAGE), &Reply, &ReplyLength, NULL);
	if (status != STATUS_SUCCESS) {
		DBG_PRINT("Failed to send message to user-mode application: <0x%08x>.\n", status);
		//goto cleanup;
	}

	// Return infected state.
	*Infected = Reply.Infected;

cleanup:
	if (FilePath.Buffer != NULL) {
		ExFreePool(FilePath.Buffer);
	}

	//if (Message != NULL) {
	//	ExFreePoolWithTag(Message, 'nacS');
	//}

	return status;
}