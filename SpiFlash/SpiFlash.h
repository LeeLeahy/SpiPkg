/** @file

  SpiFlashDxe declares the items necessary to manage SPI flash devices.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include <Uefi.h>
#include <Library/AsciiDump.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/LegacySpiFlash.h>
#include <Protocol/SpiIo.h>
#include <Protocol/SpiNorFlash.h>

extern EFI_GUID gSpiNorFlashProtocolGuid;

#define FLASH_SIGNATURE         SIGNATURE_32 ('F', 'l', 's', 'h')

typedef struct _FLASH
{
  //
  // Structure identification
  //
  UINT32 Signature;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  EFI_HANDLE ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  CONST EFI_SPI_IO_PROTOCOL *SpiIo;
  CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA *FlashConfig;
  EFI_LEGACY_SPI_FLASH_PROTOCOL LegacySpiFlash;
} FLASH;

#define FLASH_CONTEXT_FROM_PROTOCOL(protocol)         \
    CR (protocol, FLASH, LegacySpiFlash.FlashProtocol, FLASH_SIGNATURE)

VOID
EFIAPI
FlashDisplayManufactureName (
  IN UINT8 Manufacture
  );

EFI_STATUS
EFIAPI
SpiInstallProtocol (
  IN EFI_HANDLE *Handle,
  IN EFI_GUID *ProtocolGuid,
  IN VOID *Protocol
  );

EFI_STATUS
EFIAPI
FlashStartup (
  IN EFI_HANDLE ControllerHandle,
  IN CONST EFI_SPI_IO_PROTOCOL *SpiIo
  );

EFI_STATUS
EFIAPI
SpiCloseProtocol(
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  IN EFI_HANDLE               AgentHandle,
  IN EFI_HANDLE               ControllerHandle
  );

extern EFI_GUID *gFlashIoProtocolGuid;
extern EFI_GUID *gFlashProtocolGuid;
extern EFI_GUID *gFlashLegacyProtocolGuid;

#endif	// __SPI_FLASH_H__
