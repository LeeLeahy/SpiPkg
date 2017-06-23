/** @file

  QuarkLegacySpi.h declares the items necessary to manage the legacy SPI
  controller.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __QUARK_LEGACY_SPI_H__
#define __QUARK_LEGACY_SPI_H__

#include <Uefi.h>
#include <IndustryStandard/Pci.h>
#include <Intel/LegacySpiConfig.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/LegacySpiController.h>
#include <Protocol/SpiHc.h>
#include <Protocol/SpiNorFlash.h>
#include <Library/UefiBootServicesTableLib.h>

#define INTEL_VENDOR_ID         0x8086
#define LEGACY_BRIDGE_ID        0x095e

//
// BC - BIOS control
//
#define BC                      0xd8
#define BC_PFE                  0x00000100      // Prefetch enable
#define BC_SMM_WPD              0x00000020      // SMM write protect disable
#define BC_CD                   0x00000004      // Cache disable
#define BC_LE                   0x00000002      // Lock enable
#define BC_WPD                  0x00000001      // Write protect disable

//
// RCBA - Root complex base address
//        Datasheet 21.3.15
//
#define RCBA                    0xf0
#define RCBA_BA                 0xffffc000
#define RCBA_EN                 0x00000001

//
// SPISTS - SPI status
//          Datasheet 21.7.4.1
//
#define SPISTS                  0x3020
#define SPISTS_CLD              0x8000  // SPI configuration lock down
#define SPISTS_BA               0x0008  // Blocked access
#define SPISTS_CD               0x0004  // Cycle done
#define SPISTS_CIP              0x0001  // Cycle in progress

//
// SPICTL - SPI control
//          Datasheet 21.7.4.2
//
#define SPICTL                  0x3022
#define SPICTL_SMIEN            0x8000  // SMI B enable
#define SPICTL_DC               0x4000  // Data cycle
#define SPICTL_DBCNT            0x3f00  // Data byte count - 1
#define SPICTL_DBCNT_SHIFT      8
#define SPICTL_COPTR            0x0070  // Cycle opcode pointer
#define SPICTL_COPTR_SHIFT      4
#define SPICTL_SOPTR            0x0008  // Sequence prefix opcode pointer
#define SPICTL_ACS              0x0004  // Atomic cycle sequence
#define SPICTL_CG               0x0002  // Cycle go
#define SPICTL_AR               0x0001  // Access Request

//
// SPIADDR - SPI address
//           Datasheet 21.7.4.3
//
#define SPIADDR                 0x3024
#define SPIADDR_CSC             0xc0000000  // Chip select control
#define SPIADDR_CA              0x00ffffff  // Cycle address

//
// SPIDx_y - SPI data
//           Datasheet 21.7.4.4 - 21.7.4.19
//
#define SPID0_1                 0x3028  // Lower 32 bits
#define SPID0_2                 0x302c  // Upper 32 bits
#define SPID1_1                 0x3030  // Lower 32 bits
#define SPID1_2                 0x3034  // Upper 32 bits
#define SPID2_1                 0x3038  // Lower 32 bits
#define SPID2_2                 0x303c  // Upper 32 bits
#define SPID3_1                 0x3040  // Lower 32 bits
#define SPID3_2                 0x3044  // Upper 32 bits
#define SPID4_1                 0x3048  // Lower 32 bits
#define SPID4_2                 0x304c  // Upper 32 bits
#define SPID5_1                 0x3050  // Lower 32 bits
#define SPID5_2                 0x3054  // Upper 32 bits
#define SPID6_1                 0x3058  // Lower 32 bits
#define SPID6_2                 0x305c  // Upper 32 bits
#define SPID7_1                 0x3060  // Lower 32 bits
#define SPID7_2                 0x3064  // Upper 32 bits

//
// BBAR - BIOS Base Address
//        Datasheet 21.7.4.20
//
#define BBAR                    0x3070
#define BBAR_BOSF               0x00ffff00  // Bottom of system flash

//
// PREOP - Prefix opcode configuration
//         Datasheet 21.7.4.21
//
#define PREOP                   0x3074
#define PREOP_PO1               0xff00  // Prefix opcode 1
#define PREOP_PO2               0x00ff  // Prefix opcode 2

//
// OPTYPE - Opcode type configuration
//          Datasheet 21.7.4.21
//
#define OPTYPE                  0x3076
#define OPTYPE_OT7              0xc000  // Opcode type 7
#define OPTYPE_OT6              0x3000  // Opcode type 6
#define OPTYPE_OT5              0x0c00  // Opcode type 5
#define OPTYPE_OT4              0x0300  // Opcode type 4
#define OPTYPE_OT3              0x00c0  // Opcode type 3
#define OPTYPE_OT2              0x0030  // Opcode type 2
#define OPTYPE_OT1              0x000c  // Opcode type 1
#define OPTYPE_OT0              0x0003  // Opcode type 0

#define OPTYPE_READ_NO_ADDR     0
#define OPTYPE_WRITE_NO_ADDR    1
#define OPTYPE_READ_ADDR        2
#define OPTYPE_WRITE_ADDR       3
#define OPTYPE_MASK             3

//
// OPMENU_x - Opcode menu configuration
//            Datasheet 21.7.4.23 - 27.7.4.24
//
#define OPMENU_1                0x3078  // Opcodes 0 - 3
#define OPMENU_2                0x307c  // Opcodes 4 - 7

//
// PBRx - Protected BIOS range 0
//        Datasheet 21.7.4.25 - 21.7.4.27
//
#define PBR0                    0x3080
#define PBR1                    0x3084
#define PBR2                    0x3088
#define PBR_WPE                 0x80000000  // Write protection enable
#define PBR_PRL                 0x00fff000  // Protected range limit
#define PBR_PRB                 0x00000fff  // Protected range base
#define PBR_PRB_SHIFT           12

///
/// Read data from the SPI NOR flash part
/// One command byte and 3 address bytes to send followed by one or more
/// bytes of data to receive
///
#define OPCODE_READ_DATA_INDEX          1
#define OPCODE_READ_DATA_TYPE           OPTYPE_READ_ADDR
#define OPCODE_READ_DATA                SPI_NOR_LOW_FREQUENCY_READ_DATA

///
/// Read status register
/// One command byte followed by one or two bytes to receive
///
#define OPCODE_READ_STATUS_INDEX        2
#define OPCODE_READ_STATUS_TYPE         OPTYPE_READ_NO_ADDR
#define OPCODE_READ_STATUS              SPI_NOR_READ_STATUS

///
/// Read the three bytes of manufacture and device ID
/// One command byte to send followed by 3 bytes of data to receive
///
#define OPCODE_READ_ID_INDEX            3
#define OPCODE_READ_ID_TYPE             OPTYPE_READ_NO_ADDR
#define OPCODE_READ_ID                  SPI_NOR_READ_MANUFACTURE_ID

///
/// Page program: Write data to the SPI NOR flash part
/// One prefix byte, one command byte and 3 address bytes to send followed by
/// up to 256 bytes of data
///
#define OPCODE_WRITE_DATA_INDEX         4
#define OPCODE_WRITE_DATA_TYPE          OPTYPE_WRITE_ADDR
#define OPCODE_WRITE_DATA               SPI_NOR_PAGE_PROGRAM

///
/// Erase 4 KBytes
/// One prefix byte, one command byte and 3 address bytes to send
///
#define OPCODE_ERASE_4KB_INDEX          5
#define OPCODE_ERASE_4KB_TYPE           OPTYPE_WRITE_NO_ADDR
#define OPCODE_ERASE_4KB                SPI_NOR_ERASE_4KB

///
/// Erase block
/// One prefix byte, one command byte and 3 address bytes to send
///
#define OPCODE_ERASE_BLOCK_INDEX        6
#define OPCODE_ERASE_BLOCK_TYPE         OPTYPE_WRITE_NO_ADDR
#define OPCODE_ERASE_32KB               SPI_NOR_ERASE_32KB
#define OPCODE_ERASE_64KB               SPI_NOR_ERASE_64KB

///
/// Write status
/// One prefix byte, one command byte and one or two bytes of data to send
///
#define OPCODE_WRITE_STATUS_INDEX       7
#define OPCODE_WRITE_STATUS_TYPE        OPTYPE_WRITE_NO_ADDR
#define OPCODE_WRITE_STATUS             SPI_NOR_WRITE_STATUS

///
/// Prefix byte to enable write or erase operations
///
#define PREFIX_WRITE_ERASE_INDEX        0
#define PREFIX_WRITE_ERASE              SPI_NOR_ENABLE_WRITE_OR_ERASE

///
/// Prefix byte to enable status writes
///
#define PREFIX_STATUS_WRITE_INDEX       1
#define PREFIX_STATUS_WRITE             SPI_NOR_ENABLE_WRITE_OR_ERASE

///
/// Transaction setup flags
///

///
/// Use prefix 1 on the next SPI transaction
///
#define SPI_HC_FLAG_USE_PREFIX_1        0x00000001

///
/// SPI prefix sent
///
#define SPI_HC_FLAG_PREFIX_SENT         0x00000002

#define SPI_INPUT_CLOCK         MHz(20)

#define SPI_HC_SIGNATURE        SIGNATURE_32 ('L', 's', 'p', 'i')

typedef struct _SPI_HC
{
  //
  // Structure identification
  //
  UINT32 Signature;

  //
  // SPI host controller base address
  //
  UINT32 BaseAddress;

  //
  // Transaction setup flags
  //
  UINT32 Flags;

  //
  // SPI chip select
  //
  UINT32 ChipSelect;

  EFI_HANDLE ControllerHandle;
  EFI_GUID *SpiHcGuid;
  EFI_SPI_HC_PROTOCOL SpiHcProtocol;

  //
  // The BIOS base address in the SPI flash part
  //
  UINT32 BiosBaseAddress;

  //
  // Controller status
  //
  BOOLEAN ControllerLocked;

  //
  // Maximum offset from the BIOS base address that is able to be protected.
  //
  UINT32 MaximumOffset;

  //
  // Maximum number of bytes that can be protected by one range register.
  //
  UINT32 MaximumRangeBytes;

  //
  // The number of registers available for protecting the BIOS.
  //
  UINT32 RangeRegisterCount;

  EFI_LEGACY_SPI_CONTROLLER_PROTOCOL LegacySpiProtocol;
} SPI_HC;

#define SPI_HC_CONTEXT_FROM_PROTOCOL(protocol)                  \
    CR (protocol, SPI_HC, SpiHcProtocol, SPI_HC_SIGNATURE)

#define SPI_HC_CONTEXT_FROM_LEGACY_PROTOCOL(protocol)           \
    CR (protocol, SPI_HC, LegacySpiProtocol, SPI_HC_SIGNATURE)

EFI_STATUS
EFIAPI
SpiHcInitialize (
  SPI_HC **SpiHcPtr,
  EFI_GUID *SpiHcGuid
  );

#endif	// __QUARK_LEGACY_SPI_H__
