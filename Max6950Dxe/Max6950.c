/** @file

  Maxim MAX6950 protocol for displaying data on seven-segment displays.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Max6950Dxe.h"

//
//                ---a---
//               |       |
//               f       b
//               |       |        7  6  5  4  3  2  1  0
//                ---g---        dp  a  b  c  d  e  f  g
//               |       |
//               e       c
//               |       |
//                ---d---    dp
//
CONST UINT8 CharacterTranslationTable[] = {
//  0     1     2     3     4     5     6     7
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x00
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x08
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x18
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,  // 0x28
  0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70,  // 0x30
  0x7f, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x38
  0x00, 0x77, 0x1f, 0x4e, 0x3f, 0x4f, 0x47, 0x00,  // 0x40
};

/**
  Display the contents of a string on the seven segment displays.

  This routine must be called at or below TPL_CALLBACK.

  This routine takes the input data and converts it from ASCII into something
  that can be displayed on the seven segment display.  Only certain characters
  of the ASCII set (0-127) are displayable, all others display as blanks.

  A period only consumes the a display if another character is not preceeding
  it.  When a character preceeds the period, both the character and the period
  are displayed on the same seven segment display.

  @param[in]  This              Pointer to a MAXIM_MAX6950_PROTOCOL structure.
  @param[in]  Data              ASCII character data to display.
  @param[in]  LengthInBytes     The length of the string in bytes.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The display was successful.
  @retval EFI_INVALID_PARAMETER Data was NULL
  @retval EFI_INVALID_PARAMETER Length too long for string being displayed
  @retval EFI_UNSUPPORTED       The board does not support seven-segment
                                displays
**/
EFI_STATUS
EFIAPI
Max6950DisplayString (
  IN CONST MAXIM_MAX6950_PROTOCOL *This,
  IN UINT8 *Data,
  IN UINTN LengthInBytes
  )
{
  UINTN Character;
  UINTN DisplayNumber;
  UINTN Index;
  MAX6950 *Max6950;
  UINTN MaxBytes;
  UINT16 RegisterAddress;
  EFI_STATUS Status;
  UINT16 WriteData;

  //
  // Get the driver data structure
  //
  Max6950 = MAX6950_CONTEXT_FROM_PROTOCOL (This);

  //
  // Verify that seven-segment displays are supported
  //
  if (Max6950->Max6950Protocol.DigitsInDisplay == 0) {
    DEBUG ((EFI_D_ERROR, "ERROR - No seven-segment displays are available!\n"));
    Status = EFI_UNSUPPORTED;
    goto Failure;
  }

  //
  // Verify the input parameters
  //
  if (Data == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Data is NULL!\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Failure;
  }

  //
  // Translate the input data into the data for the display
  //
  MaxBytes = Max6950->Max6950Protocol.DigitsInDisplay * 2;
  DisplayNumber = 0;
  for (Index = 0; Index < LengthInBytes; Index++) {
    //
    // Determine the regsiter address for this display
    //
    if (DisplayNumber >= Max6950->Max6950Protocol.DigitsInDisplay) {
      RegisterAddress = MAX6950_DIGIT0_P1;
    } else {
      RegisterAddress = MAX6950_DIGIT0_PX;
    }
    RegisterAddress += Max6950->DisplayOrder[DisplayNumber
          % Max6950->Max6950Protocol.DigitsInDisplay];
    DisplayNumber += 1;

    //
    // Determine which segments to light for the input character
    //
    Character = Data[Index];
    WriteData = (Character > sizeof(CharacterTranslationTable))
             ? 0 : CharacterTranslationTable[Character];

    //
    // Display the following period on the same display where possible
    //
    if ((Character != '.') && ((Index + 1) < LengthInBytes)
      && (Data[Index + 1] == '.')) {
      WriteData |= CharacterTranslationTable['.'];
      Index += 1;
    }

    //
    // Send this digit to the display
    //
    WriteData |= RegisterAddress << 8;
    Status = Max6950->SpiIo->Transaction(
                    Max6950->SpiIo,              // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_ONLY,  // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    16,                          // 16-bits per frame
                    sizeof(WriteData),           // WriteBytes
                    (UINT8 *)&WriteData,         // WriteBuffer
                    0,                           // ReadBytes
                    NULL                         // ReadBuffer
                    );
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - MAX6950 failed digit write, Status: %r\n", Status));
      goto Failure;
    }

    //
    // Account for this display
    //
    if ((DisplayNumber == MaxBytes) && ((Index + 1) < LengthInBytes)) {
      DEBUG ((EFI_D_ERROR, "ERROR - Input string too long!\n"));
      Status = EFI_INVALID_PARAMETER;
      goto Failure;
    }
  }

  //
  // Clear the rest of the display
  //
  Index = DisplayNumber % Max6950->Max6950Protocol.DigitsInDisplay;
  if (Index != 0) {
    for (; Index < Max6950->Max6950Protocol.DigitsInDisplay; Index++) {
      //
      // Determine the regsiter address for this display
      //
      if (DisplayNumber >= Max6950->Max6950Protocol.DigitsInDisplay) {
        RegisterAddress = MAX6950_DIGIT0_P1;
      } else {
        RegisterAddress = MAX6950_DIGIT0_PX;
      }
      RegisterAddress += Max6950->DisplayOrder[DisplayNumber
            % Max6950->Max6950Protocol.DigitsInDisplay];
      DisplayNumber += 1;

      //
      // Clear the digit on the display
      //
      WriteData = RegisterAddress << 8;
      Status = Max6950->SpiIo->Transaction(
                    Max6950->SpiIo,              // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_ONLY,  // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    16,                          // 16-bits per frame
                    sizeof(WriteData),           // WriteBytes
                    (UINT8 *)&WriteData,         // WriteBuffer
                    0,                           // ReadBytes
                    NULL                         // ReadBuffer
                    );
      if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - MAX6950 failed clear digit, Status: %r\n", Status));
        goto Failure;
      }
    }
  }

  //
  // If the string is too long for the display then the display must alternate
  // between the first and second halves of the string to be displayed.
  //
  WriteData = (MAX6950_CONFIG << 8) | MAX6950_CONFIG_NORMAL
           | ((DisplayNumber > Max6950->Max6950Protocol.DigitsInDisplay)
                  ? MAX6950_CONFIG_BLINK_ENABLE : 0);
  Status = Max6950->SpiIo->Transaction(
                    Max6950->SpiIo,              // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_WRITE_ONLY,  // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    16,                          // 16-bits per frame
                    sizeof(WriteData),           // WriteBytes
                    (UINT8 *)&WriteData,         // WriteBuffer
                    0,                           // ReadBytes
                    NULL                         // ReadBuffer
                    );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - MAX6950 failed blink control, Status: %r\n", Status));
  }

Failure:
  return Status;
}

/**
  Shuts down the driver.

  This routine must be called at or below TPL_NOTIFY.

  This routine deallocates the resources supporting the SPI bus operation.

  @param[in]  Max6950           Pointer to a MAX6950 data structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
STATIC
VOID
EFIAPI
Max6950ShutdownWorker (
  IN MAX6950 *Max6950
  )
{
  MAXIM_MAX6950_PROTOCOL *Max6950Protocol;
  EFI_STATUS Status;

  //
  // Determine if the job is already done
  //
  if (Max6950 != NULL) {
    //
    // Release the SPI IO protocol
    //
    if (Max6950->SpiIo != NULL) {
      gBS->CloseProtocol (
             Max6950->ControllerHandle,
             (VOID *)&Maxim_MAX6950_Driver,
             Max6950->DriverBinding->DriverBindingHandle,
             Max6950->ControllerHandle
             );
    }

    //
    // Determine if the MAXIM_MAX6950_PROTOCOL is present
    //
    Status = gBS->OpenProtocol (
                    Max6950->ControllerHandle,
                    &gMaximMax6950ProtocolGuid,
                    (VOID **) &Max6950Protocol,
                    Max6950->DriverBinding->DriverBindingHandle,
                    Max6950->ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (Status == EFI_SUCCESS) {
      //
      // Remove the MAXIM_MAX6950_PROTOCOL
      //
      Status = gBS->UninstallProtocolInterface (
                       Max6950->ControllerHandle,
                       &gMaximMax6950ProtocolGuid,
                       Max6950Protocol
                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - Max6950 failed to remove MAXIM_MAX6950_PROTOCOL!\n"));
        ASSERT_EFI_ERROR(Status);
      }
    }

    //
    // Free the data structure
    //
    FreePool (Max6950);
  }
}

/**
  Shuts down the SPI bus layer.

  This routine must be called at or below TPL_NOTIFY.

  This routine determines if the bus layer is not busy.  Once it is idle, the
  supporting resources are deallocated.

  @param[in]  Max6950Protocol   A pointer to the MAXIM_MAX6950_PROTOCOL instance

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
EFI_STATUS
EFIAPI
Max6950Shutdown (
  IN MAXIM_MAX6950_PROTOCOL *Max6950Protocol
  )
{
  EFI_STATUS Status;

DEBUG ((EFI_D_ERROR, "Max6950Shutdown entered\n"));
  Status = EFI_UNSUPPORTED;
DEBUG ((EFI_D_ERROR, "Max6950Shutdown exiting, Status: %r\n", Status));
  return Status;
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
Max6950Startup (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN EFI_HANDLE ControllerHandle
  )
{
  
  MAX6950 *Max6950;
  CONST MAX6950_CONFIGURATION_DATA *Max6950Config;
  EFI_STATUS Status;

  //
  // Allocate the controller data structure
  //
  Max6950 = AllocateZeroPool (sizeof (MAX6950));
  if (Max6950 == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate SPI_BUS!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save the inputs for the shutdown operation
  //
  Max6950->Signature = MAX6950_SIGNATURE;
  Max6950->DriverBinding = DriverBinding;
  Max6950->ControllerHandle = ControllerHandle;

  //
  // Get access to the SPI IO Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  (VOID *)&Maxim_MAX6950_Driver,
                  (VOID **)&Max6950->SpiIo,
                  DriverBinding->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR - Max6950 failed to open SPI IO protocol!\n"));
    goto Failure;
  }

  //
  // Initialize the MAX6950 protocol
  //
  Max6950Config = Max6950->SpiIo->SpiPeripheral->ConfigurationData;
  if (Max6950Config == NULL) {
    DEBUG ((EFI_D_ERROR,
         "MAX6950 Configuration data missing, please specify\n"
         ));
    Status = EFI_UNSUPPORTED;
    goto Failure;
  }
  if (Max6950Config->DisplayOrder == NULL) {
    DEBUG ((EFI_D_ERROR,
         "MAX6950 Display order array missing, please add display order data\n"
         ));
    Status = EFI_UNSUPPORTED;
    goto Failure;
  }
  Max6950->Max6950Protocol.DigitsInDisplay = Max6950Config->DisplayOrderSize;
  Max6950->Max6950Protocol.DisplayString = Max6950DisplayString;
  Max6950->DisplayOrder = Max6950Config->DisplayOrder;

  //
  // Install the MAXIM_MAX6950_PROTOCOL
  //
  Status = gBS->InstallProtocolInterface (
                   &ControllerHandle,
                   &gMaximMax6950ProtocolGuid,
                   EFI_NATIVE_INTERFACE,
                   &Max6950->Max6950Protocol
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Max6950 failed to install MAXIM_MAX6950_PROTOCOL!\n"));
    goto Failure;
  }
  return EFI_SUCCESS;

Failure:
  //
  // Release the SPI host controller resources
  //
  Max6950ShutdownWorker (Max6950);
  return Status;
}
