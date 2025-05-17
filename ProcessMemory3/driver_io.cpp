#include "driver_io.h"
#include "memory_inspector.h"
// ниже чисто база 
_Use_decl_annotations_
NTSTATUS MemoryInspectorCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
// тут тоже
_Use_decl_annotations_
NTSTATUS MemoryInspectorDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    auto stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    switch (stack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_MEMORY_INSPECTOR_GET_MEMORY: {
        auto pid = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
        status = GetProcessMemoryRegions(pid, Irp, stack);
        break;
    }
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
} 