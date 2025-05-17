#pragma once
#include <ntifs.h>
#include <ntddk.h>
// функции для работы с io окак
NTSTATUS MemoryInspectorCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS MemoryInspectorDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp); 