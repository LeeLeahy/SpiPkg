/** @file

  This module implements the driver binding and the *name protocols.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "QuarkSpiDxe.h"

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.


  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.


  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.


  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus

                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the

                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the

                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in

                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
SpiHcComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

//
// Driver name table 
//
STATIC EFI_UNICODE_STRING_TABLE mSpiHcDriverNameTable[] = {
  { "eng;en", L"SPI Host Driver" },
  { NULL , NULL }
};

//
// EFI Component Name Protocol
//
STATIC EFI_COMPONENT_NAME_PROTOCOL  mSpiHcComponentName = {
  (EFI_COMPONENT_NAME_GET_DRIVER_NAME) SpiHcComponentNameGetDriverName,
  (EFI_COMPONENT_NAME_GET_CONTROLLER_NAME) SpiHcComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
STATIC EFI_COMPONENT_NAME2_PROTOCOL mSpiHcComponentName2 = {
  SpiHcComponentNameGetDriverName,
  SpiHcComponentNameGetControllerName,
  "en"
};

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.
  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
SpiHcComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mSpiHcDriverNameTable,
           DriverName,
           (BOOLEAN)(This != &mSpiHcComponentName2)
           );
}

/**
  Tests to see if this driver supports a given controller. If a child device is
  provided, it further tests to see if this driver supports creating a handle
  for the specified child device.

  This function checks to see if the driver specified by This supports the
  device specified by ControllerHandle. Drivers will typically use the device
  path attached to ControllerHandle and/or the services from the bus I/O
  abstraction attached to ControllerHandle to determine if the driver supports
  ControllerHandle. This function may be called many times during platform
  initialization. In order to reduce boot times, the tests performed by this
  function must be very small, and take as little time as possible to execute.
  This function must not change the state of any hardware devices, and this
  function must be aware that the device specified by ControllerHandle may
  already be managed by the same driver or a different driver. This function
  must match its calls to AllocatePages() with FreePages(), AllocatePool() with
  FreePool(), and OpenProtocol() with CloseProtocol().  Since ControllerHandle
  may have been previously started by the same driver, if a protocol is already
  in the opened state, then it must not be closed with CloseProtocol(). This is
  required to guarantee the state of ControllerHandle is not modified by this
  function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This
                                   handle must support a protocol interface that
                                   supplies an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a
                                   device path.  This parameter is ignored by
                                   device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter
                                   is not NULL, then the bus driver must
                                   determine if the bus controller specified by
                                   ControllerHandle and the child controller
                                   specified by RemainingDevicePath are both
                                   supported by this bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the
                                   driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed
                                   by the driver specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed
                                   by a different driver or an application that
                                   requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the
                                   driver specified by This.
**/
EFI_STATUS
EFIAPI
SpiHcDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  UINT16 Data[2];
  EFI_PCI_IO_PROTOCOL *PciIo;
  EFI_SPI_HC_PROTOCOL *SpiProtocol;
  EFI_STATUS Status;

  //
  // Open the PCI IO Protocol on ControllerHandle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read the vendor and device IDs from the PCI configuration header
  //
  Status = PciIo->Pci.Read (
                        PciIo,                // This
                        EfiPciIoWidthUint16,  // Width
                        PCI_VENDOR_ID_OFFSET, // Offset
                        2,                    // Count
                        &Data                 // Buffer
                        );
  if (!EFI_ERROR (Status)) {
    //
    // Determine if this is a Quark SPI controller
    //
    if ((Data[0] != INTEL_VENDOR_ID) || (Data[1] != QUARK_SPI_DEVICE_ID)) {
      Status = EFI_UNSUPPORTED;
    } else {
      //
      // Determine if the SPI host controller protocol is already running
      //
      Status = gBS->OpenProtocol (
                      ControllerHandle,
                      &gEfiSpiHcProtocolGuid,
                      (VOID **) &SpiProtocol,
                      This->DriverBindingHandle,
                      ControllerHandle,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (Status == EFI_SUCCESS) {
        Status = EFI_ALREADY_STARTED;
      } else {
        Status = EFI_SUCCESS;
      }
    }
  }

  //
  // Close the PCI IO Protocol
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return Status;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service
  ConnectController().  As a result, much of the error checking on the
  parameters to Start() has been moved into this common boot service.  It is
  legal to call Start() from other locations, but the following calling
  restrictions must be followed, or the system behavior will not be
  deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a
     naturally aligned EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver
     specified by This must have been called with the same calling parameters,
     and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This
                                   handle must support a protocol interface that
                                   supplies an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a
                                   device path.  This parameter is ignored by
                                   device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter
                                   is NULL, then handles for all the children of
                                   Controller are created by this driver.  If
                                   this parameter is not NULL and the first
                                   Device Path Node is not the End of Device
                                   Path Node, then only the handle for the
                                   child device specified by the first Device
                                   Path Node of RemainingDevicePath is created
                                   by this driver.  If the first Device Path
                                   Node of RemainingDevicePath is the End of
                                   Device Path Node, no child handle is created
                                   by this driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a
                                   device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a
                                   lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
SpiHcDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL        *This,
  IN EFI_HANDLE                         ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL           *RemainingDevicePath
  )
{
  EFI_STATUS Status;

  Status = SpiHcStartup (This, ControllerHandle);
  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service
  DisconnectController().  As a result, much of the error checking on the
  parameters to Stop() has been moved into this common boot service. It is legal
  to call Stop() from other locations, but the following calling restrictions
  must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous
     call to this same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in
     this driver's Start() function, and the Start() function must have called
     OpenProtocol() on ControllerHandle with an Attribute of
     EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle
                                must support a bus specific I/O protocol for the
                                driver to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in
                                ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be
                                NULL if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device
                                error.

**/
EFI_STATUS
EFIAPI
SpiHcDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL       *This,
  IN  EFI_HANDLE                        ControllerHandle,
  IN  UINTN                             NumberOfChildren,
  IN  EFI_HANDLE                        *ChildHandleBuffer
  )
{
  EFI_SPI_HC_PROTOCOL *SpiProtocol;
  EFI_STATUS Status;

DEBUG ((EFI_D_ERROR, "SpiHcDriverStop entered\n"));

  //
  // Determine if the driver is already shutdown
  //
DEBUG ((EFI_D_ERROR, "  Calling OpenProtocol(gEfiSpiHcProtocolGuid)\n"));
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSpiHcProtocolGuid,
                  (VOID **) &SpiProtocol,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
DEBUG ((EFI_D_ERROR, "  Status: %r\n", Status));
  if (EFI_ERROR(Status)) {
    return EFI_SUCCESS;
  }

  //
  // Attempt to shutdown the SPI host controller
  //
  Status = SpiHcShutdown (This, SpiProtocol);
DEBUG ((EFI_D_ERROR, "  Calling CloseProtocol(gEfiSpiHcProtocolGuid)\n"));
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
DEBUG ((EFI_D_ERROR, "SpiHcDriverStop exiting, Status: %r\n", Status));
  return Status;
}

STATIC EFI_DRIVER_BINDING_PROTOCOL mSpiHcDriverBinding = {
  SpiHcDriverSupported,
  SpiHcDriverStart,
  SpiHcDriverStop,
  0x10,
  NULL,
  NULL
};

/**
  The entry point for the Quark SPI controller driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
QuarkSpiEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &mSpiHcDriverBinding,
             ImageHandle,
             &mSpiHcComponentName,
             &mSpiHcComponentName2
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
