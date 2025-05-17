#include <ntifs.h>
#include <ntddk.h>

#define IOCTL_MEMORY_INSPECTOR_GET_MEMORY 0x222000
typedef struct _MemoryRegion {
    PVOID StartAddress;  // ��������� ����� ������� ������
    PVOID EndAddress;    // �������� ����� ������� ������
    SIZE_T Size;         // ������ ������� ������
    ULONG AccessRights;  // ����� ������� � ������
} MemoryRegion;


void MemoryInspectorUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS MemoryInspectorCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS MemoryInspectorDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\datamem1");
UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\datamem1"); // ������������� ������ ��� ��������� � ��������

extern "C"
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) { // ����� �����
    UNREFERENCED_PARAMETER(RegistryPath);
    DbgPrint("Hello kernel mode!!");
    DriverObject->DriverUnload = MemoryInspectorUnload; // ������� ��������� ������. 
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MemoryInspectorCreateClose; // ���������� ������������ ��������, ����� ��������� �����-���� ������� 
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MemoryInspectorCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MemoryInspectorDeviceControl;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS status = IoCreateDevice( // ������� ���������� � ������� ����� ����������������� �� ����������������� ������
        DriverObject,
        0,
        &devName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &DeviceObject // ��������� ������������
    );

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to create device (0x%08X)\n", status);
        return status;
    }
    else {
        DbgPrint("Device %wZ created", &devName);
    }

    status = IoCreateSymbolicLink(&symLink, &devName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to create symbolic link (0x%08X)\n", status);
        IoDeleteDevice(DeviceObject);
        return status;
    }
    else {
        DbgPrint("Symbolic link of %wZ is created", &symLink);
    }
    
    return STATUS_SUCCESS;
}

void MemoryInspectorUnload(_In_ PDRIVER_OBJECT DriverObject) { // ������� ��������� ������
    IoDeleteSymbolicLink(&symLink);
    IoDeleteDevice(DriverObject->DeviceObject);
    DbgPrint("Goodbye!");
}



_Use_decl_annotations_
NTSTATUS MemoryInspectorCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) { //
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


_Use_decl_annotations_
NTSTATUS MemoryInspectorDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    auto stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    switch (stack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_MEMORY_INSPECTOR_GET_MEMORY: {
        auto pid = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;

        PEPROCESS process;
        status = PsLookupProcessByProcessId((HANDLE)pid, &process); // ��������� pid � ���������� ��������� �� ��������� PEPROCESS
        if (!NT_SUCCESS(status)) {
            DbgPrint("Failed to find process with PID %d (0x%08X)\n", pid, status);
            break;
        }

        // ��������� ����� ������ ��������
        MemoryRegion regions[1000];
        SIZE_T regionCount = 0;

        KAPC_STATE apcState;
        KeStackAttachProcess(process, &apcState); // ������������ ������� ����� � ��������  
        DbgPrint("������������� � �������� \n");

        MEMORY_BASIC_INFORMATION mbi; // ��������� ���������� � ��������� ������� ��
        PVOID address = nullptr;

        while (NT_SUCCESS(ZwQueryVirtualMemory(
            ZwCurrentProcess(), // ������� ������� � �������� �� ��������������
            address,
            MemoryBasicInformation,
            &mbi,
            sizeof(mbi),
            nullptr
        ))) {
            if (regionCount >= sizeof(regions) / sizeof(regions[0])) {
                break;
            }

            regions[regionCount].StartAddress = mbi.BaseAddress;
            regions[regionCount].EndAddress = (PVOID)((ULONG_PTR)mbi.BaseAddress + mbi.RegionSize);
            regions[regionCount].Size = mbi.RegionSize;
            regions[regionCount].AccessRights = mbi.Protect;
            regionCount++;

            address = (PVOID)((ULONG_PTR)mbi.BaseAddress + mbi.RegionSize);
        }

        KeUnstackDetachProcess(&apcState);  // ��������� ������� ����� �� ��������� ������������ �������� 
        ObDereferenceObject(process);

        // �������� ������ � �������� �����
        if (regionCount > 0 &&
            stack->Parameters.DeviceIoControl.OutputBufferLength >= regionCount * sizeof(MemoryRegion)) {
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, regions,
                regionCount * sizeof(MemoryRegion)); // �������� �� regions � SystemBuffer, 
            Irp->IoStatus.Information = regionCount * sizeof(MemoryRegion);
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
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
