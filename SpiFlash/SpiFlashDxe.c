/** @file

  This module implements the SPI NOR flash protocol interface.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SpiFlash.h"

EFI_GUID *gFlashIoProtocolGuid = &gEfiSpiNorFlashDriverGuid;
EFI_GUID *gFlashProtocolGuid = &gEfiSpiNorFlashProtocolGuid;
EFI_GUID *gFlashLegacyProtocolGuid = &gEfiLegacySpiFlashProtocolGuid;
VOID *gFlashIoProtocolRegistration;

/**
  Install the a protocol for the driver.

  @param[in]  Handle            Pointer to the handle on which to install
                                the protocol.
  @param[in]  ProtocolGuid      Pointer to the protocol GUID.
  @param[in]  Protocol          Pointer to the protocol structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The protocol was successfully installed
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
**/
EFI_STATUS
EFIAPI
SpiInstallProtocol (
  IN EFI_HANDLE *Handle,
  IN EFI_GUID *ProtocolGuid,
  IN VOID *Protocol
  )
{
  EFI_STATUS Status;

  //
  // Install the protocol
  //
  Status = gBS->InstallProtocolInterface (Handle,
                                          ProtocolGuid,
                                          EFI_NATIVE_INTERFACE,
                                          Protocol);
  return Status;
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
  return gBS->CloseProtocol (Handle, Protocol, AgentHandle, ControllerHandle);
}

/**
  Zero or more SPI NOR flash parts are available

  This routine must be called at or below TPL_NOTIFY.

  Determine if any new SPI NOR flash parts have been installed since the
  last invokation of this routine.

  @param[in]  Event             Event whose notification function is being
                                invoked.
  @param[in]  Context           Pointer to the notification function's context.

**/
VOID
EFIAPI
FlashIoProtocolAvailable (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  EFI_HANDLE ControllerHandle;
  EFI_HANDLE *HandleArray;
  UINTN HandleCount;
  UINTN Index;
  CONST EFI_SPI_IO_PROTOCOL *SpiIo;
  EFI_STATUS Status;

  //
  // Locate the SPI parts in the system
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  gFlashIoProtocolGuid,
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
      // Determine if this driver is already using this protocol
      //
      Status = gBS->OpenProtocol (
                      ControllerHandle,
                      gFlashIoProtocolGuid,
                      (VOID **)&SpiIo,
                      gImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_EXCLUSIVE
                      );
      if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - Flash unable to open SPI IO protocol, Status: %r\n",
                Status));
      } else {
        //
        // Start the flash protocol for this SPI part
        //
        FlashStartup (ControllerHandle, SpiIo);
      }
    }

    //
    // Done with the array
    //
    FreePool (HandleArray);
  }
}

/**
  The entry point for the SPI flash driver.

  @param[in] ImageHandle        The firmware allocated handle for the EFI image.  
  @param[in] SystemTable        A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS           The entry point is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  Not able to create event or registration

**/
EFI_STATUS
EFIAPI
FlashEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_EVENT Event;

  //
  // Create a flash driver instance for each SPI NOR flash part
  //
  Event = EfiCreateProtocolNotifyEvent (gFlashIoProtocolGuid,
                                        TPL_CALLBACK,
                                        FlashIoProtocolAvailable,
                                        (VOID *)gST,
                                        &gFlashIoProtocolRegistration
                                        );
  if (Event == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  return EFI_SUCCESS;
}
