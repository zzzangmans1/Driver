#include <ntddk.h>
/*
	NT Legacy Style 드라이버는 설치 파일이 아니기 때문에 .inf 파일 삭제
*/
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DrvierObject,
	IN PUNICODE_STRING RegistryPath)
{
	// UNREFERENCED_PARAMETER 컴파일 할 때 파라미터를 사용하지 않으면 오류가 발생하는데 그 오류방지 차원
	UNREFERENCED_PARAMETER(DrvierObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Hello World\n");
	return STATUS_UNSUCCESSFUL;
}
