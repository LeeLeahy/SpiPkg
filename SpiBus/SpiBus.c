/** @file

  This module implements the SPI bus control.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SpiBus.h"

/**
  Enumerate the SPI devices on the bus and create an EFI_SPI_IO_PROTOCOL
  instance for each one.

  @param[in]  SpiBus            A pointer to the SPI_BUS instance.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_INVALID_PARAMETER The SpiPeripheral->SpiBus is NULL
  @retval EFI_INVALID_PARAMETER The SpiPeripheral->SpiBus pointing at wrong bus
  @retval EFI_INVALID_PARAMETER The SpiPeripheral->SpiPart is NULL
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory available to support the
                                full enumeration.
**/
EFI_STATUS
EFIAPI
SpiBusEnumerateSpiDevices (
  IN SPI_BUS *SpiBus
  )
{
  CONST EFI_SPI_BUS *BusConfig;
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral;
  EFI_STATUS Status;

  //
  // Walk the SPI peripherals on the bus
  //
  BusConfig = SpiBus->BusConfig;
  SpiPeripheral = BusConfig->PeripheralList;
  Status = EFI_SUCCESS;
  while (SpiPeripheral != NULL) {
    //
    // Validate the parameters for this SPI peripheral
    //
    if (SpiPeripheral->SpiBus == NULL) {
      DEBUG ((EFI_D_ERROR, "ERROR - SpiPeripheral->SpiBus is NULL\n"));
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    if (SpiPeripheral->SpiBus != SpiBus->BusConfig) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - SpiPeripheral->SpiBus pointing at wrong SPI bus!\n"));
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    if (SpiPeripheral->SpiPart == NULL) {
      DEBUG ((EFI_D_ERROR, "ERROR - SpiPeripheral->SpiPart is NULL\n"));
      Status = EFI_INVALID_PARAMETER;
      break;
    }

    //
    // Create an EFI_SPI_IO_PROTOCOL instance for this SPI peripheral
    //
    Status = SpiIoStartup (SpiBus, SpiPeripheral);
    if (EFI_ERROR(Status)) {
      break;
    }

    //
    // Locate the next SPI peripheral on this bus
    //
    SpiPeripheral = SpiPeripheral->NextSpiPeripheral;
  }
  return Status;
}

/**
  Release the buffers allocated during the call to SpiBusSetupBuffers

  This routine must be called at TPL_NOTIFY.

  @param[in]  SpiBus            Pointer to a SPI_BUS structure.
  @param[in]  Status            SPI transaction status

**/
VOID
EFIAPI
SpiBusReleaseBuffers (
  IN SPI_BUS *SpiBus,
  IN EFI_STATUS Status
  )
{
  EFI_SPI_BUS_TRANSACTION *BusTransaction;
  SPI_IO_TRANSACTION *IoTransaction;
  UINT8 *ReadBuffer;
  UINT32 ReadBytes;
  UINT8 *ReceiveBuffer;
  UINT32 ReceiveBytes;
  UINT32 ReceiveDataProcessing;

  //
  // Locate the data structures
  //
  IoTransaction = &SpiBus->IoTransaction;
  BusTransaction = &IoTransaction->BusTransaction;

  //
  // Perform the receive data copy or frame conversion operations
  //
  ReceiveDataProcessing = IoTransaction->SetupFlags
                        & (SETUP_FLAG_COPY_READ_DATA
                           | SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_16
                           | SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_24
                           | SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_32);
  ReadBytes = IoTransaction->ReadBytes;
  ReadBuffer = IoTransaction->ReadBuffer;
  ReceiveBuffer = BusTransaction->ReadBuffer;
  ReceiveBytes = BusTransaction->ReadBytes;
  if (ReceiveDataProcessing & SETUP_FLAG_COPY_READ_DATA) {
    ReceiveBuffer += IoTransaction->WriteBytes;
    ReceiveBytes -= IoTransaction->WriteBytes;
  }
  switch (ReceiveDataProcessing) {
  case SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_16 | SETUP_FLAG_COPY_READ_DATA:
  case SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_16:
    //
    // Convert the data from big-endian to little-endian
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Converting 0x%08x bytes of 8-bit frames at 0x%08x\n",
              ReceiveBytes,
              ReceiveBuffer));
      DEBUG ((EFI_D_ERROR,
              "SpiBus: into 0x%08x bytes of 16-bit frames at 0x%08x\n",
              ReadBytes,
              ReadBuffer));
    }
    if (!EFI_ERROR(Status)) {
      while (ReadBytes > 0) {
        ReadBuffer[0] = ReceiveBuffer[1];
        ReadBuffer[1] = ReceiveBuffer[0];
        ReceiveBuffer += 2;
        ReadBuffer += 2;
        ReadBytes -= 2;
      }
    }
    break;

  case SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_24 | SETUP_FLAG_COPY_READ_DATA:
  case SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_24:
    //
    // Convert the data from big-endian to little-endian
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Converting 0x%08x bytes of 8-bit frames at 0x%08x\n",
              ReceiveBytes,
              ReceiveBuffer));
      DEBUG ((EFI_D_ERROR,
              "SpiBus: into 0x%08x bytes of 24-bit frames at 0x%08x\n",
              ReadBytes,
              ReadBuffer));
    }
    if (!EFI_ERROR(Status)) {
      while (ReadBytes > 0) {
        ReadBuffer[0] = ReceiveBuffer[2];
        ReadBuffer[1] = ReceiveBuffer[1];
        ReadBuffer[2] = ReceiveBuffer[0];
        ReadBuffer[3] = 0;
        ReceiveBuffer += 3;
        ReadBuffer += 4;
        ReadBytes -= 4;
      }
    }
    break;

  case SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_32 | SETUP_FLAG_COPY_READ_DATA:
  case SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_32:
    //
    // Convert the data from big-endian to little-endian
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Converting 0x%08x bytes of 8-bit frames at 0x%08x\n",
              ReceiveBytes,
              ReceiveBuffer));
      DEBUG ((EFI_D_ERROR,
              "SpiBus: into 0x%08x bytes of 32-bit frames at 0x%08x\n",
              ReadBytes,
              ReadBuffer));
    }
    if (!EFI_ERROR(Status)) {
      while (ReadBytes > 0) {
        ReadBuffer[0] = ReceiveBuffer[3];
        ReadBuffer[1] = ReceiveBuffer[2];
        ReadBuffer[2] = ReceiveBuffer[1];
        ReadBuffer[3] = ReceiveBuffer[0];
        ReceiveBuffer += 4;
        ReadBuffer += 4;
        ReadBytes -= 4;
      }
    }
    break;

  case SETUP_FLAG_COPY_READ_DATA:
    //
    // Copy the read data into the original buffer
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
      "SpiBus: Copying 0x%08x bytes of received data from 0x%08x into 0x%08x\n",
              ReadBytes,
              ReceiveBuffer,
              ReadBuffer));
    }
    if (!EFI_ERROR(Status)) {
      CopyMem (ReadBuffer, ReceiveBuffer, ReadBytes);
    }
    break;
  }

  //
  // Release the allocated buffers
  //
  if ((IoTransaction->SetupFlags & SETUP_FLAG_DISCARD_WRITE_BUFFER) != 0) {
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiBus: Freeing WriteBuffer at 0x%08x\n",
              BusTransaction->WriteBuffer));
    }
    FreePool (BusTransaction->WriteBuffer);
  }
  if ((IoTransaction->SetupFlags & SETUP_FLAG_DISCARD_READ_BUFFER) != 0) {
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiBus: Freeing ReadBuffer at 0x%08x\n",
              BusTransaction->ReadBuffer));
    }
    FreePool (BusTransaction->ReadBuffer);
  }
}

/**
  Start the SPI transaction on the SPI host controller.

  This routine must be called at TPL_NOTIFY.

  @param[in]  SpiBus            Pointer to a SPI_BUS structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI transaction completed successfully
  @retval EFI_BAD_BUFFER_SIZE   The WriteBytes value was invalid.
  @retval EFI_BAD_BUFFER_SIZE   The ReadBytes value was invalid.
  @retval EFI_INVALID_PARAMETER BusWidth not supported by SPI peripheral or
                                SPI host controller
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for SPI transaction
  @retval EFI_UNSUPPORTED       The FrameSize is not supported by the SPI
                                bus layer or the SPI host controller.
  @retval EFI_UNSUPPORTED       The SPI controller was not able to support the
                                frequency requested by ClockHz
**/
EFI_STATUS
EFIAPI
SpiBusTransaction (
  IN SPI_BUS *SpiBus
  )
{
  CONST EFI_SPI_BUS *BusConfig;
  EFI_SPI_BUS_TRANSACTION *BusTransaction;
  UINT32 ClockFrequency;
  SPI_IO_TRANSACTION *IoTransaction;
  BOOLEAN PinValue;
  CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol;
  CONST EFI_SPI_PART *SpiPart;
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral;
  EFI_STATUS Status;
  EFI_STATUS TempStatus;

  //
  // Validate the inputs
  //
  ASSERT (SpiBus != NULL);

  //
  // Locate the data structures
  //
  IoTransaction = &SpiBus->IoTransaction;
  BusTransaction = &IoTransaction->BusTransaction;
  BusConfig = SpiBus->BusConfig;
  SpiHcProtocol = SpiBus->SpiHcProtocol;
  SpiPeripheral = BusTransaction->SpiPeripheral;

  //
  // Validate the data structures
  //
  ASSERT (IoTransaction->SpiIo != NULL);
  ASSERT (IoTransaction->BusTransaction.SpiPeripheral != NULL);
  ASSERT (SpiPeripheral != NULL);
  ASSERT (BusConfig != NULL);
  ASSERT (SpiHcProtocol != NULL);

  SpiPart = SpiPeripheral->SpiPart;
  ASSERT (SpiPart != NULL);

  //
  // Each SPI transaction is performed in the following steps:
  //
  //  1.  Set up the clock for the transaction
  //  2.  Select the chip
  //  3.  Perform the data transfer
  //  4.  Deselect the chip
  //  5.  Stop the clock
  //

  if (BusTransaction->DebugTransaction) {
    DEBUG ((EFI_D_ERROR, "SpiBus: IoTransaction 0x%08x starting\n",
            IoTransaction));
  }

  //--------------------------------------------------
  //  1.  Set up the clock for the transaction
  //--------------------------------------------------

  //
  // Get the maximum frequency that the chip supports
  //
  ClockFrequency = SpiPart->MaxClockHz;
  if ((SpiPeripheral->MaxClockHz != 0)
    && (ClockFrequency > SpiPeripheral->MaxClockHz)) {
    ClockFrequency = SpiPeripheral->MaxClockHz;
  }

  //
  // Reduce this frequency on an operation specific basis
  //
  if ((IoTransaction->ClockHz != 0 )
    && (IoTransaction->ClockHz < ClockFrequency)) {
    ClockFrequency = IoTransaction->ClockHz;
  }

  //
  // Display the clock set up if requested
  //
  if (BusTransaction->DebugTransaction) {
    DEBUG ((EFI_D_ERROR, "SpiBus: Requested SCLK Frequency: %d.%03d MHz\n",
           ClockFrequency / 1000000, (ClockFrequency % 1000000) / 1000));
  }

  //
  // Select the proper clock frequency, polarity and phase
  //
  if (BusConfig->Clock != NULL) {
    Status = BusConfig->Clock (SpiPeripheral, &ClockFrequency);
  } else {
    Status = SpiHcProtocol->Clock(SpiHcProtocol, SpiPeripheral,
             &ClockFrequency);
  }
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
           "ERROR - SpiBus failed to set the clock frequency\n"));
    goto TransactionFailure;
  }
  IoTransaction->SetupFlags |= SETUP_FLAG_CLOCK_RUNNING;

  //
  // Display the clock set up if requested
  //
  if (BusTransaction->DebugTransaction) {
    DEBUG ((EFI_D_ERROR, "SpiBus: SCLK Frequency: %d.%06d MHz\n",
           ClockFrequency / 1000000, ClockFrequency % 1000000));
    DEBUG ((EFI_D_ERROR, "SpiBus: SCLK Polarity: %d\n",
           SpiPeripheral->ClockPolarity ? 1 : 0));
    DEBUG ((EFI_D_ERROR, "SpiBus: SCLK Phase: %d\n",
           SpiPeripheral->ClockPhase ? 1 : 0));
  }

  //
  // Verify the minimum clock frequency
  //
  if ((ClockFrequency < SpiPart->MinClockHz) || (ClockFrequency == 0)) {
    DEBUG ((EFI_D_ERROR,
           "ERROR - SCLK < minimum clock frequency\n"));
    Status = EFI_UNSUPPORTED;
    goto TransactionFailure;
  }

  //--------------------------------------------------
  //  2.  Select the chip
  //--------------------------------------------------

  //
  // Determine the proper pin value to assert chip select
  //
  PinValue = SpiPart->ChipSelectPolarity;
  if (BusTransaction->DebugTransaction) {
    DEBUG ((EFI_D_ERROR, "SpiBus: Assert chip select: %d\n", PinValue));
  }

  //
  // Assert the chip select
  //
  if (SpiPeripheral->ChipSelect != NULL) {
    Status = SpiPeripheral->ChipSelect (SpiPeripheral, PinValue);
  } else {
    Status = SpiHcProtocol->ChipSelect (SpiHcProtocol, SpiPeripheral, PinValue);
  }

  //
  // Verify that the chip was properly selected
  //
  if (EFI_ERROR(Status)) {
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - Chip select failure, Status: %r\n", Status));
    }
    goto TransactionFailure;
  }
  IoTransaction->SetupFlags |= SETUP_FLAG_CHIP_SELECTED;

  //--------------------------------------------------
  //  3.  Perform the data transfer
  //--------------------------------------------------

  //
  // Use the SPI host controller to perform the transaction
  //
  if (BusTransaction->DebugTransaction) {
    DEBUG ((EFI_D_ERROR,
            "SpiBus: SPI transaction handed to host controller\n"));
  }
  Status = SpiHcProtocol->Transaction (
                SpiHcProtocol,
                BusTransaction
                );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiBus failed the SPI transaction!\n"));
  }

  //
  // Release any buffers allocated to support this transaction
  //
  SpiBusReleaseBuffers (SpiBus, Status);

  //--------------------------------------------------
  //  4.  Deassert the chip select
  //--------------------------------------------------

TransactionFailure:
  //
  // Deassert chip select if necessary
  //
  if ((IoTransaction->SetupFlags & SETUP_FLAG_CHIP_SELECTED) != 0) {
    //
    // Determine the proper pin value to deassert chip select
    //
    PinValue = !SpiPart->ChipSelectPolarity;

    //
    // Deassert the chip select
    //
    if (SpiPeripheral->ChipSelect != NULL) {
      SpiPeripheral->ChipSelect (SpiPeripheral, PinValue);
    } else {
      SpiHcProtocol->ChipSelect (SpiHcProtocol, SpiPeripheral, PinValue);
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiBus: Deasserted chip select: %d\n", PinValue));
    }
  }

  //--------------------------------------------------
  //  5.  Stop the clock
  //--------------------------------------------------

  //
  // Turn off the clock if necessary
  //
  if ((IoTransaction->SetupFlags & SETUP_FLAG_CLOCK_RUNNING) != 0) {
    //
    // Turn off the clock
    //
    ClockFrequency = 0;
    if (BusConfig->Clock != NULL) {
      TempStatus = BusConfig->Clock (SpiPeripheral, &ClockFrequency);
    } else {
      TempStatus = SpiHcProtocol->Clock(SpiHcProtocol, SpiPeripheral,
                                    &ClockFrequency);
    }
    if (EFI_ERROR(TempStatus)) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - SpiBus failed to turn off the clock, Status: %r\n",
              TempStatus));
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiBus: SCLK stopped\n"));
    }
  }

  //
  // Return the SPI transaction status
  //
  return Status;
}

/**
  Converts the transmit frames to 8-bits/frame if necessary

  This routine must be called at TPL_NOTIFY.

  Not all SPI controllers support all frame sizes.  When they don't the SPI bus
  layer converts the frame size to 8-bits/frame.  This requires allocating a
  buffer and reformatting the data passed from the SPI peripheral layer into
  the new buffer.  The SPI peripheral layer passes data in little-endian format
  in the buffer.  The SPI device transmits and receives data most-significant
  bit first.  Thus converting from 16-bit, 24-bit, or 32-bit, the data must be
  placed into the buffer in big-endian format.  Additionally the conversion from
  24-bit reduces the transmit and receive buffer sizes by 25%.

  The SpiBusCompleteTransaction routine is responsible for doing the conversion
  on the receive data back to 16-bits, 24-bits or 32-bits per frame.

  @param[in]  IoTransaction     Pointer to an IO_TRANSACTION structure.
  @param[in]  AllocateBuffers   TRUE if buffers needs to be allocated, FALSE
                                if the buffers were already allocated.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The frame-size is supported by the SPI host
                                controller.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for the transfer operation.

**/
EFI_STATUS
EFIAPI
ConvertTransmitFrames (
  IN SPI_IO_TRANSACTION *IoTransaction,
  IN BOOLEAN AllocateBuffers
  )
{
  UINT32 AlignmentMask;
  UINT32 BufferLength;
  EFI_SPI_BUS_TRANSACTION *BusTransaction;
  UINTN Data;
  UINT32 FrameSize;
  UINT8 *NewBuffer;
  union {
    UINT8 *U8;
    UINT16 *U16;
    UINT32 *U32;
  } PreviousBuffer;
  SPI_BUS *SpiBus;
  CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol;

  //
  // Locate the data structures
  //
  BusTransaction = &IoTransaction->BusTransaction;
  SpiBus = IoTransaction->SpiIo->SpiBus;
  SpiHcProtocol = SpiBus->SpiHcProtocol;
  FrameSize = BusTransaction->FrameSize;

  //
  // Determine if the SPI host controller supports the requested frame size
  // Note the SPI host controller must always support 8-bits/frame
  //
  if (((SpiHcProtocol->FrameSizeSupportMask & (1 << (FrameSize - 1))) != 0)
       || (FrameSize == 8)) {
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiBus: %d-bits/frame supported by SPI host controller\n",
              BusTransaction->FrameSize));
    }
    return EFI_SUCCESS;
  }
  if (BusTransaction->DebugTransaction) {
    DEBUG ((EFI_D_ERROR,
            "SpiBus: %d-bits/frame not supported by SPI host controller\n",
            BusTransaction->FrameSize));
  }

  //
  // Allocate the buffers if necessary
  //
  PreviousBuffer.U8 = BusTransaction->WriteBuffer;
  if (AllocateBuffers) {
    if (BusTransaction->WriteBuffer != NULL) {
      if (BusTransaction->ReadBuffer != NULL) {
        //
        // Save the SPI peripheral layer buffer values
        //
        IoTransaction->WriteBytes = BusTransaction->WriteBytes;
        IoTransaction->ReadBytes = BusTransaction->ReadBytes;
        IoTransaction->ReadBuffer = BusTransaction->ReadBuffer;
        BufferLength = BusTransaction->WriteBytes + BusTransaction->ReadBytes;

        //
        // Allocate the write and read buffers
        //
        AlignmentMask = 8 - 1;
        BusTransaction->WriteBuffer = AllocateRuntimePool (BufferLength
                                                    + AlignmentMask);
        if (BusTransaction->WriteBuffer == NULL) {
          if (BusTransaction->DebugTransaction) {
            DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate WriteBuffer!\n"));
          }
          return EFI_OUT_OF_RESOURCES;
        }

        //
        // Split the buffers
        //
        BusTransaction->ReadBuffer = BusTransaction->WriteBuffer
                        + ((BufferLength + AlignmentMask) & (~AlignmentMask));
        if (BusTransaction->DebugTransaction) {
          DEBUG ((EFI_D_ERROR, "SpiBus: Allocated WriteBuffer at 0x%08x\n",
                  BusTransaction->WriteBuffer));
        }
        if (BusTransaction->DebugTransaction) {
          DEBUG ((EFI_D_ERROR, "SpiBus: Using ReadBuffer at 0x%08x\n",
                  BusTransaction->ReadBuffer));
        }

        //
        // Indicate to the completion routine how to do the buffer clean up
        //
        IoTransaction->SetupFlags |= SETUP_FLAG_DISCARD_WRITE_BUFFER;
      } else {
        //
        // Allocate the write buffer
        //
        BufferLength = BusTransaction->WriteBytes;
        BusTransaction->WriteBuffer = AllocateRuntimePool (BufferLength);
        if (BusTransaction->WriteBuffer == NULL) {
          if (BusTransaction->DebugTransaction) {
            DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate WriteBuffer!\n"));
          }
          return EFI_OUT_OF_RESOURCES;
        }
        if (BusTransaction->DebugTransaction) {
          DEBUG ((EFI_D_ERROR, "SpiBus: Allocated WriteBuffer at 0x%08x\n",
                  BusTransaction->WriteBuffer));
        }

        //
        // Indicate to the completion routine how to do the buffer clean up
        //
        IoTransaction->SetupFlags |= SETUP_FLAG_DISCARD_WRITE_BUFFER;
      }
    } else {
      //
      // Save the SPI peripheral layer buffer values
      //
      IoTransaction->ReadBytes = BusTransaction->ReadBytes;
      IoTransaction->ReadBuffer = BusTransaction->ReadBuffer;
      BufferLength = BusTransaction->WriteBytes + IoTransaction->ReadBytes;

      //
      // Allocate the read buffer
      //
      BufferLength = BusTransaction->ReadBytes;
      BusTransaction->ReadBuffer = AllocateRuntimePool (BusTransaction->WriteBytes);
      if (BusTransaction->ReadBuffer == NULL) {
        if (BusTransaction->DebugTransaction) {
          DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate ReadBuffer!\n"));
        }
        return EFI_OUT_OF_RESOURCES;
      }
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "SpiBus: Allocated ReadBuffer at 0x%08x\n",
                BusTransaction->ReadBuffer));
      }

      //
      // Indicate to the completion routine how to do the buffer clean up
      //
      IoTransaction->SetupFlags |= SETUP_FLAG_DISCARD_READ_BUFFER;
    }
  }

  //
  // Perform the frame conversion
  //
  if (BusTransaction->DebugTransaction) {
    DEBUG ((EFI_D_ERROR,
            "SpiBus: Converting from %d-bits/frame to 8-bits/frame\n",
            FrameSize));
  }
  BufferLength = BusTransaction->WriteBytes;
  NewBuffer = BusTransaction->WriteBuffer;
  BusTransaction->FrameSize = 8;
  switch (FrameSize) {
  case 16:
    //
    // Indicate to the completion routine how to do the frame conversion
    //
    if (IoTransaction->ReadBytes != 0) {
      IoTransaction->SetupFlags |= SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_16;
    }

    //
    // Convert the data from little-endian to big-endian
    //
    while (BufferLength > 0) {
      Data = *PreviousBuffer.U16++;
      *NewBuffer++ = (UINT8)(Data >> 8);
      *NewBuffer++ = (UINT8)Data;
      BufferLength -= 2;
    }
    break;

  case 24:
    //
    // Indicate to the completion routine how to do the frame conversion
    //
    if (IoTransaction->ReadBytes != 0) {
      IoTransaction->SetupFlags |= SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_24;
    }

    //
    // Convert the data from little-endian to big-endian
    //
    while (BufferLength > 0) {
      Data = *PreviousBuffer.U32++;
      *NewBuffer++ = (UINT8)(Data >> 16);
      *NewBuffer++ = (UINT8)(Data >> 8);
      *NewBuffer++ = (UINT8)Data;
      BufferLength -= 4;
    }

    //
    // Reduce the transmit size
    //
    BusTransaction->WriteBytes -= BusTransaction->WriteBytes / 4;
    IoTransaction->WriteBytes -= IoTransaction->WriteBytes / 4;
    break;

  case 32:
    //
    // Indicate to the completion routine how to do the frame conversion
    //
    if (IoTransaction->ReadBytes != 0) {
      IoTransaction->SetupFlags |= SETUP_FLAG_CONVERT_FRAME_BITS_8_TO_32;
    }

    //
    // Convert the data from little-endian to big-endian
    //
    while (BufferLength > 0) {
      Data = *PreviousBuffer.U32++;
      *NewBuffer++ = (UINT8)(Data >> 24);
      *NewBuffer++ = (UINT8)(Data >> 16);
      *NewBuffer++ = (UINT8)(Data >> 8);
      *NewBuffer++ = (UINT8)Data;
      BufferLength -= 4;
    }
    break;
  }
  return EFI_SUCCESS;
}

/**
  Setup the buffers for the SPI transfer.

  This routine must be called at TPL_NOTIFY.

  Not all SPI controllers support all modes of operations.  When they don't
  the SPI bus layer converts the transaction from the requested type into
  a full-duplex transfer which must be supported on all SPI controllers.  This
  is done by allocating additional buffers and copying data into and out of
  these buffers.  These buffers are then released upon the completion of the
  SPI transaction.

  @param[in]  SpiBus            Pointer to a SPI_BUS structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The buffers are ready for the SPI transaction.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for the transfer operation.

**/
EFI_STATUS
EFIAPI
SpiBusSetupBuffers (
  IN SPI_BUS *SpiBus
  )
{
  UINT32 AlignmentMask;
  UINT32 BufferLength;
  EFI_SPI_BUS_TRANSACTION *BusTransaction;
  SPI_IO_TRANSACTION *IoTransaction;
  CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol;
  UINT8 *WriteBuffer;

  //
  // Validate the inputs
  //
  ASSERT (SpiBus != NULL);
  IoTransaction = &SpiBus->IoTransaction;
  ASSERT (IoTransaction->SpiIo != NULL);

  //
  // Locate the data structures
  //
  BusTransaction = &IoTransaction->BusTransaction;
  SpiBus = IoTransaction->SpiIo->SpiBus;
  ASSERT (SpiBus != NULL);
  SpiHcProtocol = SpiBus->SpiHcProtocol;

  //
  // Save the requested number of write bytes in for write-then-read and
  // 24-bit to 8-bit frame conversion
  //
  IoTransaction->WriteBytes = BusTransaction->WriteBytes;

  //
  // Determine if the SPI host controller supports the operation type
  //
  switch (BusTransaction->TransactionType) {
  case SPI_TRANSACTION_FULL_DUPLEX:
    //
    // Since SPI is naturally a full-duplex interface, all SPI host controllers
    // must support full-duplex transactions.
    //
    return ConvertTransmitFrames (IoTransaction, TRUE);

  case SPI_TRANSACTION_WRITE_ONLY:
    //
    // Data flowing from the host to the SPI peripheral.  ReadBytes must be
    // zero.  WriteBytes must be non-zero and WriteBuffer must be provided.
    //
    if ((SpiHcProtocol->Attributes & HC_SUPPORTS_WRITE_ONLY_OPERATIONS) != 0) {
      return ConvertTransmitFrames (IoTransaction, TRUE);
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Transaction not supported by SPI host controller\n"));
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Converting request to full-duplex SPI transaction\n"));
    }

    //
    // Convert this to a full-duplex operation by allocating a read-buffer
    // of the same length.  The data read into the read buffer will be discarded
    // at the end of the SPI transaction.  The original ReadBytes value is
    // already in the IoTransaction.
    //
    BusTransaction->ReadBuffer = AllocateRuntimePool (BusTransaction->WriteBytes);
    if (BusTransaction->ReadBuffer == NULL) {
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate ReadBuffer!\n"));
      }
      return EFI_OUT_OF_RESOURCES;
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiBus: Allocated ReadBuffer at 0x%08x\n",
              BusTransaction->ReadBuffer));
    }

    //
    // Finish converting this to a full-duplex transaction
    //
    IoTransaction->SetupFlags |= SETUP_FLAG_DISCARD_READ_BUFFER;
    BusTransaction->ReadBytes = BusTransaction->WriteBytes;
    BusTransaction->TransactionType = SPI_TRANSACTION_FULL_DUPLEX;
    break;

  case SPI_TRANSACTION_READ_ONLY:
    //
    // Data flowing from the SPI peripheral to the host.  WriteBytes must be
    // zero.  ReadBytes must be non-zero and ReadBuffer must be provided.
    //
    if ((SpiHcProtocol->Attributes & HC_SUPPORTS_READ_ONLY_OPERATIONS) != 0) {
      return ConvertTransmitFrames (IoTransaction, TRUE);
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Transaction not supported by SPI host controller\n"));
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Converting request to full-duplex SPI transaction\n"));
    }

    //
    // Convert this to a full-duplex operation by allocating a write-buffer
    // of the same length.  The write data will be all zeros and the buffer
    // will be discarded at the end of the SPI transaction.
    //
    BusTransaction->WriteBuffer = AllocateRuntimeZeroPool (BusTransaction->ReadBytes);
    if (BusTransaction->WriteBuffer == NULL) {
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate WriteBuffer!\n"));
      }
      return EFI_OUT_OF_RESOURCES;
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiBus: Allocated WriteBuffer at 0x%08x\n",
              BusTransaction->WriteBuffer));
    }

    //
    // Finish converting this to a full-duplex transaction
    //
    IoTransaction->SetupFlags |= SETUP_FLAG_DISCARD_WRITE_BUFFER;
    BusTransaction->WriteBytes = BusTransaction->ReadBytes;
    BusTransaction->TransactionType = SPI_TRANSACTION_FULL_DUPLEX;
    break;

  case SPI_TRANSACTION_WRITE_THEN_READ:
    //
    // Data first flowing from the host to the SPI peripheral and then data
    // flows from the SPI peripheral to the host.  These types of operations
    // get used for SPI flash devices when control data (opcode, address) must
    // be passed to the SPI peripheral to specify the data to be read.
    //
    if ((SpiHcProtocol->Attributes & HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS)
         != 0) {
      return ConvertTransmitFrames (IoTransaction, TRUE);
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Transaction not supported by SPI host controller\n"));
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Converting request to full-duplex SPI transaction\n"));
    }

    //
    // Save the caller's buffers and lengths
    //
    WriteBuffer = BusTransaction->WriteBuffer;
    IoTransaction->ReadBytes = BusTransaction->ReadBytes;
    IoTransaction->ReadBuffer = BusTransaction->ReadBuffer;

    //
    // Convert this to a full-duplex operation by allocating both a read and
    // write buffer of the total transfer length.  Extra write data will be
    // zeros.
    //
    BufferLength = IoTransaction->WriteBytes + IoTransaction->ReadBytes;
    AlignmentMask = 8 - 1;
    BusTransaction->WriteBuffer = AllocateRuntimePool ((BufferLength * 2)
                                                    + AlignmentMask);
    if (BusTransaction->WriteBuffer == NULL) {
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate WriteBuffer!\n"));
      }
      return EFI_OUT_OF_RESOURCES;
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiBus: Allocated WriteBuffer at 0x%08x\n",
              BusTransaction->WriteBuffer));
    }

    //
    // Copy the write data into the new buffer
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
         "SpiBus: Copying 0x%08x bytes of write data from 0x%08x into 0x%08x\n",
         IoTransaction->WriteBytes,
         WriteBuffer,
         BusTransaction->WriteBuffer));
    }
    CopyMem (BusTransaction->WriteBuffer,
             WriteBuffer,
             IoTransaction->WriteBytes);

    //
    // Zero the additional bytes in the write buffer
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiBus: Zeroing 0x%08x bytes of write data at 0x%08x\n",
              IoTransaction->ReadBytes,
              BusTransaction->WriteBuffer + IoTransaction->WriteBytes));
    }
    ZeroMem (BusTransaction->WriteBuffer + IoTransaction->WriteBytes,
             IoTransaction->ReadBytes);

    //
    // Finish converting this to a full-duplex transaction
    //
    IoTransaction->SetupFlags |= SETUP_FLAG_DISCARD_WRITE_BUFFER
                               | SETUP_FLAG_COPY_READ_DATA;
    BusTransaction->ReadBuffer = BusTransaction->WriteBuffer
                    + ((BufferLength + AlignmentMask) & (~AlignmentMask));
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiBus: Using ReadBuffer at 0x%08x\n",
              BusTransaction->ReadBuffer));
    }
    BusTransaction->WriteBytes = BufferLength;
    BusTransaction->ReadBytes = BufferLength;
    BusTransaction->TransactionType = SPI_TRANSACTION_FULL_DUPLEX;
    break;
  }
  return ConvertTransmitFrames (IoTransaction, FALSE);
}

/**
  Shuts down the SPI bus layer.

  This routine must be called at or below TPL_NOTIFY.

  This routine deallocates the resources supporting the SPI bus operation.

  @param[in]  SpiBus            Pointer to a SPI_BUS data structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
STATIC
VOID
EFIAPI
SpiBusShutdown (
  IN SPI_BUS *SpiBus
  )
{
  SPI_BUS *SpiBusLayerTag;
  EFI_STATUS Status;

  //
  // Determine if the job is already done
  //
  if (SpiBus != NULL) {
    //
    // Release the SPI HC protocol
    //
    if (SpiBus->SpiHcProtocol != NULL) {
      SpiCloseProtocol (
             SpiBus->ControllerHandle,
             gSpiHcProtocolGuid,
             gImageHandle,
             NULL
             );
    }

    //
    // Determine if the SPI bus layer tag is present
    //
    Status = SpiOpenProtocol (
                    SpiBus->ControllerHandle,
                    &gSpiBusLayerGuid,
                    (VOID **) &SpiBusLayerTag,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (Status == EFI_SUCCESS) {
      //
      // Remove the SPI bus layer tag
      //
      Status = SpiUninstallBusProtocol (
                       SpiBus->ControllerHandle,
                       SpiBusLayerTag
                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - SpiBus failed to remove SPI bus layer tag!\n"));
        ASSERT_EFI_ERROR(Status);
      }
    }

    //
    // Free the data structure
    //
    FreePool (SpiBus);
  }
}

/**
  Set up the necessary pieces to start SPI bus layer this device.

  This routine must be called at or below TPL_NOTIFY.

  Initialize the context data structure to support the SPI bus layer.  Gain
  access to the SPI HC protocol.  Upon successful completion, install the SPI
  IO protocols for each of the child controllers.

  @param[in]  ControllerHandle  Handle containing the EFI_SPI_HC_PROTOCOL
  @param[in]  SpiHcProtocol     Pointer to the EFI_SPI_HC_PROTOCOL

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI host controller was started successfully
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
**/
EFI_STATUS
EFIAPI
SpiBusStartup (
  IN EFI_HANDLE ControllerHandle,
  IN CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol
  )
{
  SPI_BUS *SpiBus;
  EFI_STATUS Status;

  //
  // Allocate the SPI host controller data structure
  //
  SpiBus = AllocateRuntimeZeroPool (sizeof (SPI_BUS));
  if (SpiBus == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate SPI_BUS!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save the inputs for the shutdown operation
  //
  SpiBus->ControllerHandle = ControllerHandle;
  SpiBus->SpiHcProtocol = SpiHcProtocol;

  //
  // Get access to the legacy SPI controller protocol
  //
  Status = SpiOpenProtocol (
                  ControllerHandle,
                  gLegacySpiControllerProtocolGuid,
                  (VOID **)&SpiBus->LegacySpiProtocol,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_EXCLUSIVE
                  );
  if (EFI_ERROR (Status)) {
    SpiBus->LegacySpiProtocol = NULL;
  }

  //
  // Connect the SPI bus to the host controller
  //
  Status = SpiBusConnectHc (SpiBus);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - SpiBus: Host controller not found in board database!\n"));
    goto Failure;
  }

  //
  // Display the rest of the host controller attributes
  //
  SpiHcProtocol = SpiBus->SpiHcProtocol;
  if (SpiBus->LegacySpiProtocol) {
    DEBUG ((EFI_D_INFO, "  | Legacy SPI Host Controller\n"));
  }
  DEBUG ((EFI_D_INFO, "  | Supported transaction types:\n"));
  DEBUG ((EFI_D_INFO, "  |   Full-duplex\n"));
  if ((SpiHcProtocol->Attributes & HC_SUPPORTS_WRITE_ONLY_OPERATIONS) != 0) {
    DEBUG ((EFI_D_INFO, "  |   Write-only\n"));
  }
  if ((SpiHcProtocol->Attributes & HC_SUPPORTS_READ_ONLY_OPERATIONS) != 0) {
    DEBUG ((EFI_D_INFO, "  |   Read-only\n"));
  }
  if ((SpiHcProtocol->Attributes & HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS)
       != 0) {
    DEBUG ((EFI_D_INFO, "  |   Write-then-read\n"));
  }
  DEBUG ((EFI_D_INFO, "  | Frame Justification\n"));
  DEBUG ((EFI_D_INFO, "  |   Transmit frame in %a significant bits\n",
          (SpiHcProtocol->Attributes & HC_TX_FRAME_IN_MOST_SIGNIFICANT_BITS)
          ? "most" : "least"));
  DEBUG ((EFI_D_INFO, "  |   Receive frame in %a significant bits\n",
          (SpiHcProtocol->Attributes & HC_RX_FRAME_IN_MOST_SIGNIFICANT_BITS)
          ? "most" : "least"));
  DEBUG ((EFI_D_INFO, "  | 0x%08x: Frame size support mask\n",
          SpiHcProtocol->FrameSizeSupportMask));
  DEBUG ((EFI_D_INFO, "  | Bus width support\n"));
  DEBUG ((EFI_D_INFO, "  |   1-bit data bus\n"));
  if ((SpiHcProtocol->Attributes & HC_SUPPORTS_2_BIT_DATA_BUS_WIDTH) != 0) {
    DEBUG ((EFI_D_INFO, "  |   2-bit data bus\n"));
  }
  if ((SpiHcProtocol->Attributes & HC_SUPPORTS_4_BIT_DATA_BUS_WIDTH) != 0) {
    DEBUG ((EFI_D_INFO, "  |   4-bit data bus\n"));
  }
  if ((SpiHcProtocol->Attributes & HC_TRANSFER_SIZE_INCLUDES_OPCODE) == 0) {
    DEBUG ((EFI_D_INFO, "  |             Opcode byte +\n"));
  }
  if ((SpiHcProtocol->Attributes & HC_TRANSFER_SIZE_INCLUDES_ADDRESS) == 0) {
    DEBUG ((EFI_D_INFO, "  |             Three address bytes +\n"));
  }
  DEBUG ((EFI_D_INFO, "  | 0x%08x: Maximum transfer size in bytes\n",
          SpiHcProtocol->MaximumTransferBytes));
  DEBUG ((EFI_D_INFO, "  |\n"));

  //
  // Verify the MaximumTransferSize
  //
  ASSERT (SpiHcProtocol->MaximumTransferBytes != 0);

  //
  // Install the SPI bus layer tag
  //
  Status = SpiInstallBusProtocol(&ControllerHandle, &SpiBus);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - SpiBus failed to install SPI bus layer tag!\n"));
    goto Failure;
  }

  //
  // Enumerate the SPI devices
  //
  SpiBusEnumerateSpiDevices (SpiBus);
  return EFI_SUCCESS;

Failure:
  //
  // Release the SPI host controller resources
  //
  SpiBusShutdown (SpiBus);
  return Status;
}
