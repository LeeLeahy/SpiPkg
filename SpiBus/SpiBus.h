/** @file

  QuarkSpi declares the items necessary to manage the Quark's SPI controller.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SPI_BUS_H__
#define __SPI_BUS_H__

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/SpiHc.h>
#include <Protocol/SpiIo.h>
#include <Library/UefiBootServicesTableLib.h>

typedef struct _SPI_IO SPI_IO;

typedef struct _SPI_IO_TRANSACTION
{
  //
  // Parameters for the SPI host controller
  //
  EFI_SPI_BUS_TRANSACTION BusTransaction;

  //
  // Pointer to the SPI_IO data structure
  //
  SPI_IO *SpiIo;

  //
  // Maximum clock frequency for this transaction
  //
  UINTN ClockHz OPTIONAL;

  //
  // Setup flags in the event of a setup error
  //
  UINT32 SetupFlags;

  ///
  /// Number of transmit bytes containing SPI peripheral layer data
  ///
  UINTN WriteBytes;

  ///
  /// Number of receive data bytes requested by the SPI peripheral layer
  ///
  UINTN ReadBytes;

  ///
  /// SPI peripheral layer buffer for receive data
  ///
  UINT8 *ReadBuffer;
} SPI_IO_TRANSACTION;

typedef struct _SPI_BUS
{
  EFI_HANDLE ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  CONST EFI_SPI_BUS *BusConfig;

  //
  // SPI host controller driver interface
  //
  CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol;

  //
  // IO_TRANSACTION shared with all SPI peripheral drivers in the SPI bus layer.
  // The BUS_TRANSACTION portion is passed to the SPI host controller.
  //
  SPI_IO_TRANSACTION IoTransaction;

  //
  // Legacy SPI host controller protocol
  //
  CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol;
} SPI_BUS;

#define SPI_IO_SIGNATURE        SIGNATURE_32 ('S', 'P', 'I', 'O')

typedef struct _SPI_IO
{
  //
  // Structure identification
  //
  UINT32 Signature;
  SPI_BUS *SpiBus;
  EFI_HANDLE Handle;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_SPI_IO_PROTOCOL SpiIoProtocol;
} SPI_IO;

#define SPI_IO_CONTEXT_FROM_PROTOCOL(protocol)         \
    CR (protocol, SPI_IO, SpiIoProtocol, SPI_IO_SIGNATURE)

#define SPI_IO_TRANSACTION_SIGNATURE    SIGNATURE_32 ('S', 'P', 'I', 'T')

#define IO_TRANSACTION_FROM_ENTRY(a)    \
        CR (a, SPI_IO_TRANSACTION, Link, SPI_IO_TRANSACTION_SIGNATURE);

#define SETUP_FLAG_CLOCK_RUNNING                0x00000001
#define SETUP_FLAG_CHIP_SELECTED                0x00000002
#define SETUP_FLAG_DISCARD_WRITE_BUFFER         0x00000004
#define SETUP_FLAG_DISCARD_READ_BUFFER          0x00000008
#define SETUP_FLAG_COPY_READ_DATA               0x00000010
#define SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_16   0x00000020
#define SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_24   0x00000040
#define SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_32   0x00000080

typedef struct _SPI_DEVICE_PATH
{
  CONTROLLER_DEVICE_PATH Controller;
  EFI_DEVICE_PATH_PROTOCOL  End;
} SPI_DEVICE_PATH;

#define SPI_PART_NUMBER(PartNumber) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_CONTROLLER_DP, \
      { \
        (UINT8) (sizeof (CONTROLLER_DEVICE_PATH)), \
        (UINT8) ((sizeof (CONTROLLER_DEVICE_PATH)) >> 8) \
      }, \
    }, \
    (PartNumber) \
  }

#define END_DEVICE_PATH \
  { \
    END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, \
    { \
      END_DEVICE_PATH_LENGTH, \
      0 \
    } \
  }

EFI_STATUS
EFIAPI
SpiBusSetupBuffers (
  IN SPI_BUS *SpiBus
  );

EFI_STATUS
EFIAPI
SpiBusTransaction (
  IN SPI_BUS *SpiBus
  );

VOID
EFIAPI
SpiIoShutdown (
  IN SPI_IO *SpiIo
  );

EFI_STATUS
EFIAPI
SpiIoStartup (
  IN SPI_BUS *SpiBus,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral
  );

EFI_STATUS
EFIAPI
SpiInstallBusProtocol (
  IN EFI_HANDLE *Handle,
  IN VOID *Protocol
  );

EFI_STATUS
EFIAPI
SpiUninstallBusProtocol (
  IN EFI_HANDLE *Handle,
  IN VOID *Protocol
  );

EFI_STATUS
EFIAPI
SpiInstallIoProtocol (
  IN EFI_HANDLE *Handle,
  IN SPI_IO *SpiIo,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral
  );

EFI_STATUS
EFIAPI
SpiBusStartup (
  IN EFI_HANDLE ControllerHandle,
  IN CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol
  );

EFI_STATUS
EFIAPI
SpiBusConnectHc (
  IN SPI_BUS *SpiBus
  );

EFI_STATUS
EFIAPI
SpiOpenProtocol (
  IN  EFI_HANDLE                Handle,
  IN  EFI_GUID                  *Protocol,
  OUT VOID                      **Interface, OPTIONAL
  IN  EFI_HANDLE                AgentHandle,
  IN  EFI_HANDLE                ControllerHandle,
  IN  UINT32                    Attributes
  );

EFI_STATUS
EFIAPI
SpiCloseProtocol(
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  IN EFI_HANDLE               AgentHandle,
  IN EFI_HANDLE               ControllerHandle
  );

VOID
EFIAPI
SpiConnectController(
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    *DriverImageHandle,   OPTIONAL
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath, OPTIONAL
  IN  BOOLEAN                       Recursive
  );

EFI_TPL
EFIAPI
SpiRaiseTpl(
  IN EFI_TPL      NewTpl
  );

VOID
EFIAPI
SpiRestoreTpl(
  IN EFI_TPL      OldTpl
  );

/* Define the externals to enable support of SMM */
extern EFI_GUID *gLegacySpiControllerProtocolGuid;
extern EFI_GUID *gSpiHcProtocolGuid;
extern EFI_GUID gSpiBusLayerGuid;

#endif	// __SPI_BUS_H__
