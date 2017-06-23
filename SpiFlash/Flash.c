/** @file

  This module implements the SPI NOR flash protocol interface.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SpiFlash.h"

VOID *gFlashProtocolRegistration;

/**
  Read the 3 byte manufacture and device ID from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine reads the 3 byte manufacture and device ID from the flash part
  filling the buffer provided.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[out] Buffer            Pointer to a 3 byte buffer to receive the
                                manufacture and device ID.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The manufacture and device ID was read
                                successfully.
  @retval EFI_INVALID_PARAMETER Buffer is NULL
  @retval EFI_DEVICE_ERROR      Invalid data received from SPI flash part.
**/
EFI_STATUS
EFIAPI
FlashGetFlashId (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  OUT UINT8 *Buffer
  )
{
  UINT8 Command;
  UINT8 DeviceId[3];
  FLASH *Flash;
  UINTN Index;
  UINT32 ReadFrequency;
  EFI_STATUS Status;

  //
  // Validate the inputs
  //
  if (Buffer == NULL) {
    DEBUG((EFI_D_ERROR, "ERROR - Buffer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the driver data structure
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);

  //
  // Get the manufacture and device ID from the SPI NOR flash part
  //
  Command = SPI_NOR_READ_MANUFACTURE_ID;

  //
  // Discard the first result
  //
  ReadFrequency = MHz(1);
  Buffer[0] = 0;
  Buffer[1] = 0;
  Buffer[2] = 0;
  for (Index = 0; Index < 5; Index++) {
    Status = Flash->SpiIo->Transaction(
                    Flash->SpiIo,                // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_THEN_READ, // TransactionType
                    FALSE,                       // DebugTransaction
                    ReadFrequency,               // Maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    sizeof(Command),             // WriteBytes
                    &Command,                    // WriteBuffer
                    3,                           // ReadBytes
                    &DeviceId[0]                 // ReadBuffer
                    );
    if ((EFI_ERROR(Status)) && (ReadFrequency == MHz(1))) {
      //
      // Assume that the SPI controller does not like the clock frequency.
      // Choose a clock frequency which is supported by the SPI controller.
      //
      ReadFrequency = Flash->FlashConfig->ReadFrequency;
      continue;
    }
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - Failed to read flash manufacture and device ID!\n"));
      return Status;
    }
    DEBUG ((EFI_D_INFO, "Received: %02x %02x %02x\n", DeviceId[0], DeviceId[1],
            DeviceId[2]));
    //
    // Verify that the data is not all ones or all zeros
    //
    if (((DeviceId[0] == 0xff) && (DeviceId[1] == 0xff)
          && (DeviceId[2] == 0xff))
      || ((DeviceId[0] == 0) && (DeviceId[1] == 0) && (DeviceId[2] == 0))) {
      continue;
    }

    //
    // Done when two ID values match
    //
    if ((DeviceId[0] == Buffer[0]) && (DeviceId[1] == Buffer[1])
      && (DeviceId[2] == Buffer[2])) {
      break;
    }

    //
    // Provide the latest ID value
    //
    Buffer[0] = DeviceId[0];
    Buffer[1] = DeviceId[1];
    Buffer[2] = DeviceId[2];
  }

  //
  // Verify that the data is not all ones or all zeros
  //
  if (((DeviceId[0] == 0xff) && (DeviceId[1] == 0xff) && (DeviceId[2] == 0xff))
      || ((DeviceId[0] == 0) && (DeviceId[1] == 0) && (DeviceId[2] == 0))) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - All %s received for manufacture and device ID!\n",
            DeviceId[0] ? L"ones" : L"zeros" ));
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

/**
  Low frequency read data from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine reads data from the SPI part in the buffer provided.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address in the flash to start reading
  @param[in]  LengthInBytes     Read length in bytes
  @param[in]  Buffer            Address of a buffer to receive the data

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_INVALID_PARAMETER The Buffer is NULL.
  @retval EFI_INVALID_PARAMETER The FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER The LengthInBytes > This->FlashSize
                                                    - FlashAddress
**/
EFI_STATUS
EFIAPI
FlashLfReadData (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 LengthInBytes,
  IN UINT8 *Buffer
  )
{
  FLASH *Flash;
  UINT32 ReadBytes;
  UINT8 ReadCommand[4];
  UINT32 ReadFrequency;
  CONST EFI_SPI_IO_PROTOCOL *SpiIo;
  EFI_STATUS Status;

  //
  // Validate the inputs
  //
  if (Buffer == NULL) {
    DEBUG((EFI_D_ERROR, "ERROR - Buffer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }
  if (FlashAddress >= This->FlashSize) {
    DEBUG((EFI_D_ERROR, "ERROR - FlashAddress (0x%08x) >= 0x%08x\n", FlashAddress, This->FlashSize));
    return EFI_INVALID_PARAMETER;
  }
  if (LengthInBytes > (This->FlashSize - FlashAddress)) {
    DEBUG((EFI_D_ERROR, "ERROR - LengthInBytes (0x%08x) > 0x%08x\n", LengthInBytes, This->FlashSize
           - FlashAddress));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the driver data structures
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  SpiIo = Flash->SpiIo;

  //
  // Determine if the transfer needs to be broken up
  //
  ReadFrequency = Flash->FlashConfig->ReadFrequency;
  if (SpiIo->MaximumTransferBytes >= LengthInBytes) {
    //
    // Build the read command
    //
    ReadCommand[0] = SPI_NOR_LOW_FREQUENCY_READ_DATA;
    ReadCommand[1] = (UINT8)(FlashAddress >> 16);
    ReadCommand[2] = (UINT8)(FlashAddress >> 8);
    ReadCommand[3] = (UINT8)FlashAddress;

    //
    // Read the data from the SPI NOR flash part
    //
    Status = SpiIo->Transaction(
                      SpiIo,                       // EFI_SPI_IO_PROTOCOL
                      SPI_TRANSACTION_WRITE_THEN_READ, // TransactionType
                      FALSE,                       // DebugTransaction
                      ReadFrequency,               // Maximum clock frequency
                      1,                           // Bus width in bits
                      8,                           // 8-bits per frame
                      sizeof(ReadCommand),         // WriteBytes
                      &ReadCommand[0],             // WriteBuffer
                      LengthInBytes,               // ReadBytes
                      Buffer                       // ReadBuffer
                      );
  } else {
    //
    // Remnove the opcode and address bytes from transfer size if necessary
    //
    ReadBytes = SpiIo->MaximumTransferBytes;
    if ((SpiIo->Attributes & SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE) != 0) {
      ReadBytes -= 1;
    }
    if ((SpiIo->Attributes & SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS) != 0) {
      ReadBytes -= 3;
    }
    do {
      //
      // Determine the number of bytes to transfer
      //
      if (ReadBytes > LengthInBytes) {
        ReadBytes = LengthInBytes;
      }

      //
      // Build the read command
      //
      ReadCommand[0] = SPI_NOR_LOW_FREQUENCY_READ_DATA;
      ReadCommand[1] = (UINT8)(FlashAddress >> 16);
      ReadCommand[2] = (UINT8)(FlashAddress >> 8);
      ReadCommand[3] = (UINT8)FlashAddress;

      //
      // Read the data from the SPI NOR flash part
      //
      Status = SpiIo->Transaction(
                      SpiIo,                       // EFI_SPI_IO_PROTOCOL
                      SPI_TRANSACTION_WRITE_THEN_READ, // TransactionType
                      FALSE,                       // DebugTransaction
                      ReadFrequency,               // Maximum clock frequency
                      1,                           // Bus width in bits
                      8,                           // 8-bits per frame
                      sizeof(ReadCommand),         // WriteBytes
                      &ReadCommand[0],             // WriteBuffer
                      ReadBytes,                   // ReadBytes
                      Buffer                       // ReadBuffer
                      );
      if (EFI_ERROR(Status)) {
        break;
      }

      //
      // Prepare for the next transfer
      //
      LengthInBytes -= ReadBytes;
      Buffer += ReadBytes;
      FlashAddress += ReadBytes;
    } while (LengthInBytes > 0);
  }
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Failed to read flash data, Status: %r!\n", Status));
  }
  return Status;
}

/**
  Read data from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine reads data from the SPI part in the buffer provided.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address in the flash to start reading
  @param[in]  LengthInBytes     Read length in bytes
  @param[out] Buffer            Address of a buffer to receive the data

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The data was read successfully.
  @retval EFI_INVALID_PARAMETER Buffer is NULL
  @retval EFI_INVALID_PARAMETER FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER LengthInBytes > This->FlashSize- FlashAddress
**/
EFI_STATUS
EFIAPI
FlashReadData (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 LengthInBytes,
  OUT UINT8 *Buffer
  )
{
  FLASH *Flash;
  UINT32 ReadBytes;
  UINT8 ReadCommand[5];
  CONST EFI_SPI_IO_PROTOCOL *SpiIo;
  EFI_STATUS Status;

  //
  // Determine if low frequency reads should be used
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  if (Flash->FlashConfig->LowFrequencyReadOnly ) {
    return FlashLfReadData (This, FlashAddress, LengthInBytes, Buffer);
  }

  //
  // Validate the inputs
  //
  if (Buffer == NULL) {
    DEBUG((EFI_D_ERROR, "ERROR - Buffer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }
  if (FlashAddress >= This->FlashSize) {
    DEBUG((EFI_D_ERROR, "ERROR - FlashAddress (0x%08x) >= 0x%08x\n", FlashAddress, This->FlashSize));
    return EFI_INVALID_PARAMETER;
  }
  if (LengthInBytes > (This->FlashSize - FlashAddress)) {
    DEBUG((EFI_D_ERROR, "ERROR - LengthInBytes (0x%08x) > 0x%08x\n", LengthInBytes, This->FlashSize
           - FlashAddress));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine if the transfer needs to be broken up
  //
  SpiIo = Flash->SpiIo;
  if (SpiIo->MaximumTransferBytes >= LengthInBytes) {
    //
    // Build the read command
    //
    ReadCommand[0] = SPI_NOR_READ_DATA;
    ReadCommand[1] = (UINT8)(FlashAddress >> 16);
    ReadCommand[2] = (UINT8)(FlashAddress >> 8);
    ReadCommand[3] = (UINT8)FlashAddress;
    ReadCommand[4] = 0;

    //
    // Read the data from the SPI NOR flash part
    //
    Status = Flash->SpiIo->Transaction(
                    Flash->SpiIo,                // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_THEN_READ, // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    sizeof(ReadCommand),         // WriteBytes
                    &ReadCommand[0],             // WriteBuffer
                    LengthInBytes,               // ReadBytes
                    Buffer                       // ReadBuffer
                    );
  } else {
    //
    // Remove the opcode and address bytes from transfer size if necessary
    //
    ReadBytes = SpiIo->MaximumTransferBytes;
    if ((SpiIo->Attributes & SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE) != 0) {
      ReadBytes -= 1;
    }
    if ((SpiIo->Attributes & SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS) != 0) {
      ReadBytes -= 3;
    }
    do {
      //
      // Determine the number of bytes to transfer
      //
      if (ReadBytes > LengthInBytes) {
        ReadBytes = LengthInBytes;
      }

      //
      // Build the read command
      //
      ReadCommand[0] = SPI_NOR_READ_DATA;
      ReadCommand[1] = (UINT8)(FlashAddress >> 16);
      ReadCommand[2] = (UINT8)(FlashAddress >> 8);
      ReadCommand[3] = (UINT8)FlashAddress;
      ReadCommand[4] = 0;

      //
      // Read the data from the SPI NOR flash part
      //
      Status = Flash->SpiIo->Transaction(
                    Flash->SpiIo,                // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_THEN_READ, // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    sizeof(ReadCommand),         // WriteBytes
                    &ReadCommand[0],             // WriteBuffer
                    ReadBytes,                   // ReadBytes
                    Buffer                       // ReadBuffer
                    );
      if (EFI_ERROR(Status)) {
        break;
      }

      //
      // Prepare for the next transfer
      //
      LengthInBytes -= ReadBytes;
      Buffer += ReadBytes;
      FlashAddress += ReadBytes;
    } while (LengthInBytes > 0);
  }
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Failed to read flash data, Status: %r!\n", Status));
  }
  return Status;
}

/**
  Read the flash status register.

  This routine must be called at or below TPL_NOTIFY.

  This routine reads the flash part status register.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  LengthInBytes     Number of status bytes to read.
  @param[out] FlashStatus       Pointer to a buffer to receive the flash status.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The status register was read successfully.

**/
EFI_STATUS
EFIAPI
FlashReadStatus (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 LengthInBytes,
  OUT UINT8 *FlashStatus
  )
{
  UINT8 Command;
  FLASH *Flash;
  EFI_STATUS Status;

  //
  // Get the driver data structures
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);

  //
  // Read the flash status from the SPI NOR flash part
  //
  Command = SPI_NOR_READ_STATUS;
  Status = Flash->SpiIo->Transaction(
                    Flash->SpiIo,                // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_THEN_READ, // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    sizeof(Command),             // WriteBytes
                    &Command,                    // WriteBuffer
                    LengthInBytes,               // ReadBytes
                    FlashStatus                  // ReadBuffer
                    );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Failed to read flash status!\n"));
    return Status;
  }
  return EFI_SUCCESS;
}

/**
  Write enable the SPI flash part.

  This routine must be called at or below TPL_NOTIFY.

  This routine sends the write enable to the SPI flash part.

  @param[in]  SpiIo             Pointer to an EFI_SPI_IO_PROTOCOL data structure

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.

**/
EFI_STATUS
EFIAPI
FlashWriteEnable (
  CONST EFI_SPI_IO_PROTOCOL *SpiIo
  )
{
  UINT8 Command;
  EFI_STATUS Status;

  Command = SPI_NOR_ENABLE_WRITE_OR_ERASE;
  Status = SpiIo->Transaction(
                    SpiIo,                       // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_ONLY,  // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    sizeof(Command),             // WriteBytes
                    &Command,                    // WriteBuffer
                    0,                           // ReadBytes
                    NULL                         // ReadBuffer
                    );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR - Flash failed to enable SPI writes\n"));
  }
  return Status;
}

/**
  Write the flash status register.

  This routine must be called at or below TPL_NOTIFY.

  This routine writes the flash part status register.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  LengthInBytes     Number of status bytes to write.
  @param[in]  FlashStatus       Pointer to a buffer containing the new status.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The status write was successful.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the write buffer.

**/
EFI_STATUS
EFIAPI
FlashWriteStatus (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 LengthInBytes,
  IN UINT8 *FlashStatus
  )
{
  FLASH *Flash;
  EFI_STATUS Status;
  UINT8 *WriteBuffer;

  //
  // Get the driver data structures
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);

  //
  // Allocate the write buffer
  //
  WriteBuffer = AllocatePool (LengthInBytes + 1);
  if (WriteBuffer == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate write buffer!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Write the flash status to the SPI NOR flash part
  //
  WriteBuffer[0] = SPI_NOR_WRITE_STATUS;
  CopyMem (&WriteBuffer[1], FlashStatus, LengthInBytes);
  Status = FlashWriteEnable (Flash->SpiIo);
  if (!EFI_ERROR(Status)) {
    Status = Flash->SpiIo->Transaction(
                    Flash->SpiIo,                // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_ONLY,  // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    LengthInBytes + 1,           // WriteBytes
                    WriteBuffer,                 // WriteBuffer
                    0,                           // ReadBytes
                    NULL                         // ReadBuffer
                    );
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - Failed to write flash status!\n"));
    }
  }

  //
  // Done with the write buffer
  //
  FreePool (WriteBuffer);
  return Status;
}

/**
  Wait for the flash operation to complete.

  This routine must be called at or below TPL_NOTIFY.

  This routine polls the flash part until the operation is complete.

  @param[in]  Flash             Pointer to an FLASH data structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.

**/
EFI_STATUS
EFIAPI
FlashWaitOperationComplete (
  IN FLASH *Flash
  )
{
  UINT8 FlashStatus;
  EFI_STATUS Status;
  UINT64 Time;
  UINT64 Timeout;

  //
  // Write and erase operations should take much less than 1 second
  //
  Timeout = GetTimeInNanoSecond (GetPerformanceCounter ())
            + ( 1ULL * 1000 * 1000 * 1000);

  //
  // Wait for the SPI NOR flash part to complete the write or erase operation
  //
  do {
    Status = FlashReadStatus (&Flash->LegacySpiFlash.FlashProtocol,
                              sizeof(FlashStatus),
                              &FlashStatus);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    //
    // Check for timeout
    //
    Time = GetTimeInNanoSecond (GetPerformanceCounter ());
    if (Time >= Timeout) {
      return EFI_TIMEOUT;
    }
  } while ((FlashStatus & SPI_STATUS1_BUSY) != 0);
  return EFI_SUCCESS;
}

/**
  Write data to the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine writes data to the SPI part from the buffer provided.

  @param[in]  Flash             Pointer to a FLASH data structure.
  @param[in]  FlashAddress      Address in the flash to start writing
  @param[in]  LengthInBytes     Write length in bytes
  @param[in]  Buffer            Address of a buffer containing the data
  @param[in]  WriteBuffer       Address of the temporary buffer

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.

**/
EFI_STATUS
EFIAPI
FlashWrite (
  IN FLASH *Flash,
  IN UINT32 FlashAddress,
  IN UINT32 LengthInBytes,
  IN UINT8 *Buffer,
  IN UINT8 *WriteBuffer
  )
{
  CONST EFI_SPI_IO_PROTOCOL *SpiIo;
  EFI_STATUS Status;
  UINT32 WriteBytes;

  //
  // Determine if the transfer needs to be broken up
  //
  SpiIo = Flash->SpiIo;
  if (SpiIo->MaximumTransferBytes >= LengthInBytes) {
    //
    // Enable writes to the SPI flash
    //
    Status = FlashWriteEnable (SpiIo);
    if (!EFI_ERROR(Status)) {
      //
      // Prepare the write buffer
      //
      WriteBuffer[0] = SPI_NOR_PAGE_PROGRAM;
      WriteBuffer[1] = (UINT8)(FlashAddress >> 16);
      WriteBuffer[2] = (UINT8)(FlashAddress >> 8);
      WriteBuffer[3] = (UINT8)FlashAddress;
      CopyMem (&WriteBuffer[4], Buffer, LengthInBytes);

      //
      // Write the data from the SPI NOR flash part
      //
      Status = SpiIo->Transaction(
                    SpiIo,                       // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_ONLY,  // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    4 + LengthInBytes,           // WriteBytes
                    &WriteBuffer[0],             // WriteBuffer
                    0,                           // ReadBytes
                    NULL                         // ReadBuffer
                    );
      if (!EFI_ERROR(Status)) {
        Status = FlashWaitOperationComplete (Flash);
      }
    }
  } else {
    //
    // Remove the opcode and address bytes from transfer size if necessary
    //
    WriteBytes = SpiIo->MaximumTransferBytes;
    if ((SpiIo->Attributes & SPI_IO_TRANSFER_SIZE_INCLUDES_OPCODE) != 0) {
      WriteBytes -= 1;
    }
    if ((SpiIo->Attributes & SPI_IO_TRANSFER_SIZE_INCLUDES_ADDRESS) != 0) {
      WriteBytes -= 3;
    }
    do {
      //
      // Enable writes to the SPI flash
      //
      Status = FlashWriteEnable (SpiIo);
      if (EFI_ERROR(Status)) {
        break;
      }

      //
      // Determine the number of bytes to transfer
      //
      if (WriteBytes > LengthInBytes) {
        WriteBytes = LengthInBytes;
      }

      //
      // Prepare the write buffer
      //
      WriteBuffer[0] = SPI_NOR_PAGE_PROGRAM;
      WriteBuffer[1] = (UINT8)(FlashAddress >> 16);
      WriteBuffer[2] = (UINT8)(FlashAddress >> 8);
      WriteBuffer[3] = (UINT8)FlashAddress;
      CopyMem (&WriteBuffer[4], Buffer, WriteBytes);

      //
      // Write the data from the SPI NOR flash part
      //
      Status = SpiIo->Transaction(
                    SpiIo,                       // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_ONLY,  // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    4 + WriteBytes,              // WriteBytes
                    &WriteBuffer[0],             // WriteBuffer
                    0,                           // ReadBytes
                    NULL                         // ReadBuffer
                    );
      if (EFI_ERROR(Status)) {
        break;
      }
      Status = FlashWaitOperationComplete (Flash);
      if (EFI_ERROR(Status)) {
        break;
      }

      //
      // Prepare for the next transfer
      //
      LengthInBytes -= WriteBytes;
      Buffer += WriteBytes;
      FlashAddress += WriteBytes;
    } while (LengthInBytes > 0);
  }
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Failed to write flash data, Status: %r!\n", Status));
  }
  return Status;
}

/**
  Write data to the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine breaks up the write operation as necessary to write the data to
  the SPI part.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address in the flash to start writing
  @param[in]  LengthInBytes     Write length in bytes
  @param[in]  Buffer            Address of a buffer containing the data

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The data was written successfully.
  @retval EFI_INVALID_PARAMETER The Buffer is NULL.
  @retval EFI_INVALID_PARAMETER The FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER The LengthInBytes > This->FlashSize
                                                    - FlashAddress
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory to copy buffer.

**/
EFI_STATUS
EFIAPI
FlashWriteData (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 LengthInBytes,
  IN UINT8 *Buffer
  )
{
  FLASH *Flash;
  EFI_STATUS Status;
  UINT8 *WriteBuffer;
  UINT32 WriteBytes;
  UINT32 WritePageBytes;

  //
  // Validate the inputs
  //
  if (Buffer == NULL) {
    DEBUG((EFI_D_ERROR, "ERROR - Buffer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }
  if (FlashAddress >= This->FlashSize) {
    DEBUG((EFI_D_ERROR, "ERROR - FlashAddress (0x%08x) >= 0x%08x\n", FlashAddress, This->FlashSize));
    return EFI_INVALID_PARAMETER;
  }
  if (LengthInBytes > (This->FlashSize - FlashAddress)) {
    DEBUG((EFI_D_ERROR, "ERROR - LengthInBytes (0x%08x) > 0x%08x\n", LengthInBytes, This->FlashSize
           - FlashAddress));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the driver data structures
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);

  //
  // Allocate the write buffer
  //
  WritePageBytes = Flash->FlashConfig->WritePageBytes;
  WriteBuffer = AllocatePool (1 + 3 + WritePageBytes);
  if (WriteBuffer == NULL) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - SpiFlashDxe write buffer allocation failed!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // If the data is not flash page aligned, write the first portion of the data
  //
  Status = EFI_SUCCESS;
  WriteBytes = FlashAddress & (WritePageBytes - 1);
  if (WriteBytes != 0) {
    //
    // Determine the number of bytes to write
    //
    WriteBytes = WritePageBytes - WriteBytes;
    if (WriteBytes > LengthInBytes) {
      WriteBytes = LengthInBytes;
    }

    //
    // Write the portion in the first page
    //
    Status = FlashWrite (Flash, FlashAddress, WriteBytes, Buffer, WriteBuffer);

    //
    // Prepare for the next write operation
    //
    FlashAddress += WriteBytes;
    Buffer += WriteBytes;
    LengthInBytes -= WriteBytes;
  }
  if (!EFI_ERROR(Status)) {
    //
    // Write pages of data to the flash
    //
    WriteBytes = WritePageBytes;
    while (LengthInBytes > 0) {
      //
      // Determine the number of bytes to write
      //
      if (WriteBytes > LengthInBytes) {
        WriteBytes = LengthInBytes;
      }

      //
      // Write the data to the flash
      //
      Status = FlashWrite(Flash, FlashAddress, WriteBytes, Buffer, WriteBuffer);
      if (EFI_ERROR(Status)) {
        break;
      }

      //
      // Prepare for the next write operation
      //
      FlashAddress += WriteBytes;
      Buffer += WriteBytes;
      LengthInBytes -= WriteBytes;
    }
  }

  //
  // Done with the temporary write buffer
  //
  FreePool (WriteBuffer);
  return Status;
}

/**
  Erases one or blocks in the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  Flash             Pointer to a FLASH data structure.
  @param[in]  FlashAddress      Address in the flash to start writing
  @param[in]  EraseOpcode       Opcode to perform the erase operation
  @param[in]  BlockBytes        Number of bytes in the erase block
  @param[in]  BlockCount        Number of blocks to erase

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_INVALID_PARAMETER The FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER The BlockCount * BlockBytes > This->FlashSize
                                                            - FlashAddress
**/
EFI_STATUS
EFIAPI
FlashEraseBlocks (
  IN FLASH *Flash,
  IN UINT32 FlashAddress,
  IN UINT8 EraseOpcode,
  IN UINT32 BlockBytes,
  IN UINT32 BlockCount
  )
{
  UINT8 Command [4];
  UINT32 FlashSize;
  CONST EFI_SPI_IO_PROTOCOL *SpiIo;
  EFI_STATUS Status;

  //
  // Validate the inputs
  //
  FlashSize = Flash->FlashConfig->FlashSize;
  if (FlashAddress >= FlashSize) {
    DEBUG((EFI_D_ERROR, "ERROR - FlashAddress (0x%08x) >= 0x%08x\n", FlashAddress, FlashSize));
    return EFI_INVALID_PARAMETER;
  }
  if ((BlockCount * BlockBytes) > (FlashSize - FlashAddress))
  {
    DEBUG((EFI_D_ERROR, "ERROR - BlockCount * %d = 0x%08x > 0x%08x\n",
          BlockBytes, BlockCount * BlockBytes, FlashSize - FlashAddress));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Erase the blocks
  //
  Status = EFI_SUCCESS;
  Command [0] = EraseOpcode;
  FlashAddress &= ~(BlockBytes - 1);
  SpiIo = Flash->SpiIo;
  while (BlockCount-- > 0) {
    //
    // Build the erase command
    //
    Command [1] = (UINT8)(FlashAddress >> 16);
    Command [2] = (UINT8)(FlashAddress >> 8);
    Command [3] = (UINT8)FlashAddress;

    //
    // Erase the next block
    //
    Status = FlashWriteEnable (SpiIo);
    if (!EFI_ERROR(Status)) {
      Status = SpiIo->Transaction(
                    SpiIo,                       // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_ONLY,  // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    8,                           // 8-bits per frame
                    sizeof(Command),             // WriteBytes
                    &Command[0],                 // WriteBuffer
                    0,                           // ReadBytes
                    NULL                         // ReadBuffer
                    );
    }
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "ERROR - Flash failed to erase %d bytes at 0x%08x\n",
              BlockBytes, FlashAddress));
      break;
    }
    Status = FlashWaitOperationComplete (Flash);
    if (EFI_ERROR(Status)) {
      break;
    }

    //
    // Set the address of the next flash block
    //
    FlashAddress += BlockBytes;
  }
  return Status;
}

/**
  Erases one or more 4KiB regions in the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address in the flash to start writing
  @param[in]  BlockCount        Number of 4 KiB regions to erase

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_INVALID_PARAMETER The FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER The BlockCount * 4 KiB > This->FlashSize
                                                         - FlashAddress
**/
EFI_STATUS
EFIAPI
FlashErase4KiB (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 BlockCount
  )
{
  FLASH *Flash;

  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  return FlashEraseBlocks (Flash, FlashAddress, SPI_NOR_ERASE_4KB, 4096,
                           BlockCount);
}

/**
  Determine the erase block opcode from the erase block size.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  Flash             Pointer to a FLASH data structure.

  @return  The erase block opcode value

**/
UINT8
EFIAPI
FlashEraseBlockOpcode (
  IN FLASH *Flash
  )
{
  UINT32 EraseBlockBytes;

  //
  // Determine the erase block opcode
  //
  EraseBlockBytes = Flash->FlashConfig->EraseBlockBytes;
  return (EraseBlockBytes == 65536) ? SPI_NOR_ERASE_64KB : SPI_NOR_ERASE_32KB;
}

/**
  Erases one or blocks in the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address in the flash to start writing
  @param[in]  BlockCount        Number of blocks to erase

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_INVALID_PARAMETER The FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER The BlockCount * 4 KiB > This->FlashSize
                                                         - FlashAddress
**/
EFI_STATUS
EFIAPI
FlashEraseBlock (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 BlockCount
  )
{
  UINT32 EraseBlockBytes;
  FLASH *Flash;

  //
  // Get the driver data structures
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  EraseBlockBytes = Flash->FlashConfig->EraseBlockBytes;
  return FlashEraseBlocks (Flash,
                           FlashAddress,
                           FlashEraseBlockOpcode (Flash),
                           EraseBlockBytes,
                           BlockCount);
}

/**
  Efficiently erases one or more 4KiB regions in the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine uses a combination of 4 KiB and larger blocks to erase the
  specified area.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address within a 4 KiB block to start erasing
  @param[in]  BlockCount        Number of 4 KiB blocks to erase

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The erase was completed successfully.
  @retval EFI_INVALID_PARAMETER FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER BlockCount * 4 KiB > This->FlashSize
                                                     - FlashAddress
**/
EFI_STATUS
EFIAPI
FlashErase (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 BlockCount
  )
{
  UINT32 EraseBlocks;
  UINT32 EraseBlockBytes;
  FLASH *Flash;
  UINT32 FlashSize;
  EFI_STATUS Status;

  //
  // Align the flash address to 4 KiB
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  FlashAddress &= ~(BIT12 - 1);

  //
  // Validate the inputs
  //
  FlashSize = Flash->FlashConfig->FlashSize;
  if (FlashAddress >= FlashSize) {
    DEBUG((EFI_D_ERROR, "ERROR - FlashAddress (0x%08x) >= 0x%08x\n", FlashAddress, FlashSize));
    return EFI_INVALID_PARAMETER;
  }
  if ((BlockCount * BIT12) > (FlashSize - FlashAddress))
  {
    DEBUG((EFI_D_ERROR, "ERROR - BlockCount * %d = 0x%08x > 0x%08x\n",
          BIT12, BlockCount * BIT12, FlashSize - FlashAddress));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Erase 4 KiB blocks to start
  //
  Status = EFI_SUCCESS;
  EraseBlockBytes = Flash->FlashConfig->EraseBlockBytes;
  EraseBlocks = ((0 - FlashAddress) & ~(EraseBlockBytes - 1)) >> 12;
  if (EraseBlocks > BlockCount) {
    EraseBlocks = BlockCount;
  }
  if (EraseBlocks > 0) {
    BlockCount -= EraseBlocks;
    Status = FlashErase4KiB (This, FlashAddress, EraseBlocks);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //
  // Erase large blocks next
  //
  EraseBlocks = BlockCount / (EraseBlockBytes >> 12);
  if (EraseBlocks > 0) {
    BlockCount -= EraseBlocks;
    Status = FlashEraseBlock (This, FlashAddress, EraseBlocks);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //
  // Erase any blocks which are left
  //
  EraseBlocks = BlockCount;
  if (EraseBlocks > 0) {
    Status = FlashErase4KiB (This, FlashAddress, EraseBlocks);
  }
  return Status;
}

/**
  Display data from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine reads data from the SPI part in the buffer provided.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  LowFrequency      Use low frequency read routine
  @param[in]  FlashAddress      Address in the flash to start reading
  @param[in]  LengthInBytes     Read length in bytes
  @param[in]  Buffer            Address of a buffer to receive the data

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_INVALID_PARAMETER The Buffer is NULL.
  @retval EFI_INVALID_PARAMETER The FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER The LengthInBytes > This->FlashSize
                                                    - FlashAddress
**/
EFI_STATUS
EFIAPI
FlashDump (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN BOOLEAN LowFrequency,
  IN UINT32 FlashAddress,
  IN UINT32 LengthInBytes,
  IN UINT8 *Buffer
  )
{
  EFI_STATUS Status;

  //
  // Read the data from the flash
  //
  if (LowFrequency) {
    Status = This->LfReadData (This,
                               FlashAddress,
                               LengthInBytes,
                               Buffer);
  } else {
    Status = This->ReadData (This,
                             FlashAddress,
                             LengthInBytes,
                             Buffer);
  }
  if (!EFI_ERROR(Status)) {
    AsciiDump ((UINT8 *)(UINTN)FlashAddress, &Buffer[0], LengthInBytes);
  }
  return Status;
}

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
  @retval EFI_DEVICE_ERROR      The SPI controller is locked
  @retval EFI_INVALID_PARAMETER BiosBaseAddress > This->MaximumOffset
  @retval EFI_UNSUPPORTED       The BIOS base address was already set
  @retval EFI_UNSUPPORTED       Not a legacy SPI host controller

**/
EFI_STATUS
EFIAPI
FlashBiosBaseAddress (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This,
  IN UINT32 BiosBaseAddress
  )
{
  FLASH *Flash;
  CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol;

  //
  // Determine if a legacy flash (SPI host) controller is available
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  LegacySpiProtocol = Flash->SpiIo->LegacySpiProtocol;
  if (LegacySpiProtocol == NULL ) {
    DEBUG ((EFI_D_ERROR, "ERROR - Not connected to a legacy SPI controller\n"));
    return EFI_UNSUPPORTED;
  }
  return LegacySpiProtocol->BiosBaseAddress (LegacySpiProtocol,
                                             BiosBaseAddress);
}

/**
  Clear the SPI protect range registers.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS uses this routine to set an initial condition on the SPI protect
  range registers.

  @param[in]  This              Pointer to an EFI_LEGACY_SPI_FLASH_PROTOCOL data
                                structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The register was successfully updated
  @retval EFI_ACCESS_ERROR      The SPI controller is locked
  @retval EFI_UNSUPPORTED       Not a legacy SPI host controller

**/
EFI_STATUS
EFIAPI
FlashClearSpiProtect (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This
  )
{
  FLASH *Flash;
  CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol;

  //
  // Determine if a legacy flash (SPI host) controller is available
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  LegacySpiProtocol = Flash->SpiIo->LegacySpiProtocol;
  if (LegacySpiProtocol == NULL ) {
    DEBUG ((EFI_D_ERROR, "ERROR - Not connected to a legacy SPI controller\n"));
    return EFI_UNSUPPORTED;
  }
  return LegacySpiProtocol->ClearSpiProtect (LegacySpiProtocol);
}

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
BOOLEAN
EFIAPI
FlashIsRangeProtected (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This,
  IN UINT32 BiosAddress,
  IN UINT32 BlocksToProtect
  )
{
  FLASH *Flash;
  CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol;

  //
  // Determine if a legacy flash (SPI host) controller is available
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  LegacySpiProtocol = Flash->SpiIo->LegacySpiProtocol;
  if (LegacySpiProtocol == NULL ) {
    DEBUG ((EFI_D_ERROR, "ERROR - Not connected to a legacy SPI controller\n"));
    return FALSE;
  }
  return LegacySpiProtocol->IsRangeProtected (LegacySpiProtocol,
                                              BiosAddress,
                                              BlocksToProtect);
}

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
  @retval EFI_DEVICE_ERROR      The SPI controller is locked
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
EFI_STATUS
EFIAPI
FlashProtectNextRange (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This,
  IN UINT32 BiosAddress,
  IN UINT32 BlocksToProtect
  )
{
  FLASH *Flash;
  CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol;

  //
  // Determine if a legacy flash (SPI host) controller is available
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  LegacySpiProtocol = Flash->SpiIo->LegacySpiProtocol;
  if (LegacySpiProtocol == NULL ) {
    DEBUG ((EFI_D_ERROR, "ERROR - Not connected to a legacy SPI controller\n"));
    return EFI_UNSUPPORTED;
  }
  return LegacySpiProtocol->ProtectNextRange (LegacySpiProtocol,
                                              BiosAddress,
                                              BlocksToProtect);
}

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
EFI_STATUS
EFIAPI
FlashLockController (
  IN CONST EFI_LEGACY_SPI_FLASH_PROTOCOL *This
  )
{
  FLASH *Flash;
  CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol;

  //
  // Determine if a legacy flash (SPI host) controller is available
  //
  Flash = FLASH_CONTEXT_FROM_PROTOCOL (This);
  LegacySpiProtocol = Flash->SpiIo->LegacySpiProtocol;
  if (LegacySpiProtocol == NULL ) {
    DEBUG ((EFI_D_ERROR, "ERROR - Not connected to a legacy SPI controller\n"));
    return EFI_UNSUPPORTED;
  }
  return LegacySpiProtocol->LockController (LegacySpiProtocol);
}

/**
  Shuts down the driver.

  This routine must be called at or below TPL_NOTIFY.

  This routine deallocates the resources supporting the SPI bus operation.

  @param[in]  Flash             Pointer to a Flash data structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
VOID
EFIAPI
FlashShutdown (
  IN FLASH *Flash
  )
{
  //
  // Determine if the job is already done
  //
  if (Flash != NULL) {
    //
    // Release the SPI IO protocol
    //
    if (Flash->SpiIo != NULL) {
      SpiCloseProtocol (
             Flash->ControllerHandle,
             (VOID *)gFlashIoProtocolGuid,
             gImageHandle,
             NULL
             );
    }

    //
    // Free the data structure
    //
    FreePool (Flash);
  }
}

/**
  Set up the necessary pieces to start SPI bus layer this device.

  This routine must be called at or below TPL_NOTIFY.

  Initialize the context data structure to support the SPI bus layer.  Gain
  access to the SPI HC protocol.  Upon successful completion, install the SPI
  IO protocols for each of the child controllers.

  @param[in]  DriverBinding     A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                instance.
  @param[in]  ControllerHandle  The handle of the controller to start. This
                                handle must support a protocol interface that
                                supplies an I/O abstraction to the driver.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI host controller was started successfully
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
**/
EFI_STATUS
EFIAPI
FlashStartup (
  IN EFI_HANDLE ControllerHandle,
  IN CONST EFI_SPI_IO_PROTOCOL *SpiIo
  )
{
  FLASH *Flash;
  CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA *FlashConfig;
  EFI_SPI_NOR_FLASH_PROTOCOL *FlashProtocol;
  EFI_LEGACY_SPI_FLASH_PROTOCOL *LegacySpiFlash;
  CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *LegacySpiProtocol;
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral;
  EFI_STATUS Status;

  //
  // Allocate the controller data structure
  //
  Flash = AllocateZeroPool (sizeof (FLASH));
  if (Flash == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate SPI_BUS!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save the inputs for the shutdown operation
  //
  Flash->Signature = FLASH_SIGNATURE;
  Flash->ControllerHandle = ControllerHandle;
  Flash->SpiIo = SpiIo;

  //
  // Verify that the flash configuration structure is available
  //
  SpiIo = Flash->SpiIo;
  SpiPeripheral = SpiIo->SpiPeripheral;
  FlashConfig = SpiPeripheral->ConfigurationData;
  if (FlashConfig == NULL) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Flash configuration data structure is missing!\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Failure;
  }
  Flash->FlashConfig = FlashConfig;

  //
  // Initialize the SPI flash protocol
  //
  LegacySpiFlash = &Flash->LegacySpiFlash;
  FlashProtocol = &LegacySpiFlash->FlashProtocol;
  FlashProtocol->SpiPeripheral = SpiPeripheral;

  FlashProtocol->GetFlashId = FlashGetFlashId;
  FlashProtocol->ReadData = FlashReadData;
  FlashProtocol->LfReadData = FlashLfReadData;
  FlashProtocol->ReadStatus = FlashReadStatus;
  FlashProtocol->WriteStatus = FlashWriteStatus;
  FlashProtocol->WriteData = FlashWriteData;
  FlashProtocol->Erase = FlashErase;

  //
  // Initialize the legacy SPI flash controller interface
  //
  LegacySpiFlash->LockController = FlashLockController;
  LegacySpiFlash->ClearSpiProtect = FlashClearSpiProtect;
  LegacySpiFlash->IsRangeProtected = FlashIsRangeProtected;
  LegacySpiFlash->ProtectNextRange = FlashProtectNextRange;
  LegacySpiFlash->BiosBaseAddress = FlashBiosBaseAddress;

  //
  // Identify the SPI NOR flash part
  //
  Status = FlashProtocol->GetFlashId (FlashProtocol,
                                      &FlashProtocol->DeviceId[0]);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Flash failed to determine SPI NOR flash size!\n"));
    goto Failure;
  }

  //
  // Locate the corresponding flash part if possible
  //
  if (((FlashConfig->DeviceId[0] != FlashProtocol->DeviceId[0])
    || (FlashConfig->DeviceId[1] != FlashProtocol->DeviceId[1])
    || (FlashConfig->DeviceId[2] != FlashProtocol->DeviceId[2]))
    && (FlashConfig->SpiFlashList != NULL)) {
    SpiPeripheral = FlashConfig->SpiFlashList;
    while (SpiPeripheral != NULL) {
      //
      // Verify that the flash configuration data is present
      //
      FlashConfig = SpiPeripheral->ConfigurationData;
DEBUG ((EFI_D_ERROR, "SPI Flash: %s: %s: %s %s (%02x %02x %02x)\n", FlashProtocol->SpiPeripheral->FriendlyName, SpiPeripheral->FriendlyName, SpiPeripheral->SpiPart->Vendor, SpiPeripheral->SpiPart->PartNumber, FlashConfig->DeviceId[0], FlashConfig->DeviceId[1], FlashConfig->DeviceId[2]));
      if (FlashConfig == NULL) {
        DEBUG ((EFI_D_ERROR, "SPI Flash: %s: %s\n",
                FlashProtocol->SpiPeripheral->FriendlyName,
                SpiPeripheral->FriendlyName));
        DEBUG ((EFI_D_ERROR,
                "ERROR - Flash configuration data structure is missing!\n"));
        Status = EFI_INVALID_PARAMETER;
        goto Failure;
      }

      //
      // Determine if the device ID matches
      //
      if ((FlashConfig->DeviceId[0] == FlashProtocol->DeviceId[0])
        && (FlashConfig->DeviceId[1] == FlashProtocol->DeviceId[1])
        && (FlashConfig->DeviceId[2] == FlashProtocol->DeviceId[2])) {

        //
        //  The device ID matches, use this SpiPeripheral instead
        //
        FlashProtocol->SpiPeripheral = SpiPeripheral;
        Flash->FlashConfig = FlashConfig;
        Status = Flash->SpiIo->UpdateSpiPeripheral (SpiIo, SpiPeripheral);
        if (EFI_ERROR(Status)) {
          DEBUG ((EFI_D_ERROR, "SPI Flash: %s: %s\n",
                  FlashProtocol->SpiPeripheral->FriendlyName,
                  SpiPeripheral->FriendlyName));
          Status = EFI_INVALID_PARAMETER;
          goto Failure;
        }
        break;
      }

      //
      // Check the next flash device in the list
      //
      SpiPeripheral = SpiPeripheral->NextSpiPeripheral;
    }

    //
    // Display the flash part choice
    //
    if (SpiPeripheral == NULL) {
      SpiPeripheral = FlashProtocol->SpiPeripheral;
    }
    DEBUG ((EFI_D_INFO, "Found %s: %s %s\n", SpiPeripheral->FriendlyName,
           SpiPeripheral->SpiPart->Vendor, SpiPeripheral->SpiPart->PartNumber));
  }

  //
  // Verify the erase size in the flash configuration structure
  //
  FlashConfig = Flash->FlashConfig;
  if ((FlashConfig->EraseBlockBytes != BIT15)
    && (FlashConfig->EraseBlockBytes != BIT16)) {
    DEBUG ((EFI_D_ERROR,
           "ERROR - Flash erase block size in bytes is not %d or %d!\n",
           BIT15, BIT16));
    Status = EFI_INVALID_PARAMETER;
    goto Failure;
  }

  //
  // Update the flash configuration
  //
  FlashProtocol->FlashSize = FlashConfig->FlashSize;
  FlashProtocol->EraseBlockBytes = FlashConfig->EraseBlockBytes;
  
  //
  // Update the legacy flash controller's opcode menu table with the proper
  // erase block opcode.
  //
  LegacySpiProtocol = Flash->SpiIo->LegacySpiProtocol;
  if (LegacySpiProtocol != NULL) {
    Status = LegacySpiProtocol->EraseBlockOpcode (LegacySpiProtocol,
                                                  FlashEraseBlockOpcode(Flash));
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "ERROR - Failed to set erase block size!\n"));
      goto Failure;
    }

    //
    // Update the legacy flash controller's prefix table with the proper write
    // status prefix opcode.
    //
    Status = LegacySpiProtocol->WriteStatusPrefix(LegacySpiProtocol,
                                          FlashConfig->WriteStatusPrefixOpcode);
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "ERROR - Failed to set erase block size!\n"));
      goto Failure;
    }
  }

  //
  // Determine the manufacture
  //
  DEBUG_CODE_BEGIN();
    FlashDisplayManufactureName (FlashProtocol->DeviceId[0]);
  DEBUG_CODE_END();

  //
  // Display flash part size
  //
  if (FlashProtocol->FlashSize < BIT20) {
    DEBUG ((EFI_D_INFO, "SPI flash size: %d KiBytes\n",
            FlashProtocol->FlashSize / 1024));
  } else {
    DEBUG ((EFI_D_INFO, "SPI flash size: %d MiBytes\n",
            FlashProtocol->FlashSize / BIT20));
  }

  //
  // Display a little flash data
  //
  DEBUG_CODE_BEGIN();
    UINT32 FlashAddress;
    UINT8 ReadData[64];

    if (!FlashConfig->LowFrequencyReadOnly) {
      //
      // Display a little data from the beginning of the flash
      //
      FlashAddress = 0;
      FlashDump (FlashProtocol, TRUE, FlashAddress, sizeof(ReadData),
                 &ReadData[0]);

      //
      // Display a little data from the end of the flash
      //
      FlashAddress = FlashProtocol->FlashSize - sizeof(ReadData);
      FlashDump (FlashProtocol, TRUE, FlashAddress, sizeof(ReadData),
                 &ReadData[0]);
    }

    //
    // Display a little data from the beginning of the flash
    //
    FlashAddress = 0;
    FlashDump (FlashProtocol, FALSE, FlashAddress, sizeof(ReadData),
               &ReadData[0]);

    //
    // Display a little data from the end of the flash
    //
    FlashAddress = FlashProtocol->FlashSize - sizeof(ReadData);
    FlashDump (FlashProtocol, FALSE, FlashAddress, sizeof(ReadData),
               &ReadData[0]);
  DEBUG_CODE_END();

  //
  // Install the EFI_SPI_NOR_FLASH_PROTOCOL
  //
  if (LegacySpiProtocol != NULL) {
    Status = SpiInstallProtocol (
                     &ControllerHandle,
                     gFlashLegacyProtocolGuid,
                     &Flash->LegacySpiFlash
                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - Flash failed to install EFI_LEGACY_SPI_FLASH_PROTOCOL!\n"));
      goto Failure;
    }
  }
  Status = SpiInstallProtocol (
                     &ControllerHandle,
                     gFlashProtocolGuid,
                     &Flash->LegacySpiFlash.FlashProtocol
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Flash failed to install EFI_SPI_NOR_FLASH_PROTOCOL!\n"));
    goto Failure;
  }
  return EFI_SUCCESS;

Failure:
  //
  // Release the SPI host controller resources
  //
  FlashShutdown (Flash);
  return Status;
}
