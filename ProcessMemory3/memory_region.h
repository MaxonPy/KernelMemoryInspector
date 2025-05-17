#pragma once
#include <ntifs.h>
#include <ntddk.h>

// ioctl код для получения информации о памяти
#define IOCTL_MEMORY_INSPECTOR_GET_MEMORY 0x222000

// информация о регионе пам
typedef struct _MemoryRegion {
    PVOID StartAddress;  //начальный адрес региона 
    PVOID EndAddress;    // конечный адр 
    SIZE_T Size;         // разме 
    ULONG AccessRights;  // права доступа 
} MemoryRegion; 