//
//    File:  gemu8er.c
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

#include "gemul8r.h"
#include "stdlib.h"


/*
 *    Function:  DriverEntry
 *
 *    Description:  This routine is the entry point for the driver.  It 
 *                  is responsible for setting the dispatch entry points 
 *                  in the driver object and creating  the device object.  
 *                  Any resources such as ports, interrupts and DMA channels 
 *                  used must be reported.  A symbolic link must be created 
 *                  between the device name and an entry in \DosDevices in 
 *                  order to allow Win32 applications to open the device.
 *
 *    Arguments:
 *       DriverObject - Pointer to driver object created by the system.
 *
 *    Returns:
 *       STATUS_SUCCESS if the driver initialized correctly, otherwise an error
 *                      indicating the reason for failure.
 *
 */

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
   ULONG PortBase;                 // Port location, in NT's address form.
   PHYSICAL_ADDRESS PortAddress;

   PLOCAL_DEVICE_INFO pLocalInfo;  // Device extension:
                                   //      local information for each device.
   NTSTATUS Status;
   PDEVICE_OBJECT DeviceObject;

   CM_RESOURCE_LIST ResourceList;  // Resource usage list to report to system
   BOOLEAN ResourceConflict;       // This is set true if our I/O ports
                                   //      conflict with another driver

   // Try to retrieve base I/O port and range from the Parameters key of our
   // entry in the Registry.
   // If there isn't anything specified then use the values compiled into
   // this driver.
   {
      static  WCHAR               SubKeyString[] = L"\\Parameters";
      UNICODE_STRING              paramPath;
      RTL_QUERY_REGISTRY_TABLE    paramTable[3];
      ULONG                       DefaultBase = BASE_PORT;

      // Since the registry path parameter is a "counted" UNICODE string, it
      // might not be zero terminated.  For a very short time allocate memory
      // to hold the registry path as well as the Parameters key name zero
      // terminated so that we can use it to delve into the registry.
      paramPath.MaximumLength = RegistryPath->Length + sizeof(SubKeyString);
      paramPath.Buffer = ExAllocatePool(PagedPool, paramPath.MaximumLength);
      if (paramPath.Buffer != NULL) {
         RtlMoveMemory(paramPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);
         RtlMoveMemory(&paramPath.Buffer[RegistryPath->Length / 2], SubKeyString, sizeof(SubKeyString));
         paramPath.Length = paramPath.MaximumLength - 2;

         RtlZeroMemory(&paramTable[0], sizeof(paramTable));
         paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
         paramTable[0].Name = L"IoPortAddress";
         paramTable[0].EntryContext = &PortBase;
         paramTable[0].DefaultType = REG_DWORD;
         paramTable[0].DefaultData = &DefaultBase;
         paramTable[0].DefaultLength = sizeof(ULONG);

         if (!NT_SUCCESS(RtlQueryRegistryValues(
             RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
             paramPath.Buffer, &paramTable[0], NULL, NULL))) {
            PortBase = DefaultBase;
         }
         ExFreePool(paramPath.Buffer);
      }
   }

   PortAddress.LowPart  = PortBase;
   PortAddress.HighPart = 0;

   // Register resource usage (ports)
   // This ensures that there isn't a conflict between this driver and
   // a previously loaded one or a future loaded one.

   RtlZeroMemory((PVOID) &ResourceList, sizeof(ResourceList));
   ResourceList.Count = 1;
   ResourceList.List[0].InterfaceType = Isa;
   ResourceList.List[0].PartialResourceList.Count = 1;
   ResourceList.List[0].PartialResourceList.PartialDescriptors[0].Type = CmResourceTypePort;
   ResourceList.List[0].PartialResourceList.PartialDescriptors[0].ShareDisposition = CmResourceShareDriverExclusive;
   ResourceList.List[0].PartialResourceList.PartialDescriptors[0].Flags = CM_RESOURCE_PORT_IO;
   ResourceList.List[0].PartialResourceList.PartialDescriptors[0].u.Port.Start = PortAddress;
   ResourceList.List[0].PartialResourceList.PartialDescriptors[0].u.Port.Length = NUMBER_PORTS;

   // Report our resource usage and detect conflicts
   Status = IoReportResourceUsage(NULL, DriverObject, &ResourceList, 
                                  sizeof(ResourceList), NULL, NULL, 0,
                                  FALSE, &ResourceConflict);
   if (ResourceConflict)
      Status = STATUS_DEVICE_CONFIGURATION_ERROR;

   if (!NT_SUCCESS(Status)) {
       KdPrint( ("Resource reporting problem %8X", Status) );
       return (Status);
   }

   // Initialize the driver object dispatch table.
   // NT sends requests to these routines.
   DriverObject->MajorFunction[IRP_MJ_CREATE]          = GemulatorDispatch;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]           = GemulatorDispatch;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = GemulatorDispatch;
   DriverObject->DriverUnload                          = GemulatorUnload;

   // Create our device.
   Status = GemulatorCreateDevice(GPD_DEVICE_NAME, GPD_TYPE, DriverObject, &DeviceObject);
   if (NT_SUCCESS(Status)) {
      PHYSICAL_ADDRESS MappedAddress;
      ULONG MemType;

       // Convert the IO port address into a form NT likes.
       MemType = 1;                        // located in IO space
       HalTranslateBusAddress(Isa, 0, PortAddress, &MemType, &MappedAddress);

       // Initialize the local driver info for each device object.
       pLocalInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;

       pLocalInfo->PortBase = (PVOID) MappedAddress.LowPart;

       pLocalInfo->DeviceObject    = DeviceObject;
       pLocalInfo->DeviceType      = GPD_TYPE;
   }
   else {
      // Error creating device - release resources
      RtlZeroMemory((PVOID) &ResourceList, sizeof(ResourceList));
    
      // Unreport our resource usage
      Status = IoReportResourceUsage(NULL, DriverObject, &ResourceList, 
                                     sizeof(ResourceList), NULL, NULL,
                                     0, FALSE, &ResourceConflict);
   }

   return (Status);
} /* End of DriverEntry() */


/*
 *    Function:  GemulatorCreateDevice
 *
 *    Description: This routine creates the device object and the symbolic 
 *                 link in \DosDevices.  Ideally a name derived from a 
 *                 "Prototype", with a number appended at the end should be 
 *                 used.  For simplicity, just use the fixed name defined
 *                 in the include file.  This means that only one device can 
 *                 be created.  A symbolic link must be created between the 
 *                 device name and an entry in \DosDevices in order to allow 
 *                 Win32 applications to open the device.
 *
 *    Arguments:
 *       PrototypeName - Name base, # WOULD be appended to this.
 *       DeviceType    - Type of device to create
 *       DriverObject  - Pointer to driver object created by the system.
 *       ppDevObj      - Pointer to place to store pointer to created device object
 *
 *    Returns:
 *       STATUS_SUCCESS if the device and link are created correctly, otherwise
 *                      an error indicating the reason for failure.
 *
 */

NTSTATUS
GemulatorCreateDevice(IN PWSTR PrototypeName, IN DEVICE_TYPE DeviceType,
                      IN PDRIVER_OBJECT DriverObject, OUT PDEVICE_OBJECT *ppDevObj)
{
    NTSTATUS Status;                        // Status of utility calls
    UNICODE_STRING NtDeviceName;
    UNICODE_STRING Win32DeviceName;

    // Get UNICODE name for device.
    RtlInitUnicodeString(&NtDeviceName, PrototypeName);

   // Create it.
    Status = IoCreateDevice(DriverObject, sizeof(LOCAL_DEVICE_INFO),
                            &NtDeviceName, DeviceType, 0, FALSE,
                            ppDevObj);
    if (!NT_SUCCESS(Status))
        return (Status);           // Give up if create failed.

    // Clear local device info memory
    RtlZeroMemory((*ppDevObj)->DeviceExtension, sizeof(LOCAL_DEVICE_INFO));

    // Set up the rest of the device info
    //  These are used for IRP_MJ_READ and IRP_MJ_WRITE which we don't use
    //    
    //  (*ppDevObj)->Flags |= DO_BUFFERED_IO;
    //  (*ppDevObj)->AlignmentRequirement = FILE_BYTE_ALIGNMENT;

    RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);
    Status = IoCreateSymbolicLink(&Win32DeviceName, &NtDeviceName);
    if (!NT_SUCCESS(Status))
       IoDeleteDevice(*ppDevObj);

    return (Status);
} /* End of GemulatorCreateDevice() */



/*
 *    Function:  GemulatorDispatch
 *
 *    Description:  This routine is the dispatch handler for the driver.  It 
 *                  is responsible for processing the IRPs.
 *
 *    Arguments:
 *       pDO  - Pointer to device object.
 *       pIrp - Pointer to the current IRP.
 *
 *    Returns:
 *       STATUS_SUCCESS if the IRP was processed successfully, otherwise an 
 *                      error indicating the reason for failure.
 *
 */

NTSTATUS
GemulatorDispatch(IN PDEVICE_OBJECT pDO, IN PIRP pIrp)
{
   PLOCAL_DEVICE_INFO pLDI;
   PIO_STACK_LOCATION pIrpStack;
   NTSTATUS Status;

   //  Initialize the irp info field.
   //      This is used to return the number of bytes transfered.
   pIrp->IoStatus.Information = 0;
   pLDI = (PLOCAL_DEVICE_INFO)pDO->DeviceExtension;    // Get local info struct

   pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

   //  Set default return status
   Status = STATUS_NOT_IMPLEMENTED;

   // Dispatch based on major fcn code.
   switch (pIrpStack->MajorFunction) {
      case IRP_MJ_CREATE:
      case IRP_MJ_CLOSE:
         // We don't need any special processing on open/close so we'll
         // just return success.
         Status = STATUS_SUCCESS;
         break;

      case IRP_MJ_DEVICE_CONTROL:
         //  Dispatch on IOCTL
         switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
            case IOCTL_GPD_READ_PORT_UCHAR:
            case IOCTL_GPD_READ_PORT_USHORT:
            case IOCTL_GPD_READ_PORT_ULONG:
               Status = GemulatorIoctlReadPort(pLDI, pIrp, pIrpStack,
                                               pIrpStack->Parameters.DeviceIoControl.IoControlCode);
                break;

            case IOCTL_GPD_WRITE_PORT_UCHAR:
            case IOCTL_GPD_WRITE_PORT_USHORT:
            case IOCTL_GPD_WRITE_PORT_ULONG:
                Status = GemulatorIoctlWritePort(pLDI, pIrp, pIrpStack,
                                                 pIrpStack->Parameters.DeviceIoControl.IoControlCode);
                break;
         }
         break;
   }

   // We're done with I/O request.  Record the status of the I/O action.
   pIrp->IoStatus.Status = Status;

   // Don't boost priority when returning since this took little time.
   IoCompleteRequest(pIrp, IO_NO_INCREMENT );

   return (Status);
} /* End of GemulatorDispatch() */



/*
 *    Function: GemulatorIoctlReadPort
 *
 *    Description:  This routine processes the IOCTLs which read from the ports.
 *
 *    Arguments:
 *       pLDI        - our local device data
 *       pIrp        - IO request packet
 *       IrpStack    - The current stack location
 *       IoctlCode   - The ioctl code from the IRP
 *
 *    Returns:
 *       STATUS_SUCCESS           -- OK
 *
 *       STATUS_INVALID_PARAMETER -- The buffer sent to the driver
 *                                   was too small to contain the
 *                                   port, or the buffer which
 *                                   would be sent back to the driver
 *                                   was not a multiple of the data size.
 *
 */

NTSTATUS
GemulatorIoctlReadPort(IN PLOCAL_DEVICE_INFO pLDI, IN PIRP pIrp,
                       IN PIO_STACK_LOCATION IrpStack,
                       IN ULONG IoctlCode)
{
                                // NOTE:  Use METHOD_BUFFERED ioctls.
   PULONG pIOBuffer;           // Pointer to transfer buffer
   ULONG OutBufferSize;        // Max data that caller can accept.
   ULONG PortAddress;          // Port # to do I/O in

   // Size of buffer for data to be sent to application
   OutBufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

   // NT copies inbuf here before entry and copies this to outbuf after
   // return, for METHOD_BUFFERED IOCTL's.
   pIOBuffer     = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

   // BUGBUG:  need some way to validate the output buffer size
   // return (STATUS_INVALID_PARAMETER);

   PortAddress = *(PULONG)pIOBuffer;
   if (PortAddress == 0)
       PortAddress = (ULONG)pLDI->PortBase;

   READ_PORT_BUFFER_UCHAR((PUCHAR) PortAddress, (PUCHAR) pIOBuffer, (ULONG) OutBufferSize);

   // Indicate # of bytes read
   pIrp->IoStatus.Information = OutBufferSize;

   return (STATUS_SUCCESS);
} /* End of GemulatorIoctlReadPort() */



/*
 *    Function:  GemulatorIoctlWritePort
 *
 *    Description: This routine processes the IOCTLs which write to the ports.
 *
 *    Arguments:
 *       pLDI        - our local device data
 *       pIrp        - IO request packet
 *       IrpStack    - The current stack location
 *       IoctlCode   - The ioctl code from the IRP
 *
 *    Returns:
 *       STATUS_SUCCESS           -- OK
 *
 *       STATUS_INVALID_PARAMETER -- The buffer sent to the driver
 *                                   was too small to contain the
 *                                   port, or the buffer which
 *                                   would be sent back to the driver
 *                                   was not a multiple of the data size.
 *
 *       STATUS_ACCESS_VIOLATION  -- An illegal port number was given.
 *
 */

NTSTATUS
GemulatorIoctlWritePort(IN PLOCAL_DEVICE_INFO pLDI, IN PIRP pIrp, 
                        IN PIO_STACK_LOCATION IrpStack, IN ULONG IoctlCode)
{
                               // NOTE:  Use METHOD_BUFFERED ioctls.
   PULONG pIOBuffer;           // Pointer to transfer buffer
                               //      (treated as array of longs).
   ULONG InBufferSize ;        // Amount of data avail. from caller.
   ULONG OutBufferSize ;       // Max data that caller can accept.
   ULONG DataBufferSize;
   ULONG PortAddress;          // Port # to do I/O in

   // Size of buffer containing data from application
   InBufferSize  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

   // Size of buffer for data to be sent to application
   OutBufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

   // NT copies inbuf here before entry and copies this to outbuf after return,
   // for METHOD_BUFFERED IOCTL's.
   pIOBuffer     = (PULONG) pIrp->AssociatedIrp.SystemBuffer;

   // We don't return any data on a write port.
   pIrp->IoStatus.Information = 0;
   
   // Check to ensure input buffer is big enough to hold a port number as well
   // as the data to write.
   //
   // The relative port # is a ULONG, and the data is the type appropriate to
   // the IOCTL.
   //

   switch (IoctlCode) {
      default:                   // There isn't really any default but 
         /* FALL THRU */         // this will quiet the compiler.

      case IOCTL_GPD_WRITE_PORT_UCHAR:
         DataBufferSize = sizeof(UCHAR);
         break;

      case IOCTL_GPD_WRITE_PORT_USHORT:
         DataBufferSize = sizeof(USHORT);
         break;

      case IOCTL_GPD_WRITE_PORT_ULONG:
         DataBufferSize = sizeof(ULONG);
         break;
   }

   if (InBufferSize < (sizeof(ULONG) + DataBufferSize))
       return (STATUS_INVALID_PARAMETER);

   PortAddress = *(PULONG)pIOBuffer;
   if (PortAddress == 0)
       PortAddress = (ULONG)pLDI->PortBase;

   WRITE_PORT_UCHAR((PUCHAR) PortAddress, *(PUCHAR) pIOBuffer);
   

   return (STATUS_SUCCESS);
} /* End of GemulatorIoctlWritePort() */



/*
 *    Function:  GemulatorUnload
 *
 *    Description:  This routine prepares our driver to be unloaded.
 *                  It is responsible for freeing all resources allocated 
 *                  by DriverEntry as well as any allocated while the 
 *                  driver was running.  The symbolic link must be deleted 
 *                  as well.
 *
 *    Arguments:
 *       DriverObject - Pointer to driver object created by the system.
 *
 *    Returns:  Nothing
 */

VOID
GemulatorUnload(PDRIVER_OBJECT DriverObject)
{
   PLOCAL_DEVICE_INFO pLDI;
   CM_RESOURCE_LIST NullResourceList;
   BOOLEAN ResourceConflict;
   UNICODE_STRING Win32DeviceName;

   // Find our global data
   pLDI = (PLOCAL_DEVICE_INFO) DriverObject->DeviceObject->DeviceExtension;

   // Report we're not using any hardware.  If we don't do this
   // then we'll conflict with ourselves (!) on the next load
   RtlZeroMemory((PVOID)&NullResourceList, sizeof(NullResourceList));
   IoReportResourceUsage(NULL, DriverObject, &NullResourceList,
                         sizeof(ULONG), NULL, NULL, 0, FALSE,
                         &ResourceConflict);

   // Assume all handles are closed down.
   // Delete the things we allocated - devices, symbolic links
   RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);
   IoDeleteSymbolicLink(&Win32DeviceName);
   IoDeleteDevice(pLDI->DeviceObject);
} /* End of GemulatorUnload() */

/* End of gemu8er.c */

