#include <ntifs.h>
#include <ntddk.h>
#include "driver_io.h"

// наши устройства
UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\datamem1");
UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\datamem1");

// тута выгрузка драйвера
void MemoryInspectorUnload(_In_ PDRIVER_OBJECT DriverObject) {
    IoDeleteSymbolicLink(&symLink);
    IoDeleteDevice(DriverObject->DeviceObject);
    DbgPrint("Goodbye!");
}
extern "C"
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    DbgPrint("kernel mode сюдааа !!");   
    DriverObject->DriverUnload = MemoryInspectorUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MemoryInspectorCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MemoryInspectorCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MemoryInspectorDeviceControl;

    PDEVICE_OBJECT DeviceObject;
    NTSTATUS status = IoCreateDevice(
        DriverObject,
        0,
        &devName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        DbgPrint("ошибка при создании устройства((( вот с таким номером (0x%08X)\n", status);
        return status;
    }
    
    DbgPrint("победа!! СЮДООО %wZ ", &devName);

    status = IoCreateSymbolicLink(&symLink, &devName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("символическая ссылка не создалась печалька(( (0x%08X)\n", status);
        IoDeleteDevice(DeviceObject);
        return status;
    }
    
    DbgPrint("символическая ссылка создалась  %wZ  WIN", &symLink);
    return STATUS_SUCCESS;
} 