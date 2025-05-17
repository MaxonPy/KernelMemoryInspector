#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include "memory_region.h"

// получение регионов памяти процесса
NTSTATUS GetProcessMemoryRegions(ULONG pid, PIRP Irp, PIO_STACK_LOCATION stack); 