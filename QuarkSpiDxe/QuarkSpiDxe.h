/** @file

  QuarkSpi declares the items necessary to manage the Quark's SPI controller.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __QUARK_SPI_DXE_H__
#define __QUARK_SPI_DXE_H__

#include <Uefi.h>
#include <IndustryStandard/Pci.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/PciIo.h>
#include <Protocol/SpiHc.h>
#include <Library/UefiBootServicesTableLib.h>

#define   INTEL_VENDOR_ID          0x8086
#define   QUARK_SPI_DEVICE_ID      0x0935

//
// SSCR0 - SPI Control Register 0
//         Datasheet 20.5.1
//
#define SSCR0                   0
#define SSCR0_SCR               0x0000ff00  // Serial clock rate
#define SSCR0_SCR_SHIFT         8
#define SSCR0_SSE               0x00000080  // Synchronous serial port enable
#define SSCR0_FRF               0x00000060  // Frame format
#define SSCR0_FRF_SHIFT         5
#define SSCR0_DSS               0x0000001f  // Data size select
#define SSCR0_DSS_8_BIT         7
#define SSCR0_DSS_16_BIT        15
#define SSCR0_DSS_32_BIT        31

//
// SSCR1 - SPI Control Register 1
//         Datasheet 20.5.2
//
#define SSCR1                   4
#define SSCR1_STRF              0x00020000  // Select FIFO for SDR access
#define SSCR1_STRF_TRANSMIT     0
#define SSCR1_STRF_RECEIVE      SSCR1_STRF
#define SSCR1_RFT               0x0000f800  // Receive FIFO interrupt threshold
#define SSCR1_RFT_SHIFT         11
#define SSCR1_TFT               0x000007c0  // Transmit FIFO interrupt threshold
#define SSCR1_TFT_SHIFT         6
#define SSCR1_SPH               0x00000010  // Serial clock phase
#define SSCR1_SPO               0x00000008  // Serial clock polarity
#define SSCR1_TIE               0x00000002  // Transmit FIFO interrupt enable
#define SSCR1_RIE               0x00000001  // Receive FIFO interrupt enable

//
// SSSR - SPI Status Register
//        Datasheet 20.5.3
//
#define SSSR                    8
#define SSSR_RFL                0x0003e000  // Receive FIFO level
#define SSSR_RFL_SHIFT          13
#define SSSR_TFL                0x00001f00  // Transmit FIFO level
#define SSSR_TFL_SHIFT          8
#define SSSR_ROR                0x00000080  // Receiver overrun
#define SSSR_RFS                0x00000040  // Receive FIFO service flag
#define SSSR_TFS                0x00000020  // Transmit FIFO service flag
#define SSSR_BSY                0x00000010  // SPI busy flag
#define SSSR_RNE                0x00000008  // Receive FIFO not empty
#define SSSR_TNF                0x00000004  // Transmit FIFO not full

//
// SSDR - SPI Data Register
//        Datasheet 20.5.4
//
#define SSDR                    0x10

//
// DDS_RATE - DDS Clock Rate Register
//            Datasheet 20.5.5
//
#define DDS_RATE                0x28
#define DDS_CLOCK_RATE          0x00ffffff  // Value added to clock counter

#define SPI_INPUT_CLOCK         (200 * 1000 * 1000)  // 200 MHz

#define SPI_HC_SIGNATURE        SIGNATURE_32 ('S', 'p', 'i', 'C')

typedef struct _SPI_HC
{
  //
  // Structure identification
  //
  UINT32 Signature;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  EFI_HANDLE ControllerHandle;
  EFI_PCI_IO_PROTOCOL *PciIo;
  EFI_SPI_HC_PROTOCOL SpiHcProtocol;

  //
  // SPI host controller base address
  //
  UINT32 BaseAddress;

  //
  // Sscr0 register value for the transfer
  //
  UINT32 Sscr0;

  //
  // Sscr1 register value for the transfer
  //
  UINT32 Sscr1;

  //
  // Clock rate divider setting for the SPI transaction
  //
  UINT32 ClockRate;
} SPI_HC;

#define SPI_HC_CONTEXT_FROM_PROTOCOL(protocol)         \
    CR (protocol, SPI_HC, SpiHcProtocol, SPI_HC_SIGNATURE)

/**
  Perform a full-duplex SPI transaction with the SPI peripheral using the SPI
  host controller.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  BaseAddress       Address of the SPI host controller registers
  @param[in]  WriteBytes        Number of bytes to send to the SPI peripheral
  @param[in]  WriteBuffer       Pointer to the data to send to the SPI
                                peripheral
  @param[in]  ReadBytes         Number of bytes to receive from the SPI
                                peripheral
  @param[in]  ReadBuffer        Pointer to the receive data buffer
**/
typedef
VOID
(EFIAPI *SPI_TRANSACTION) (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN VOID *WriteBuffer,
  IN UINTN ReadBytes,
  IN VOID *ReadBuffer
  );

EFI_STATUS
EFIAPI
SpiHcComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
SpiHcShutdown (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol
  );

EFI_STATUS
EFIAPI
SpiHcStartup (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN EFI_HANDLE ControllerHandle
  );

VOID
EFIAPI
SpiHc8BitFullDuplexTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT8* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT8* ReadBuffer
  );

VOID
EFIAPI
SpiHc8BitWriteOnlyTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT8* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT8* ReadBuffer
  );

VOID
EFIAPI
SpiHc8BitWriteThenReadTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT8* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT8* ReadBuffer
  );

VOID
EFIAPI
SpiHc16BitFullDuplexTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT16* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT16* ReadBuffer
  );

VOID
EFIAPI
SpiHc16BitWriteOnlyTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT16* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT16* ReadBuffer
  );

VOID
EFIAPI
SpiHc16BitWriteThenReadTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT16* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT16* ReadBuffer
  );

VOID
EFIAPI
SpiHc32BitFullDuplexTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT32* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT32* ReadBuffer
  );

VOID
EFIAPI
SpiHc32BitWriteOnlyTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT32* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT32* ReadBuffer
  );

VOID
EFIAPI
SpiHc32BitWriteThenReadTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT32* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT32* ReadBuffer
  );

#endif	// __QUARK_SPI_DXE_H__
