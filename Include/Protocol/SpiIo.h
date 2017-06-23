/** @file

  This module declares the SPI IO protocol.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Support managed SPI data transactions between the SPI controller and a SPI
  chip.

  @par Revision Reference:
  This protocol is from PI Version 1.6.

**/

#ifndef __SPI_IO_H__
#define __SPI_IO_H__

#include <Protocol/SpiConfiguration.h>
#include <Protocol/LegacySpiController.h>

typedef struct _EFI_SPI_IO_PROTOCOL EFI_SPI_IO_PROTOCOL;

///
/// Define the different types of SPI transactions
///
typedef enum _EFI_SPI_TRANSACTION_TYPE {
  ///
  /// Data flowing in both direction between the host and SPI peripheral.
  /// ReadBytes must equal WriteBytes and both ReadBuffer and WriteBuffer
  /// must be provided.
  ///
  SPI_TRANSACTION_FULL_DUPLEX = 0,
  ///
  /// Data flowing from the host to the SPI peripheral.  ReadBytes must be
  /// zero.  WriteBytes must be non-zero and WriteBuffer must be provided.
  ///
  SPI_TRANSACTION_WRITE_ONLY,
  ///
  /// Data flowing from the SPI peripheral to the host.  WriteBytes must be
  /// zero.  ReadBytes must be non-zero and ReadBuffer must be provided.
  ///
  SPI_TRANSACTION_READ_ONLY,
  ///
  /// Data first flowing from the host to the SPI peripheral and then data
  /// flows from the SPI peripheral to the host.  These types of operations
  /// get used for SPI flash devices when control data (opcode, address) must
  /// be passed to the SPI peripheral to specify the data to be read.
  ///
  SPI_TRANSACTION_WRITE_THEN_READ
} EFI_SPI_TRANSACTION_TYPE;

///
/// The EFI_SPI_BUS_TRANSACTION data structure contains the description of
/// the SPI transaction to perform on the host controller.
///
typedef struct _EFI_SPI_BUS_TRANSACTION
{
  ///
  /// Pointer to the SPI peripheral being manipulated
  ///
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral;

  ///
  /// Type of transaction specified by one of the EFI_SPI_TRANSACTION_TYPE
  /// values.
  ///
  EFI_SPI_TRANSACTION_TYPE TransactionType;

  ///
  /// TRUE if the transaction is being debugged.  Debugging may be turned on for
  /// a single SPI transaction.  Only this transaction will display debugging
  /// messages.  All other transactions with this value set to FALSE will not
  /// display any debugging messages.
  ///
  BOOLEAN DebugTransaction;

  ///
  /// SPI bus width in bits: 1, 2, 4
  ///
  UINT32 BusWidth;

  ///
  /// Frame size in bits, range: 1 - 32
  ///
  UINT32 FrameSize;

  ///
  /// Length of the write buffer in bytes
  ///
  UINT32 WriteBytes;

  ///
  /// Buffer containing data to send to the SPI peripheral
  /// * Frame sizes 1-8 bits: UINT8 (one byte) per frame
  /// * Frame sizes 7-16 bits: UINT16 (two bytes) per frame
  /// * Frame sizes 17-32 bits: UINT32 (four bytes) per frame
  ///
  UINT8 *WriteBuffer;

  ///
  /// Length of the read buffer in bytes
  ///
  UINT32 ReadBytes;

  ///
  /// Buffer to receive the data from the SPI peripheral
  /// * Frame sizes 1-8 bits: UINT8 (one byte) per frame
  /// * Frame sizes 7-16 bits: UINT16 (two bytes) per frame
  /// * Frame sizes 17-32 bits: UINT32 (four bytes) per frame
  ///
  UINT8 *ReadBuffer;
} EFI_SPI_BUS_TRANSACTION;

/**
  Initiate a SPI transaction between the host and a SPI peripheral.

  This routine must be called at or below TPL_NOTIFY.

  This routine works with the SPI bus layer to pass the SPI transaction to
  the SPI controller for execution on the SPI bus.  There are four types of
  supported transactions supported by this routine:
  * Full Duplex: WriteBuffer and ReadBuffer are the same size.
  * Write Only: WriteBuffer contains data for SPI peripheral, ReadBytes = 0
  * Read Only: ReadBuffer to receive data from SPI peripheral, WriteBytes = 0
  * Write Then Read: WriteBuffer contains control data to write to SPI
    peripheral before data is placed into the ReadBuffer.  Both WriteBytes and
    ReadBytes must be non-zero.

  @param[in]  This              Pointer to an EFI_SPI_IO_PROTOCOL structure.
  @param[in]  TransactionType   Type of SPI transaction specified by one of the
                                EFI_SPI_TRANSACTION_TYPE values.
  @param[in]  DebugTransaction  Set TRUE only when debugging is desired.
                                Debugging may be turned on for a single SPI
                                transaction.  Only this transaction will display
                                debugging messages.  All other transactions with
                                this value set to FALSE will not display any
                                debugging messages.
  @param[in]  ClockHz           Specify the ClockHz value as zero (0) to use the
                                maximum clock frequency supported by the SPI
                                controller and part.  Specify a non-zero value
                                only when a specific SPI transaction requires a
                                reduced clock rate.
  @param[in]  BusWidth          Width of the SPI bus in bits: 1, 2, 4
  @param[in]  FrameSize         Frame size in bits, range: 1 - 32
  @param[in]  WriteBytes        The length of the WriteBuffer in bytes.  Specify
                                zero for read-only operations.
  @param[in]  WriteBuffer       The buffer containing data to be sent from the
                                host to the SPI chip.  Specify NULL for read
                                only operations.
                                * Frame sizes 1-8 bits: UINT8 (one byte) per
                                  frame
                                * Frame sizes 7-16 bits: UINT16 (two bytes) per
                                  frame
                                * Frame sizes 17-32 bits: UINT32 (four bytes)
                                  per frame
                                The transmit frame is in the least significant
                                N bits.
  @param[in]  ReadBytes         The length of the ReadBuffer in bytes.  Specify
                                zero for write-only operations.
  @param[in]  ReadBuffer        The buffer to receive data from the SPI chip
                                during the transaction.  Specify NULL for write
                                only operations.
                                * Frame sizes 1-8 bits: UINT8 (one byte) per
                                  frame
                                * Frame sizes 7-16 bits: UINT16 (two bytes) per
                                  frame
                                * Frame sizes 17-32 bits: UINT32 (four bytes)
                                  per frame
                                The received frame is in the least significant
                                N bits.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI transaction completed successfully
  @retval EFI_BAD_BUFFER_SIZE   The WriteBytes value was invalid.
  @retval EFI_BAD_BUFFER_SIZE   The ReadBytes value was invalid.
  @retval EFI_INVALID_PARAMETER TransactionType is not valid
  @retval EFI_INVALID_PARAMETER BusWidth not supported by SPI peripheral or
                                SPI host controller
  @retval EFI_INVALID_PARAMETER WriteBytes non-zero and WriteBuffer is NULL
  @retval EFI_INVALID_PARAMETER ReadBytes non-zero and ReadBuffer is NULL
  @retval EFI_INVALID_PARAMETER ReadBytes != WriteBytes for full-duplex type
  @retval EFI_INVALID_PARAMETER TPL too high
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for SPI transaction
  @retval EFI_UNSUPPORTED       The FrameSize is not supported by the SPI
                                bus layer or the SPI host controller.
  @retval EFI_UNSUPPORTED       The SPI controller was not able to support the
                                frequency requested by ClockHz
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_IO_PROTOCOL_TRANSACTION) (
  IN CONST EFI_SPI_IO_PROTOCOL *This,
  IN EFI_SPI_TRANSACTION_TYPE TransactionType,
  IN BOOLEAN DebugTransaction,
  IN UINT32 ClockHz OPTIONAL,
  IN UINT32 BusWidth,
  IN UINT32 FrameSize,
  IN UINT32 WriteBytes,
  IN UINT8 *WriteBuffer,
  IN UINT32 ReadBytes,
  OUT UINT8 *ReadBuffer
  );

/**
  Update the SPI peripheral associated with this SPI IO instance.

  This routine must be called at or below TPL_NOTIFY.

  Support socketed SPI parts by allowing the SPI peripheral driver to replace
  the SPI peripheral after the connection is made.  An example use is socketed
  SPI NOR flash parts, where the size and parameters change depending upon
  device is in the socket.

  @param[in]  This              Pointer to an EFI_SPI_IO_PROTOCOL structure.
  @param[in]  SpiPeripheral     Pointer to an EFI_SPI_PERIPHERAL structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI peripheral was updated successfully
  @retval EFI_INVALID_PARAMETER The SpiPeripheral value is NULL
  @retval EFI_INVALID_PARAMETER The SpiPeripheral->SpiBus is NULL
  @retval EFI_INVALID_PARAMETER The SpiPeripheral->SpiBus pointing at wrong bus
  @retval EFI_INVALID_PARAMETER The SpiPeripheral->SpiPart is NULL

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_IO_PROTOCOL_UPDATE_SPI_PERIPHERAL) (
  IN CONST EFI_SPI_IO_PROTOCOL *This,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral
  );

///
/// Transaction attributes
///

///
/// The SPI host and peripheral supports a 2-bit data bus
///
#define SPI_IO_SUPPORTS_2_BIT_DATA_BUS_WIDTH    0x00000001

///
/// The SPI host and peripheral supports a 4-bit data bus
///
#define SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH    0x00000002

///
/// Transfer size includes the opcode byte
///
#define SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE    0x00000004

///
/// Transfer size includes the 3 address bytes
///
#define SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS   0x00000008

///
/// Support managed SPI data transactions between the SPI controller and a SPI
/// chip.
///
struct _EFI_SPI_IO_PROTOCOL {
  ///
  /// Address of an *``EFI_SPI_PERIPHERAL``* data structure associated with this
  /// protocol instance.
  ///
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral;

  ///
  /// Address of the original *``EFI_SPI_PERIPHERAL``* data structure associated
  /// with this protocol instance.
  ///
  CONST EFI_SPI_PERIPHERAL *OriginalSpiPeripheral;

  ///
  /// Mask of frame sizes which the SPI IO layer supports.  Frame size
  /// of N-bits is supported when bit N-1 is set.  The host controller must
  /// support a frame size of 8-bits.  Frame sizes of 16, 24 and 32-bits are
  /// converted to 8-bit frame sizes by the SPI bus layer if the frame size
  /// is not supported by the SPI host controller.
  ///
  UINT32 FrameSizeSupportMask;

  ///
  /// Maximum transfer size in bytes: 1 - 0xffffffff
  ///
  UINT32 MaximumTransferBytes;

  ///
  /// Transaction attributes
  ///
  UINT32 Attributes;

  ///
  /// Legacy SPI controller protocol
  ///
  CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol;

  EFI_SPI_IO_PROTOCOL_TRANSACTION Transaction;
  EFI_SPI_IO_PROTOCOL_UPDATE_SPI_PERIPHERAL UpdateSpiPeripheral;
};

#endif  //  __SPI_IO_H__
