//
// Created by zsy on 3/11/2022.
//

#include "main.h"
#include <ntddk.h>

VOID driverUnload(PDRIVER_OBJECT _driverObject) {
    UNREFERENCED_PARAMETER(_driverObject);
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT _driverObject, PUNICODE_STRING _registryPath) {
    UNREFERENCED_PARAMETER(_driverObject);
    UNREFERENCED_PARAMETER(_registryPath);
    DbgPrint("[WDKTest] Hello World\n");
    return 0;
}
