/** @file

  This module declares the legacy SPI host controller protocol.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Support a SPI data transaction between the SPI controller and a SPI chip.

  @par Revision Reference:
  This protocol is from PI Version 1.6.

**/

#ifndef __LEGACY_SPI_CONTROLLER_H__
#define __LEGACY_SPI_CONTROLLER_H__

typedef struct _EFI_LEGACY_SPI_CONTROLLER_PROTOCOL
                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL;

/**

  Set the erase block opcode.

  This routine must be called at or below TPL_NOTIFY.

  The menu table contains SPI transaction opcodes which are accessible after
  the legacy SPI flash controller's configuration is locked.  The board layer
  specifies the erase block size for the SPI NOR flash part.  The SPI NOR flash
  peripheral driver selects the erase block opcode which matches the erase
  block size and uses this API to load the opcode into the opcode menu table.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  EraseBlockOpcode  Erase block opcode to be placed into the opcode
                                menu table.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The opcode menu table was updated
  @retval EFI_ACCESS_ERROR      The SPI controller is locked

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_ERASE_BLOCK_OPCODE) (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT8 EraseBlockOpcode
  );

/**
  Set the write status prefix opcode.

  This routine must be called at or below TPL_NOTIFY.

  The prefix table contains SPI transaction write prefix opcodes which are
  accessible after the legacy SPI flash controller's configuration is locked.
  The board layer specifies the write status prefix opcode for the SPI NOR
  flash part.  The SPI NOR flash peripheral driver uses this API to load the
  opcode into the prefix table.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  WriteStatusPrefix Prefix opcode for the write status command.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The prefix table was updated
  @retval EFI_ACCESS_ERROR      The SPI controller is locked

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_WRITE_STATUS_PREFIX) (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT8 WriteStatusPrefix
  );

/**
  Set the BIOS base address.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS base address works with the protect range registers to protect
  portions of the SPI NOR flash from erase and write operations.  The BIOS
  calls this API prior to passing control to the OS loader.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  BiosBaseAddress   The BIOS base address.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The BIOS base address was properly set
  @retval EFI_ACCESS_ERROR      The SPI controller is locked
  @retval EFI_INVALID_PARAMETER The BIOS base address is greater than
                                This->MaximumOffset
  @retval EFI_UNSUPPORTED       The BIOS base address was already set

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_BIOS_BASE_ADDRESS) (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT32 BiosBaseAddress
  );

/**
  Clear the SPI protect range registers.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS uses this routine to set an initial condition on the SPI protect
  range registers.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The registers were successfully cleared
  @retval EFI_ACCESS_ERROR      The SPI controller is locked

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_CLEAR_SPI_PROTECT) (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This
  );

/**
  Determine if the SPI range is protected.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS uses this routine to verify a range in the SPI is protected.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  BiosAddress       Address within a 4 KiB block to start
                                protecting.
  @param[in]  BlocksToProtect   The number of 4 KiB blocks to protect.

  @returns  TRUE if the range is protected and FALSE if it is not protected

**/
typedef
BOOLEAN
(EFIAPI *EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_IS_RANGE_PROTECTED) (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT32 BiosAddress,
  IN UINT32 BlocksToProtect
  );

/**
  Set the next protect range register.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS sets the protect range register to prevent write and erase
  operations to a portion of the SPI NOR flash device.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
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
**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_PROTECT_NEXT_RANGE) (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
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

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI controller was successfully locked
  @retval EFI_ALREADY_STARTED   The SPI controller was already locked

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_LOCK_CONTROLLER) (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This
  );

///
/// Support extra features of the legacy SPI flash controller
///
struct _EFI_LEGACY_SPI_CONTROLLER_PROTOCOL {
  ///
  /// Maximum offset from the BIOS base address that is able to be protected.
  ///
  UINT32 MaximumOffset;

  ///
  /// Maximum number of bytes that can be protected by one range register.
  ///
  UINT32 MaximumRangeBytes;

  ///
  /// The number of registers available for protecting the BIOS.
  ///
  UINT32 RangeRegisterCount;

  EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_ERASE_BLOCK_OPCODE  EraseBlockOpcode;
  EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_WRITE_STATUS_PREFIX WriteStatusPrefix;
  EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_BIOS_BASE_ADDRESS   BiosBaseAddress;
  EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_CLEAR_SPI_PROTECT   ClearSpiProtect;
  EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_IS_RANGE_PROTECTED  IsRangeProtected;
  EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_PROTECT_NEXT_RANGE  ProtectNextRange;
  EFI_LEGACY_SPI_CONTROLLER_PROTOCOL_LOCK_CONTROLLER     LockController;
};

#endif  //  __LEGACY_SPI_CONTROLLER_H__
