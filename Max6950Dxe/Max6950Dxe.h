/** @file

  Max6950Dxe declares the items necessary to manage the seven segment display
  controller.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MAX6950_DXE_H__
#define __MAX6950_DXE_H__

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Maxim/MAX6950.h>
#include <Maxim/Protocol/Max6950.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SpiIo.h>

extern EFI_GUID gMaximMax6950ProtocolGuid;

#define MAX6950_SIGNATURE        SIGNATURE_32 ('6', '9', '5', '0')

typedef struct _MAX6950
{
  //
  // Structure identification
  //
  UINT32 Signature;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  EFI_HANDLE ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_SPI_IO_PROTOCOL *SpiIo;
  MAXIM_MAX6950_PROTOCOL Max6950Protocol;
  CONST UINT8 *DisplayOrder;
} MAX6950;

#define MAX6950_CONTEXT_FROM_PROTOCOL(protocol)         \
    CR (protocol, MAX6950, Max6950Protocol, MAX6950_SIGNATURE)

EFI_STATUS
EFIAPI
Max6950ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
Max6950Shutdown (
  IN MAXIM_MAX6950_PROTOCOL *Max6950Protocol
  );

EFI_STATUS
EFIAPI
Max6950Startup (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN EFI_HANDLE ControllerHandle
  );

#endif	// __MAX6950_DXE_H__
