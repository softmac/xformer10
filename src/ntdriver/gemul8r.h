//
//    File:  gemu8er.h
//
//    Description:  Windows NT device driver for accessing
//                  the Gemulator ROM card.  This was ported
//                  from the portio sample in the Windows NT
//                  DDK.
//
//    Who        Date     Description
//    ----------------------------------------------------------
//    dpd      10-28-95   Initial version
//

#include <ntddk.h>
#include <string.h>
#include <devioctl.h>
#include "gemioctl.h"        // Get IOCTL interface definitions

// The Gemulator ROM card defaults to port ID 0x240
// and only has a single port.
#define BASE_PORT       0x240
#define NUMBER_PORTS        1

// NT device name
#define GPD_DEVICE_NAME L"\\Device\\Gemulator0"

// File system device name.   When you execute a CreateFile call to open the
// device, use "\\.\GemulatorDev", or, given C's conversion of \\ to \, use
// "\\\\.\\GemulatorDev"
#define DOS_DEVICE_NAME L"\\DosDevices\\GemulatorDev"


// driver local data structure specific to each device object
typedef struct _LOCAL_DEVICE_INFO {
    ULONG               DeviceType;     // Our private Device Type
    PVOID               PortBase;       // base port address
    PDEVICE_OBJECT      DeviceObject;   // The Gemulator device object.
} LOCAL_DEVICE_INFO, *PLOCAL_DEVICE_INFO;

// Function prototypes

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath);

NTSTATUS GemulatorCreateDevice(IN PWSTR szPrototypeName,
                               IN DEVICE_TYPE DeviceType,
                               IN PDRIVER_OBJECT DriverObject,
                               OUT PDEVICE_OBJECT *ppDevObj);

NTSTATUS GemulatorDispatch(IN PDEVICE_OBJECT pDO, IN PIRP pIrp);

NTSTATUS GemulatorIoctlReadPort(IN PLOCAL_DEVICE_INFO pLDI,
                                IN PIRP pIrp,
                                IN PIO_STACK_LOCATION IrpStack,
                                IN ULONG IoctlCode);

NTSTATUS GemulatorIoctlWritePort(IN PLOCAL_DEVICE_INFO pLDI,
                                 IN PIRP pIrp,
                                 IN PIO_STACK_LOCATION IrpStack,
                                 IN ULONG IoctlCode);

VOID GemulatorUnload(IN PDRIVER_OBJECT DriverObject);


/* End of gemu8er.h */
