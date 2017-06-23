/** @file

  This module implements the SPI bus control.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SpiBus.h"

EFI_SPI_CONFIGURATION_PROTOCOL *gSpiConfigurationProtocol;
VOID *gSpiConfigurationProtocolRegistration;
VOID *gSpiHcProtocolRegistration;
EFI_GUID *gLegacySpiControllerProtocolGuid = &gEfiLegacySpiControllerProtocolGuid;
EFI_GUID *gSpiConfigurationProtocolGuid = &gEfiSpiConfigurationProtocolGuid;
EFI_GUID *gSpiHcProtocolGuid = &gEfiSpiHcProtocolGuid;
EFI_GUID gSpiBusLayerGuid =
{0x94edabab, 0x63e5, 0x4c63, {0x9b, 0xfa, 0x42, 0x85, 0x1d, 0xb7, 0x97, 0x1b}};

/**
  Raises a task's priority level and returns its previous level.

  @param[in]  NewTpl          The new task priority level.

  @return Previous task priority level

**/
EFI_TPL
EFIAPI
SpiRaiseTpl(
  IN EFI_TPL      NewTpl
  )
{
  return gBS->RaiseTPL(NewTpl);
}

/**
  Restores a task's priority level to its previous value.

  @param[in]  OldTpl          The previous task priority level to restore.

**/
VOID
EFIAPI
SpiRestoreTpl(
  IN EFI_TPL      OldTpl
  )
{
  gBS->RestoreTPL(OldTpl);
}

/**
  Install the SPI bus layer protocol for the driver.

  @param[in]  Handle            Pointer to the handle on which to install
                                the protocol.
  @param[in]  Protocol          Pointer to the protocol structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI host controller was started successfully
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
**/
EFI_STATUS
EFIAPI
SpiInstallBusProtocol (
  IN EFI_HANDLE *Handle,
  IN VOID *Protocol
  )
{
  EFI_STATUS Status;

  //
  // Install the SPI bus layer tag
  //
  Status = gBS->InstallProtocolInterface (
                   Handle,
                   &gSpiBusLayerGuid,
                   EFI_NATIVE_INTERFACE,
                   Protocol
                   );
  return Status;
}

/**
  Uninstall the SPI bus layer protocol for the driver.

  @param[in]  Handle            Pointer to the handle on which to uninstall
                                the protocol.
  @param[in]  Protocol          Pointer to the protocol structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI host controller was started successfully
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
**/
EFI_STATUS
EFIAPI
SpiUninstallBusProtocol (
  IN EFI_HANDLE *Handle,
  IN VOID *Protocol
  )
{
  EFI_STATUS Status;

  //
  // Uninstall the SPI bus layer tag
  //
  Status = gBS->UninstallProtocolInterface (
                   Handle,
                   &gSpiBusLayerGuid,
                   Protocol
                   );
  return Status;
}

/**
  Install the SPI IO protocol for a SPI part.

  @param[in]  Handle            Pointer to the handle on which to uninstall
                                the protocol.
  @param[in]  SpiiO             A pointer to the SPI_IO instance.
  @param[in]  SpiPeripheral     A pointer to the EFI_SPI_PERIPHERAL instance.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI host controller was started successfully

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
**/
EFI_STATUS
EFIAPI
SpiInstallIoProtocol (
  IN EFI_HANDLE *Handle,
  IN SPI_IO *SpiIo,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral
  )
{
  EFI_STATUS Status;

  //
  // Create a child handle for the SPI peripheral
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  Handle,
                  SpiPeripheral->SpiPeripheralDriverGuid,
                  &SpiIo->SpiIoProtocol,
                  &gEfiDevicePathProtocolGuid,
                  SpiIo->DevicePath,
                  NULL,
                  NULL
                  );
  return Status;
}

/**
  Connects one or more drivers to a controller.

  @param[in]  ControllerHandle      The handle of the controller to which driver(s) are to be connected.
  @param[in]  DriverImageHandle     A pointer to an ordered list handles that support the
                                    EFI_DRIVER_BINDING_PROTOCOL.
  @param[in]  RemainingDevicePath   A pointer to the device path that specifies a child of the
                                    controller specified by ControllerHandle.
  @param[in]  Recursive             If TRUE, then ConnectController() is called recursively
                                    until the entire tree of controllers below the controller specified
                                    by ControllerHandle have been created. If FALSE, then
                                    the tree of controllers is only expanded one level.

  @retval EFI_SUCCESS           1) One or more drivers were connected to ControllerHandle.
                                2) No drivers were connected to ControllerHandle, but
                                RemainingDevicePath is not NULL, and it is an End Device
                                Path Node.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_NOT_FOUND         1) There are no EFI_DRIVER_BINDING_PROTOCOL instances
                                present in the system.
                                2) No drivers were connected to ControllerHandle.
  @retval EFI_SECURITY_VIOLATION 
                                The user has no permission to start UEFI device drivers on the device path 
                                associated with the ControllerHandle or specified by the RemainingDevicePath.
**/
VOID
EFIAPI
SpiConnectController(
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    *DriverImageHandle,   OPTIONAL
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath, OPTIONAL
  IN  BOOLEAN                       Recursive
  )
{
  gBS->ConnectController (ControllerHandle,
                          DriverImageHandle,
                          RemainingDevicePath,
                          Recursive);
}

/**
  Connect SPI bus to the host controller.

  @param[in]  SpiBus            A pointer to the SPI_BUS instance.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_NOT_FOUND         A SPI bus connection to the host controller was
                                not found.
  @retval EFI_OUT_OF_RESOURCES  
**/
EFI_STATUS
EFIAPI
SpiBusConnectHc (
  IN SPI_BUS *SpiBus
  )
{
  CONST EFI_SPI_BUS *BusConfig;
  CONST EFI_DEVICE_PATH_PROTOCOL *BusNode;
  CHAR16 *DevicePath;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DevicePathToText;
  EFI_DEVICE_PATH_PROTOCOL *HcNode;
  UINT32 Index;
  UINT32 NodeLength;
  EFI_STATUS Status;

  //
  // Locate the SPI host controller's device path
  //
  SpiBus->DevicePath = DevicePathFromHandle(SpiBus->ControllerHandle);
  if (SpiBus->DevicePath == NULL) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - SpiBus failed to locate SPI HC device path!\n"));
    Status = EFI_NOT_FOUND;
    goto Failure;
  }

  //
  // Locate the board's SPI configuration protocol
  //
  DevicePath = NULL;
  DevicePathToText = NULL;

  //
  // Display the SPI host controller's path
  //
  if (FeaturePcdGet (PcdDisplaySpiHcDevicePath)) {
    //
    // Locate the device path to path protocol
    //
    Status = gBS->LocateProtocol (
                    &gEfiDevicePathToTextProtocolGuid,
                    NULL,
                    (VOID **)&DevicePathToText
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR,
            "ERROR - SpiBus failed to locate Device Path To Text protocol!\n"));
      goto Failure;
    } else {
      DevicePath = DevicePathToText->ConvertDevicePathToText(
                      SpiBus->DevicePath,
                      FALSE,
                      FALSE
                      );
      if (DevicePath == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((EFI_D_ERROR,
                "ERROR - SpiBus failed to display SPI HC device path!\n"));
        goto Failure;
      } else {
        DEBUG ((EFI_D_INFO, "SPI Host Controller\n"));
        DEBUG ((EFI_D_INFO, "  | Path: %s\n", DevicePath));
        FreePool (DevicePath);
      }
    }
  }
  
  //
  // Iterate through the SPI busses on the board
  //
  for (Index = 0; Index < gSpiConfigurationProtocol->BusCount; Index++) {
    BusConfig = gSpiConfigurationProtocol->BusList[Index];
    HcNode = SpiBus->DevicePath;
    BusNode = BusConfig->ControllerPath;

    //
    // Determine if this host controller is connected to this SPI bus
    //
    while (TRUE) {
      //
      // Determine if the device paths match
      //
      NodeLength = DevicePathNodeLength (HcNode);
      if (CompareMem (HcNode, BusNode, NodeLength) != 0) {
        break;
      }
      if (IsDevicePathEndType (HcNode)) {
        //
        // The matching host controller was found
        //
        SpiBus->BusConfig = BusConfig;
        if (BusConfig->FriendlyName != NULL) {
          DEBUG ((EFI_D_INFO, "  | Name: %s\n", BusConfig->FriendlyName));
        }
        return EFI_SUCCESS;
      }
      HcNode = NextDevicePathNode (HcNode);
      BusNode = NextDevicePathNode (BusNode);
    }
  }

  //
  // No bus matches this SPI host controller
  //
  Status = EFI_NOT_FOUND;

Failure:
  return Status;
}

/**
  Queries a handle to determine if it supports a specified protocol. If the protocol is supported by the
  handle, it opens the protocol on behalf of the calling agent.

  @param[in]   Handle           The handle for the protocol interface that is being opened.
  @param[in]   Protocol         The published unique identifier of the protocol.
  @param[out]  Interface        Supplies the address where a pointer to the corresponding Protocol
                                Interface is returned.
  @param[in]   AgentHandle      The handle of the agent that is opening the protocol interface
                                specified by Protocol and Interface.
  @param[in]   ControllerHandle If the agent that is opening a protocol is a driver that follows the
                                UEFI Driver Model, then this parameter is the controller handle
                                that requires the protocol interface. If the agent does not follow
                                the UEFI Driver Model, then this parameter is optional and may
                                be NULL.
  @param[in]   Attributes       The open mode of the protocol interface specified by Handle
                                and Protocol.

  @retval EFI_SUCCESS           An item was added to the open list for the protocol interface, and the
                                protocol interface was returned in Interface.
  @retval EFI_UNSUPPORTED       Handle does not support Protocol.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_ACCESS_DENIED     Required attributes can't be supported in current environment.
  @retval EFI_ALREADY_STARTED   Item on the open list already has requierd attributes whose agent
                                handle is the same as AgentHandle.

**/
EFI_STATUS
EFIAPI
SpiOpenProtocol (
  IN  EFI_HANDLE                Handle,
  IN  EFI_GUID                  *Protocol,
  OUT VOID                      **Interface, OPTIONAL
  IN  EFI_HANDLE                AgentHandle,
  IN  EFI_HANDLE                ControllerHandle,
  IN  UINT32                    Attributes
  )
{
  return gBS->OpenProtocol(Handle,
                           Protocol,
                           Interface,
                           AgentHandle,
                           ControllerHandle,
                           Attributes);
}

/**
  Closes a protocol on a handle that was opened using OpenProtocol().

  @param[in]  Handle            The handle for the protocol interface that was previously opened
                                with OpenProtocol(), and is now being closed.
  @param[in]  Protocol          The published unique identifier of the protocol.
  @param[in]  AgentHandle       The handle of the agent that is closing the protocol interface.
  @param[in]  ControllerHandle  If the agent that opened a protocol is a driver that follows the
                                UEFI Driver Model, then this parameter is the controller handle
                                that required the protocol interface.

  @retval EFI_SUCCESS           The protocol instance was closed.
  @retval EFI_INVALID_PARAMETER 1) Handle is NULL.
                                2) AgentHandle is NULL.
                                3) ControllerHandle is not NULL and ControllerHandle is not a valid EFI_HANDLE.
                                4) Protocol is NULL.
  @retval EFI_NOT_FOUND         1) Handle does not support the protocol specified by Protocol.
                                2) The protocol interface specified by Handle and Protocol is not
                                   currently open by AgentHandle and ControllerHandle.

**/
EFI_STATUS
EFIAPI
SpiCloseProtocol(
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  IN EFI_HANDLE               AgentHandle,
  IN EFI_HANDLE               ControllerHandle
  )
{
  return gBS->CloseProtocol(Handle, Protocol, AgentHandle, ControllerHandle);
}

/**
  Zero or more SPI host controllers are available

  This routine must be called at or below TPL_NOTIFY.

  Determine if any new SPI host controllers have been installed since the
  last invokation of this routine.

  @param[in]  Event             Event whose notification function is being
                                invoked.
  @param[in]  Context           Pointer to the notification function's context.

**/
VOID
EFIAPI
SpiBusSpiHcProtocolAvailable (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  EFI_HANDLE ControllerHandle;
  EFI_HANDLE *HandleArray;
  UINTN HandleCount;
  UINTN Index;
  CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol;
  EFI_STATUS Status;

  //
  // Locate the SPI host controllers in the system
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  gSpiHcProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleArray
                  );
  if (!EFI_ERROR(Status)) {
    //
    // Walk the handle buffer
    //
    for (Index = 0; Index < HandleCount; Index++) {
      ControllerHandle = HandleArray[Index];

      //
      // Determine if the SPI controller is already running
      //
      Status = gBS->OpenProtocol (
                      ControllerHandle,
                      gSpiHcProtocolGuid,
                      (VOID **)&SpiHcProtocol,
                      gImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_EXCLUSIVE
                      );
      if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - SpiBus unable to open SPI HC protocol, Status: %r\n",
                Status));
      } else {
        //
        // Start the SPI bus layer on this SPI host controller
        //
        SpiBusStartup (ControllerHandle, SpiHcProtocol);
      }
    }

    //
    // Done with the array
    //
    FreePool (HandleArray);
  }
}

/**
  Get the board layer's SPI configuration database

  This routine must be called at or below TPL_NOTIFY.

  Save the pointer to the EFI_SPI_CONFIGURATION_PROTOCOL which is the data
  structure describing the board layer's SPI configuration database.

  @param[in]  Event             Event whose notification function is being
                                invoked.
  @param[in]  Context           Pointer to the notification function's context.

**/
VOID
EFIAPI
SpiBusSpiConfigurationProtocolAvailable (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  EFI_EVENT SpiHcEvent;
  EFI_STATUS Status;

  //
  // Locate the board layer's SPI configuration database
  //
  Status = gBS->LocateProtocol (
                  gSpiConfigurationProtocolGuid,
                  NULL,
                  (VOID *)&gSpiConfigurationProtocol
                  );
  if (EFI_ERROR(Status)) {
    return;
  }
  DEBUG ((EFI_D_INFO,
          "SpiBus: Board layer's SPI configuration database is available\n"));

  //
  // Start a SPI bus for each SPI host controller that is installed
  //
  SpiHcEvent = EfiCreateProtocolNotifyEvent (gSpiHcProtocolGuid,
                                             TPL_CALLBACK,
                                             SpiBusSpiHcProtocolAvailable,
                                             (VOID *)gST,
                                             &gSpiHcProtocolRegistration
                                             );
  ASSERT (SpiHcEvent != NULL);

  //
  // Check to see if any SPI host controllers were installed prior to the board
  // layer's SPI configuration database
  //
  gBS->SignalEvent (SpiHcEvent);

  //
  // Prevent further notifications
  //
  gBS->CloseEvent (Event);
}

/**
  The entry point for the SPI bus driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SpiBusEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_EVENT Event;

  //
  // Wait until the board layer's SPI configuration database is available
  //
  Event = EfiCreateProtocolNotifyEvent (
                  gSpiConfigurationProtocolGuid,
                  TPL_CALLBACK,
                  SpiBusSpiConfigurationProtocolAvailable,
                  (VOID *)SystemTable,
                  &gSpiConfigurationProtocolRegistration
                  );
  ASSERT (Event != NULL);
  return EFI_SUCCESS;
}
