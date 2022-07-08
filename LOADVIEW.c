#include <ntddk.h>
# 간단한 루트킷 
#pragma warning (disable : 4996)

// 1. 프로세스가 실행 또는 종료될 때 호출됩니다.
void ProcessNotifyCallBackRoutine(
	IN HANDLE ParentId,
	IN HANDLE ProcessId,
	IN BOOLEAN Create
)
{
	UNREFERENCED_PARAMETER(ParentId);
	switch (Create)
	{
		// 프로세스가 실행될 때
	case TRUE:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "(ID:0x%X) Process is Creating\n", ProcessId);
		break;
		// 프로세스가 종료될 때
	case FALSE:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "(ID:0x%X) Process is Deleting\n", ProcessId);
		break;
	}
}

// 2. 임의의 드라이버 또는 동적라이브러리파일이 메모리에 상주할 때 호출됩니다.
void LoadImageCallBackRoutine(
	IN PUNICODE_STRING FullImageName,
	IN HANDLE ProcessId,
	IN PIMAGE_INFO ImageInfo
)
{
	WCHAR* pwsName = NULL;
	if (FullImageName == NULL)
	{
		goto exit;
	}

	pwsName = (WCHAR*)ExAllocatePool(NonPagedPool, FullImageName->Length + sizeof(WCHAR)); // 문자열을 담을 메모리를 할당합니다.
	memcpy(pwsName, FullImageName->Buffer, FullImageName->Length);
	pwsName[FullImageName->Length / sizeof(WCHAR)] = 0; // NULL 문자열을 만듭니다.
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "(ID:0x%X) Process, (%ws) is loading\n", ProcessId, pwsName);

	// systemModelImage 값이 0이 아니면 커널드라이버, 0이면 DLL
	if (ImageInfo->SystemModeImage) DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "(ID:0x%X) Process, Driver:(%ws) is loading\n", ProcessId, pwsName);
	else DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "(ID:0x%X) Process, DLL:(%ws) is loading\n", ProcessId, pwsName);

exit:
	if (pwsName) 
	{
		ExFreePool(pwsName);		// 메모리 할당 반납
	}
}

// 3. LOADVEIW 드라이버가 메모리에서 내려올 때 호출됩니다.
void MyDriverUnload(
	IN PDRIVER_OBJECT DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "LOADVIEW Driver Unloading...\n");

	// 드라이버가 메모리에서 내려가기 때문에 모든 콜백함수를 해제해야 합니다.
	PsSetCreateProcessNotifyRoutine(ProcessNotifyCallBackRoutine, TRUE);
	PsRemoveLoadImageNotifyRoutine(LoadImageCallBackRoutine);
}

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "LOADVIEW Drvier Loading...\n");
	// 드라이버 Unload 함수를 등록하지 않으면 절대로 드라이버가 내려가지 않는다.
	DriverObject->DriverUnload = MyDriverUnload;							// LOADVIEW 드라이버가 메모리에서 내려올 때 호출될 함수를 등록하는 작업
	PsSetCreateProcessNotifyRoutine(ProcessNotifyCallBackRoutine, FALSE);	// 프로세스 생성과 종료를 감지하는 콜백함수 등록
	PsSetLoadImageNotifyRoutine(LoadImageCallBackRoutine);					// 드라이버, DLL 파일이 메모리에 상주하는 것을 감지하는 콜백함수를 등록

	return STATUS_SUCCESS;
}
