/** @file

  This module implements the SPI IO protocol.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
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

#include "SpiBus.h"

STATIC SPI_DEVICE_PATH mSpiPartDevicePath =
{
  SPI_PART_NUMBER(0),
  END_DEVICE_PATH
};

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
EFI_STATUS
EFIAPI SpiIoTransaction (
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
  )
{
  EFI_SPI_BUS_TRANSACTION *BusTransaction;
  SPI_IO_TRANSACTION *IoTransaction;
  EFI_TPL PreviousTpl;
  SPI_BUS *SpiBus;
  SPI_IO *SpiIo;
  EFI_STATUS Status;

  //
  // Locate the context data structure
  //
  SpiIo = SPI_IO_CONTEXT_FROM_PROTOCOL(This);

  //
  // Validate the parameters for this SPI transaction
  //
  if (BusWidth >= 2) {
    if (((BusWidth == 2)
        && ((This->Attributes & SPI_IO_SUPPORTS_2_BIT_DATA_BUS_WIDTH) == 0))
      || ((BusWidth == 4)
        && ((This->Attributes & SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH) == 0))) {
      DEBUG((EFI_D_ERROR,
       "ERROR - System does not support a %d-bit data path!\n", BusWidth));
    }
    return EFI_INVALID_PARAMETER;
  }
  if ((FrameSize > 32)
      | ((This->FrameSizeSupportMask & (1 << (FrameSize - 1))) == 0)) {
    DEBUG((EFI_D_ERROR,
       "ERROR - SPI controller does not support FrameSize of %d bits/frame!\n",
       FrameSize));
    return EFI_UNSUPPORTED;
  }

  switch (TransactionType) {
  default:
    DEBUG((EFI_D_ERROR, "ERROR - Invalid SPI transaction type!\n"));
    return EFI_INVALID_PARAMETER;

  case SPI_TRANSACTION_FULL_DUPLEX:
    //
    // Data flowing in both direction as the same time.  ReadBytes must equal
    // WriteBytes and both ReadBuffer and WriteBuffer must be provided.
    //
    if (WriteBytes == 0) {
      DEBUG((EFI_D_ERROR, "ERROR - WriteBytes is zero!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (ReadBytes != WriteBytes) {
      DEBUG((EFI_D_ERROR, "ERROR - ReadBytes != WriteBytes!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (WriteBuffer == NULL) {
      DEBUG((EFI_D_ERROR, "ERROR - WriteBuffer is NULL!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (ReadBuffer == NULL) {
      DEBUG((EFI_D_ERROR, "ERROR - ReadBuffer is NULL!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiIo: Full-duplex SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiIo: Sending data from 0x%08x, 0x%08x bytes\n",
              WriteBuffer, WriteBytes));
      DEBUG ((EFI_D_ERROR, "SpiIo: Receiving data into 0x%08x, 0x%08x bytes\n",
              ReadBuffer, ReadBytes));
    }
    break;

  case SPI_TRANSACTION_WRITE_ONLY:
    //
    // Data flowing from the host to the SPI peripheral.  ReadBytes must be
    // zero.  WriteBytes must be non-zero and WriteBuffer must be provided.
    //
    if (ReadBytes != 0) {
      DEBUG((EFI_D_ERROR, "ERROR - ReadBytes is non-zero!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (WriteBytes == 0) {
      DEBUG((EFI_D_ERROR, "ERROR - WriteBytes is zero!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (WriteBuffer == NULL) {
      DEBUG((EFI_D_ERROR, "ERROR - WriteBuffer is NULL!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiIo: Write-only SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiIo: Sending data from 0x%08x, 0x%08x bytes\n",
              WriteBuffer, WriteBytes));
    }
    break;

  case SPI_TRANSACTION_READ_ONLY:
    //
    // Data flowing from the SPI peripheral to the host.  WriteBytes must be
    // zero.  ReadBytes must be non-zero and ReadBuffer must be provided.
    //
    if (WriteBytes != 0) {
      DEBUG((EFI_D_ERROR, "ERROR - WriteBytes is non-zero!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (ReadBytes == 0) {
      DEBUG((EFI_D_ERROR, "ERROR - ReadBytes is zero!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (ReadBuffer == NULL) {
      DEBUG((EFI_D_ERROR, "ERROR - ReadBuffer is NULL!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiIo: Read-only SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiIo: Receiving data into 0x%08x, 0x%08x bytes\n",
              ReadBuffer, ReadBytes));
    }
    break;

  case SPI_TRANSACTION_WRITE_THEN_READ:
    //
    // Data first flowing from the host to the SPI peripheral and then data
    // flows from the SPI peripheral to the host.  These types of operations
    // get used for SPI flash devices when control data (opcode, address) must
    // be passed to the SPI peripheral to specify the data to be read.
    //
    if (WriteBytes == 0) {
      DEBUG((EFI_D_ERROR, "ERROR - WriteBytes is zero!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (WriteBuffer == NULL) {
      DEBUG((EFI_D_ERROR, "ERROR - WriteBuffer is NULL!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (ReadBytes == 0) {
      DEBUG((EFI_D_ERROR, "ERROR - ReadBytes is zero!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (ReadBuffer == NULL) {
      DEBUG((EFI_D_ERROR, "ERROR - ReadBuffer is NULL!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiIo: Write-then-read SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiIo: Sending data from 0x%08x, 0x%08x bytes\n",
              WriteBuffer, WriteBytes));
      DEBUG ((EFI_D_ERROR, "SpiIo: Receiving data into 0x%08x, 0x%08x bytes\n",
              ReadBuffer, ReadBytes));
    }
    break;
  }

  //
  // Synchronize with the SPI bus layer
  //
  if (DebugTransaction) {
    DEBUG ((EFI_D_ERROR, "SpiIo: Synchronizing with SPI bus layer\n"));
  }
  PreviousTpl = SpiRaiseTpl(TPL_NOTIFY);
  if (DebugTransaction) {
    DEBUG ((EFI_D_ERROR, "SpiIo: Calling TPL: %d\n", PreviousTpl));
    DEBUG ((EFI_D_ERROR, "SpiIo: TPL: %d\n", TPL_NOTIFY));
  }

  //
  // Verify the TPL
  //
  if (PreviousTpl > TPL_NOTIFY) {
    if (DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiIo: Releasing synchronizing with SPI bus layer\n"));
      DEBUG ((EFI_D_ERROR, "SpiIo: TPL: %d\n", PreviousTpl));
    }
    SpiRestoreTpl (PreviousTpl);
    DEBUG ((EFI_D_ERROR,
            "ERROR - TPL (%d) > TPL_NOTIFY!\n",
            PreviousTpl));
    return EFI_INVALID_PARAMETER;
  }

  //
  // This SPI IO instance owns the IO_TRANSACTION until the TPL is dropped
  // below TPL notify
  //
  SpiBus = SpiIo->SpiBus;
  IoTransaction = &SpiBus->IoTransaction;
  ZeroMem (IoTransaction, sizeof(*IoTransaction));
  if (DebugTransaction) {
    DEBUG ((EFI_D_ERROR, "SpiIo: Using IoTransaction 0x%08x\n",
            IoTransaction));
  }

  //
  // Initialize the structure for the SPI transaction
  //
  IoTransaction->SpiIo = SpiIo;
  IoTransaction->ClockHz = ClockHz;

  BusTransaction = &IoTransaction->BusTransaction;
  BusTransaction->SpiPeripheral = SpiIo->SpiIoProtocol.SpiPeripheral;
  BusTransaction->TransactionType = TransactionType;
  BusTransaction->DebugTransaction = DebugTransaction;
  BusTransaction->BusWidth = BusWidth;
  BusTransaction->FrameSize = FrameSize;
  BusTransaction->WriteBytes = WriteBytes;
  BusTransaction->WriteBuffer = WriteBuffer;
  BusTransaction->ReadBytes = ReadBytes;
  BusTransaction->ReadBuffer = ReadBuffer;

  //
  // Setup the buffers for the SPI transaction
  //
  Status = SpiBusSetupBuffers (SpiBus);
  if (!EFI_ERROR(Status)) {
    //
    // Start the transaction
    //
    Status = SpiBusTransaction (SpiBus);
  }
  if (DebugTransaction) {
    DEBUG ((EFI_D_ERROR, "SpiBus: Releasing IoTransaction 0x%08x\n",
            IoTransaction));
  }

  //
  // Release the synchronization with the SPI bus layer
  //
  if (DebugTransaction) {
    DEBUG ((EFI_D_ERROR,
            "SpiIo: Releasing synchronizing with SPI bus layer\n"));
    DEBUG ((EFI_D_ERROR, "SpiIo: TPL: %d\n", PreviousTpl));
  }
  SpiRestoreTpl (PreviousTpl);
  if (DebugTransaction) {
    DEBUG ((EFI_D_ERROR,
            "SpiIo returning Status: %r\n", Status));
  }

  //
  // Return the final status
  //
  return Status;
}

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
EFI_STATUS
EFIAPI
SpiIoUpdateSpiPeripheral (
  IN CONST EFI_SPI_IO_PROTOCOL *This,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral
  )
{
  SPI_IO *SpiIo;

  //
  // Locate the context data structure
  //
  SpiIo = SPI_IO_CONTEXT_FROM_PROTOCOL(This);

  //
  // Validate the parameters for this SPI peripheral
  //
  if (SpiPeripheral == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiPeripheral is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }
  if (SpiPeripheral->SpiBus == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiPeripheral->SpiBus is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }
  if (SpiPeripheral->SpiBus != SpiIo->SpiBus->BusConfig) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - SpiPeripheral->SpiBus pointing at wrong SPI bus!\n"));
    return EFI_INVALID_PARAMETER;
  }
  if (SpiPeripheral->SpiPart == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiPeripheral->SpiPart is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Substitute the SPI peripheral
  //
  SpiIo->SpiIoProtocol.SpiPeripheral = SpiPeripheral;
  return EFI_SUCCESS;
}

/**
  Shuts down the SPI IO protocol.

  This routine must be called at or below TPL_NOTIFY.

  This routine deallocates the resources supporting the SPI IO protocol.

  @param[in]  SpiIo             Pointer to a SPI_IO data structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.

  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
VOID
EFIAPI
SpiIoShutdown (
  IN SPI_IO *SpiIo
  )
{
  //
  // Determine if the job is already done
  //
  if (SpiIo != NULL) {
    //
    // Free the device path
    //
    if (SpiIo->DevicePath != NULL) {
      FreePool (SpiIo->DevicePath);
    }

    //
    // Free the data structure
    //
    FreePool (SpiIo);
  }
}

/**
  Initialize the SPI IO protocol for a SPI part.

  This routine must be called at or below TPL_NOTIFY.

  Initialize the context data structure to support the SPI IO protocol.

  @param[in]  SpiBus            A pointer to the SPI_BUS instance.
  @param[in]  SpiPeripheral     A pointer to the EFI_SPI_PERIPHERAL instance.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI host controller was started successfully

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
**/
EFI_STATUS
EFIAPI
SpiIoStartup (
  IN SPI_BUS *SpiBus,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral
  )
{
  EFI_HANDLE Handle;
  SPI_IO *SpiIo;
  CONST EFI_SPI_PART *SpiPart;
  EFI_STATUS Status;

  //
  // Allocate the SPI host controller data structure
  //
  SpiIo = AllocateRuntimeZeroPool (sizeof (SPI_IO));
  if (SpiIo == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate SPI_IO!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save the inputs for the shutdown operation
  //
  SpiIo->Signature = SPI_IO_SIGNATURE;
  SpiIo->SpiBus = SpiBus;

  //
  // Initialize the SPI IO protocol
  //
  SpiIo->SpiIoProtocol.SpiPeripheral = SpiPeripheral;
  SpiIo->SpiIoProtocol.OriginalSpiPeripheral = SpiPeripheral;
  SpiIo->SpiIoProtocol.LegacySpiProtocol = SpiBus->LegacySpiProtocol;
  SpiIo->SpiIoProtocol.FrameSizeSupportMask =
                      SpiBus->SpiHcProtocol->FrameSizeSupportMask
                      | SUPPORT_FRAME_SIZE_BITS (8)
                      | SUPPORT_FRAME_SIZE_BITS (16)
                      | SUPPORT_FRAME_SIZE_BITS (24)
                      | SUPPORT_FRAME_SIZE_BITS (32);
  SpiIo->SpiIoProtocol.MaximumTransferBytes =
                      SpiBus->SpiHcProtocol->MaximumTransferBytes;
  if (((SpiBus->SpiHcProtocol->Attributes & HC_SUPPORTS_2_BIT_DATA_BUS_WIDTH)
       != 0) && ((SpiPeripheral->Attributes &
       SPI_PART_SUPPORTS_2_BIT_DATA_BUS_WIDTH) != 0)) {
    SpiIo->SpiIoProtocol.Attributes |= SPI_IO_SUPPORTS_2_BIT_DATA_BUS_WIDTH;
  }
  if (((SpiBus->SpiHcProtocol->Attributes & HC_SUPPORTS_4_BIT_DATA_BUS_WIDTH)
       != 0) && ((SpiPeripheral->Attributes &
       SPI_PART_SUPPORTS_4_BIT_DATA_BUS_WIDTH) != 0)) {
    SpiIo->SpiIoProtocol.Attributes |= SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH;
  }
  if ((SpiBus->SpiHcProtocol->Attributes & HC_TRANSFER_SIZE_INCLUDES_OPCODE)
       != 0) {
    SpiIo->SpiIoProtocol.Attributes |= SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE;
  }
  if ((SpiBus->SpiHcProtocol->Attributes & HC_TRANSFER_SIZE_INCLUDES_ADDRESS)
       != 0) {
    SpiIo->SpiIoProtocol.Attributes |= SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS;
  }
  SpiIo->SpiIoProtocol.Transaction = SpiIoTransaction;
  SpiIo->SpiIoProtocol.UpdateSpiPeripheral = SpiIoUpdateSpiPeripheral;

  //
  // Build the device path for this SPI device
  //
  SpiIo->DevicePath = AppendDevicePath (SpiBus->DevicePath,
                                        &mSpiPartDevicePath.Controller.Header);
  if (SpiBus == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiIo failed to build device path!\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Failure;
  }
  mSpiPartDevicePath.Controller.ControllerNumber++;

  //
  // Create a child handle for the SPI peripheral
  //
  Handle = NULL;
  Status = SpiInstallIoProtocol (&Handle, SpiIo, SpiPeripheral);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiIo failed to install protocols!\n"));
    goto Failure;
  }

  //
  // Display the peripheral that was connected to the SPI bus
  //
  SpiPart = SpiPeripheral->SpiPart;
  if ((SpiPeripheral->FriendlyName != NULL) && (SpiPart->Vendor != NULL)
    && (SpiPart->PartNumber != NULL)) {
    DEBUG ((EFI_D_INFO, "  |\n"));
    DEBUG ((EFI_D_INFO, "  +- %s %s: %s\n", SpiPart->Vendor,
            SpiPart->PartNumber, SpiPeripheral->FriendlyName));
  }

  //
  // Connect other drivers to this handle
  //
  SpiConnectController (
                  Handle,
                  NULL,
                  NULL,
                  TRUE
                  );
  return EFI_SUCCESS;

Failure:
  //
  // Release the SPI host controller resources
  //
DEBUG((EFI_D_ERROR, "Calling SpiIoShutdown, Status: %r\n", Status));
  SpiIoShutdown (SpiIo);
  return Status;
}
