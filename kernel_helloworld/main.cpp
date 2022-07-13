//
// Created by zhangsiyu on 3/11/2022.
//

#include "main.h"
#include <ntddk.h>
#include <wdmsec.h>

#define CTL_CODE_DEVICE_SEND \
    (ULONG) CTL_CODE(FILE_DEVICE_UNKNOWN, 0x911, METHOD_BUFFERED, FILE_WRITE_DATA)

#define CTL_CODE_DEVICE_RECV \
    (ULONG) CTL_CODE(FILE_DEVICE_UNKNOWN, 0x912, METHOD_BUFFERED, FILE_READ_DATA)
BOOLEAN GetInput = FALSE;
BOOLEAN IsRight = FALSE;
PDEVICE_OBJECT pdeviceObject;
UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\my_wdktest_device_531rdf135df13");
UNICODE_STRING symbolicLinkName = RTL_CONSTANT_STRING(L"\\??\\wdktest_device_531rdf135df13");
UNICODE_STRING sddl = RTL_CONSTANT_STRING(L"D:P(A;;GA;;;WD)");


// {AE7B9FAA-6695-4A10-9119-BD414C2AB89E}
static const GUID deviceGuid =
        {0xae7b9faa, 0x6695, 0x4a10, {0x91, 0x19, 0xbd, 0x41, 0x4c, 0x2a, 0xb8, 0x9e}};

VOID driverUnload(PDRIVER_OBJECT _driverObject) {
    ASSERT(pdeviceObject != NULL);
    DbgPrint("[WDKTest] DriverObject at %p unloaded\n", _driverObject);
    IoDeleteSymbolicLink(&symbolicLinkName);
    IoDeleteDevice(pdeviceObject);
}

NTSTATUS deviceDispatcher(PDEVICE_OBJECT _deviceObject, PIRP _irp) {
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(_irp);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ret_len = 0;
    UINT8 secrets[] = {
            0x94, 0xaa, 0xad, 0xa7, 0xf3, 0xb4, 0xb0, 0x9c, 0xa8, 0xa6, 0xb1, 0x8d, 0xa6, 0xaf, 0x9c, 0x91, 0xa6, 0x95,
            0xa6, 0xb1, 0x90, 0xaa, 0xad, 0xa4
    };
    do {
        if (_deviceObject != pdeviceObject) break;
        if (irpsp->MajorFunction == IRP_MJ_CREATE || irpsp->MajorFunction == IRP_MJ_CLOSE) break;
        if (irpsp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
            PVOID buffer = _irp->AssociatedIrp.SystemBuffer;
            ULONG input_len = irpsp->Parameters.DeviceIoControl.InputBufferLength;
            ULONG output_len = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
            switch (irpsp->Parameters.DeviceIoControl.IoControlCode) {
                case CTL_CODE_DEVICE_SEND:
                    if (buffer != NULL) {
                        GetInput = TRUE;
                        DbgPrint("[WDKTest] %s - %d\n", buffer, input_len);
                        if (input_len != 25) {
                            IsRight = FALSE;
                        } else {
                            for (int i = 0; i < 24; i++) {
                                if ((((PUINT8) (buffer))[i] ^ 0xc3) != secrets[i]) {
                                    IsRight = FALSE;
                                    break;
                                }
                                if (i == 23) {
                                    IsRight = TRUE;
                                }
                            }
                        }
                    } else {
                        GetInput = FALSE;
                        IsRight = FALSE;
                    }
                    break;
                case CTL_CODE_DEVICE_RECV:
                    DbgPrint("[WDKTest] %d\n", output_len);
                    if (IsRight) {
                        ret_len = 7;
                        strncpy((char *) buffer, "Success", ret_len);
                    } else {
                        ret_len = 5;
                        strncpy((char *) buffer, "Wrong", ret_len);
                    }
                    break;
                default:
                    break;
            }
        }
    } while (false);
    _irp->IoStatus.Information = ret_len;
    _irp->IoStatus.Status = status;
    IoCompleteRequest(_irp, IO_NO_INCREMENT);
    return status;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT _driverObject, PUNICODE_STRING _registryPath) {
    //init ====================================
    DbgPrint("[WDKTest] Hello World\n");
//    DbgBreakPoint();
    DbgPrint("[WDKTest] RegPath: %S\n", _registryPath);
    _driverObject->DriverUnload = driverUnload;
    NTSTATUS status;
    //do my work ====================================
//    status = IoCreateDevice(_driverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE,
//                            &pdeviceObject);
    status = IoCreateDeviceSecure(_driverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE,
                                  &sddl, (LPCGUID) &deviceGuid, &pdeviceObject);
    if (NT_SUCCESS(status)) {
        DbgPrint("[WDKTest] Device created successfully\n");
    } else {
        goto END;
    }
    status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
    if (NT_SUCCESS(status)) {
        DbgPrint("[WDKTest] Device symbolic link name created successfully\n");
    } else {
        IoDeleteDevice(pdeviceObject);
        goto END;
    }
    for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        _driverObject->MajorFunction[i] = deviceDispatcher;
    }
    END:
    return status;
}
