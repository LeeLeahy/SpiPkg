/** @file

  This module declares the Legacy SPI flash protocol interface.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LEGACY_SPI_FLASH_H__
#define __LEGACY_SPI_FLASH_H__

#include <Protocol/SpiNorFlash.h>
#include <Protocol/LegacySpiController.h>

typedef struct _EFI_LEGACY_SPI_FLASH_PROTOCOL EFI_LEGACY_SPI_FLASH_PROTOCOL;

/**
  Set the BIOS base address.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS base address works with the protect range registers to protect
  portions of the SPI NOR flash from erase and write operations.  The BIOS
  calls this API prior to passing control to the OS loader.

  @param[in]  This              Pointer to an EFI_LEGACY_SPI_FLASH_PROTOCOL data
                                structure.
  @param[in]  BiosBaseAddress   The BIOS base address.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The BIOS base address was properly set
  @retval EFI_ACCESS_ERROR      The SPI controller is locked
  @retval EFI_INVALID_PARAMETER BiosBaseAddress > This->MaximumOffset
  @retval EFI_UNSUPPORTED       The BIOS base address was already set
  @retval EFI_UNSUPPORTED       Not a legacy SPI host controller

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_FLASH_PROTOCOL_BIOS_BASE_ADDRESS) (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This,
  IN UINT32 BiosBaseAddress
  );

/**
  Clear the SPI protect range registers.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS uses this routine to set an initial condition on the SPI protect
  range registers.

  @param[in]  This              Pointer to an EFI_LEGACY_SPI_FLASH_PROTOCOL data
                                structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The registers were successfully cleared
  @retval EFI_ACCESS_ERROR      The SPI controller is locked
  @retval EFI_UNSUPPORTED       Not a legacy SPI host controller

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_FLASH_PROTOCOL_CLEAR_SPI_PROTECT) (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This
  );

/**
  Determine if the SPI range is protected.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS uses this routine to verify a range in the SPI is protected.

  @param[in]  This              Pointer to an EFI_LEGACY_SPI_FLASH_PROTOCOL data
                                structure.
  @param[in]  BiosAddress       Address within a 4 KiB block to start
                                protecting.
  @param[in]  BlocksToProtect   The number of 4 KiB blocks to protect.

  @returns  TRUE if the range is protected and FALSE if it is not protected

**/
typedef
BOOLEAN
(EFIAPI *EFI_LEGACY_SPI_FLASH_PROTOCOL_IS_RANGE_PROTECTED) (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This,
  IN UINT32 BiosAddress,
  IN UINT32 BlocksToProtect
  );

/**
  Set the next protect range register.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS sets the protect range register to prevent write and erase
  operations to a portion of the SPI NOR flash device.

  @param[in]  This              Pointer to an EFI_LEGACY_SPI_FLASH_PROTOCOL data
                                structure.
  @param[in]  BiosAddress       Address within a 4 KiB block to start
                                protecting.
  @param[in]  BlocksToProtect   The number of 4 KiB blocks to protect.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The register was successfully updated
  @retval EFI_ACCESS_ERROR      The SPI controller is locked
  @retval EFI_INVALID_PARAMETER BiosAddress < This->BiosBaseAddress
  @retval EFI_INVALID_PARAMETER BlocksToProtect * 4 KiB
                                > This->MaximumRangeBytes
  @retval EFI_INVALID_PARAMETER BiosAddress - This->BiosBaseAddress
                                + (BlocksToProtect * 4 KiB)
                                > This->MaximumRangeBytes
  @retval EFI_OUT_OF_RESOURCES  No protect range register available
  @retval EFI_UNSUPPORTED       Call This->SetBaseAddress because the BIOS base
                                address is not set
  @retval EFI_UNSUPPORTED       Not a legacy SPI host controller

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_FLASH_PROTOCOL_PROTECT_NEXT_RANGE) (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This,
  IN UINT32 BiosAddress,
  IN UINT32 BlocksToProtect
  );

/**
  Lock the SPI controller configuration.

  This routine must be called at or below TPL_NOTIFY.

  This routine locks the SPI controller's configuration so that the software
  is no longer able to update:
  * Prefix table
  * Opcode menu
  * Opcode type table
  * BIOS base address
  * Protect range registers

  @param[in]  This              Pointer to an EFI_LEGACY_SPI_FLASH_PROTOCOL data
                                structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI controller was successfully locked
  @retval EFI_ALREADY_STARTED   The SPI controller was already locked
  @retval EFI_UNSUPPORTED       Not a legacy SPI host controller

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_FLASH_PROTOCOL_LOCK_CONTROLLER) (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This
  );

///
/// The EFI_LEGACY_SPI_FLASH_PROTOCOL extends the EFI_SPI_NOR_FLASH_PROTOCOL
/// with APIs to support the legacy SPI flash controller.
///
struct _EFI_LEGACY_SPI_FLASH_PROTOCOL {
  EFI_SPI_NOR_FLASH_PROTOCOL FlashProtocol;

  ///
  /// Legacy flash (SPI host) controller support
  ///
  EFI_LEGACY_SPI_FLASH_PROTOCOL_BIOS_BASE_ADDRESS BiosBaseAddress;
  EFI_LEGACY_SPI_FLASH_PROTOCOL_CLEAR_SPI_PROTECT ClearSpiProtect;
  EFI_LEGACY_SPI_FLASH_PROTOCOL_IS_RANGE_PROTECTED IsRangeProtected;
  EFI_LEGACY_SPI_FLASH_PROTOCOL_PROTECT_NEXT_RANGE ProtectNextRange;
  EFI_LEGACY_SPI_FLASH_PROTOCOL_LOCK_CONTROLLER LockController;
};

#endif	// __LEGACY_SPI_FLASH_H__
