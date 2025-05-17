#include "memory_inspector.h"

NTSTATUS GetProcessMemoryRegions(ULONG pid, PIRP Irp, PIO_STACK_LOCATION stack) {
    PEPROCESS process;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)pid, &process);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to find process with PID %d (0x%08X)\n", pid, status);
        return status;
    }
    // получение списка регионов памяти
    MemoryRegion regions[1000];
    SIZE_T regionCount = 0;
    KAPC_STATE apcState;
    KeStackAttachProcess(process, &apcState);
    DbgPrint("Присоединились к процессу\n");
    MEMORY_BASIC_INFORMATION mbi;
    PVOID address = nullptr;

    while (NT_SUCCESS(ZwQueryVirtualMemory(
        ZwCurrentProcess(),
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

    KeUnstackDetachProcess(&apcState); //читайте доку лень объяснять
    //https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-keunstackdetachprocess
    ObDereferenceObject(process);
    // здесб копирование данных в выходной буфер
    if (regionCount > 0 &&
        stack->Parameters.DeviceIoControl.OutputBufferLength >= regionCount * sizeof(MemoryRegion)) {
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, regions,
            regionCount * sizeof(MemoryRegion));
        Irp->IoStatus.Information = regionCount * sizeof(MemoryRegion);
        return STATUS_SUCCESS;
    }

    return STATUS_BUFFER_TOO_SMALL;
} 