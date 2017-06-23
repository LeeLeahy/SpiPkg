/** @file

  This module declares the SPI host controller protocol.

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

#ifndef __SPI_HC_H__
#define __SPI_HC_H__

#include <Protocol/SpiConfiguration.h>
#include <Protocol/SpiIo.h>

typedef struct _EFI_SPI_HC_PROTOCOL EFI_SPI_HC_PROTOCOL;

/**

  Assert or deassert the SPI chip select.

  This routine is called at TPL_NOTIFY.

  Update the value of the chip select line for a SPI peripheral.  The SPI bus
  layer calls this routine either in the board layer or in the SPI controller
  to manipulate the chip select pin at the start and end of a SPI transaction.

  @param[in]  This              Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in]  SpiPeripheral     The address of an EFI_SPI_PERIPHERAL data
                                structure describing the SPI peripheral whose
                                chip select pin is to be manipulated.  The
                                routine may access the ChipSelectParameter field
                                to gain sufficient context to complete the
                                operation.
  @param[in]  PinValue          The value to be applied to the chip select line
                                of the SPI peripheral.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The chip select was set successfully
  @retval EFI_NOT_READY         Support for the chip select is not properly
                                initialized
  @retval EFI_UNSUPPORTED       The ChipSelectParameter value is invalid

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_HC_PROTOCOL_CHIP_SELECT) (
  IN CONST EFI_SPI_HC_PROTOCOL *This,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  IN BOOLEAN PinValue
  );

/**
  Set up the clock generator to produce the correct clock frequency, phase and
  polarity for a SPI chip.

  This routine is called at TPL_NOTIFY.

  This routine updates the clock generator to generate the correct frequency and
  polarity for the SPI clock.

  @param[in]  This              Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in]  SpiPeripheral     Pointer to a EFI_SPI_PERIPHERAL data structure
                                from which the routine can access the
                                ClockParameter, ClockPhase and ClockPolarity
                                fields.  The routine also has access to the
                                names for the SPI bus and chip which can be used
                                during debugging.
  @param[in,out] ClockHz        Pointer to the requested clock frequency.  The
                                SPI host controller will choose a supported
                                clock frequency which is less than or equal to
                                this value.  Specify zero to turn the clock
                                generator off.  The actual clock frequency
                                supported by the SPI host controller will be
                                returned.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The clock was set up successfully.
  @retval EFI_UNSUPPORTED       The SPI controller was not able to support the
                                frequency requested by ClockHz
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_HC_PROTOCOL_CLOCK) (
  IN CONST EFI_SPI_HC_PROTOCOL *This,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  IN UINT32 *ClockHz
  );

/**
  Perform the SPI transaction on the SPI peripheral using the SPI host
  controller.

  This routine is called at TPL_NOTIFY.

  This routine initiates the SPI transaction on the SPI host controller.  The
  routine then waits for completion of the SPI transaction prior to returning
  the final transaction status.

  @param[in]  This              Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in]  BusTransaction    Pointer to a EFI_SPI_BUS_TRANSACTION containing
                                the description of the SPI transaction to
                                perform.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI transaction completed successfully
  @retval EFI_BAD_BUFFER_SIZE   The BusTransaction->WriteBytes value is invalid
  @retval EFI_BAD_BUFFER_SIZE   The BusTransaction->ReadBytes value is invalid
  @retval EFI_UNSUPPORTED       The BusTransaction->TransactionType is
                                unsupported
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_HC_PROTOCOL_TRANSACTION) (
  IN CONST EFI_SPI_HC_PROTOCOL *This,
  IN EFI_SPI_BUS_TRANSACTION *BusTransaction
  );

///
/// Define the SPI host controller attributes
///
/// The SPI host controller must support full-duplex (receive while sending)
/// operation.
///
#define HC_SUPPORTS_WRITE_ONLY_OPERATIONS       0x00000001
#define HC_SUPPORTS_READ_ONLY_OPERATIONS        0x00000002
#define HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS  0x00000004

///
/// The SPI host controller requires the transmit frame to be in most
/// significant bits instead of least significant bits.  The host driver will
/// adjust the frames if necessary.
///
#define HC_TX_FRAME_IN_MOST_SIGNIFICANT_BITS    0x00000008

///
/// The SPI host controller places the receive frame to be in most
/// significant bits instead of least significant bits.  The host driver will
/// adjust the frames to be in the least significant bits if necessary.
///
#define HC_RX_FRAME_IN_MOST_SIGNIFICANT_BITS    0x00000010

///
/// The SPI controller supports a 2-bit data bus
///
#define HC_SUPPORTS_2_BIT_DATA_BUS_WIDTH        0x00000020

///
/// The SPI controller supports a 4-bit data bus
///
#define HC_SUPPORTS_4_BIT_DATA_BUS_WIDTH        0x00000040

///
/// Transfer size includes the opcode byte
///
#define HC_TRANSFER_SIZE_INCLUDES_OPCODE        0x00000080

///
/// Transfer size includes the 3 address bytes
///
#define HC_TRANSFER_SIZE_INCLUDES_ADDRESS       0x00000100

///
/// Macro to specify a supported frame size in bits per frame
///
#define SUPPORT_FRAME_SIZE_BITS(BitsPerFrame)	(1 << (BitsPerFrame - 1))

///
/// Support a SPI data transaction between the SPI controller and a SPI chip.
///
struct _EFI_SPI_HC_PROTOCOL {
  ///
  /// Host controller attributes
  ///
  UINT32 Attributes;
  ///
  /// Mask of frame sizes which the SPI host controller supports. Frame size
  /// of N-bits is supported when bit N-1 is set.  The host controller must
  /// support a frame size of 8-bits.
  ///
  UINT32 FrameSizeSupportMask;
  ///
  /// Maximum transfer size in bytes: 1 - 0xffffffff
  ///
  UINT32 MaximumTransferBytes;
  EFI_SPI_HC_PROTOCOL_CHIP_SELECT ChipSelect;
  EFI_SPI_HC_PROTOCOL_CLOCK Clock;
  EFI_SPI_HC_PROTOCOL_TRANSACTION Transaction;
};

#endif  //  __SPI_HC_H__
