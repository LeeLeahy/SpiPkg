/** @file

  This module implements the Texas Instruments ADC108S102 protocol for the
  analog to digital converter.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Adc108s102Dxe.h"

/**
  Read the 10-bit analog to digital converter channel value.

  This routine must be called at or below TPL_CALLBACK.

  @param[in]  This              Pointer to a
                                TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL structure.
  @param[in]  Channel           The channel to read
  @param[out] AdcValue          Pointer to a buffer to receive the value.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The conversion was done successfully.
  @retval EFI_INVALID_PARAMETER AdcValue was NULL
  @retval EFI_INVALID_PARAMETER Channel > 7
**/
EFI_STATUS
EFIAPI
AdcReadChannel (
  IN CONST TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL *This,
  IN UINT8 Channel,
  IN UINT16 *AdcValue
  )
{
  ADC108S102 *Adc108s102;
  UINT16 ReadData;
  EFI_STATUS Status;
  UINT16 Value;
  UINT16 WriteData;

  //
  // Get the driver data structure
  //
  Adc108s102 = ADC108S102_CONTEXT_FROM_PROTOCOL (This);

  //
  // Verify the input parameters
  //
  if (AdcValue == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Data is NULL!\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Failure;
  }
  if (Channel > 7) {
    DEBUG ((EFI_D_ERROR, "ERROR - Channel > 7!\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Failure;
  }

  //
  // Determine if this is the channel that the ADC is reading
  //
  if (Adc108s102->NextChannel != Channel) {
    Adc108s102->NextChannel = Channel;

    //
    // Request that the ADC read the desired channel
    //
    Status = AdcReadChannel(This, Channel, AdcValue);
    if (!EFI_ERROR(Status)) {
      goto Failure;
    }
  }

  //
  // Read the ADC channel value
  //
  WriteData = Channel << 11;
  Status = Adc108s102->SpiIo->Transaction(
                    Adc108s102->SpiIo,           // EFI_SPI_IO_PROTOCOL
                    SPI_TRANSACTION_FULL_DUPLEX, // TransactionType
                    FALSE,                       // DebugTransaction
                    0,                           // Use maximum clock frequency
                    1,                           // Bus width in bits
                    16,                          // 8-bits per frame
                    sizeof(WriteData),           // WriteBytes
                    (UINT8 *)&WriteData,         // WriteBuffer
                    sizeof(ReadData),            // ReadBytes
                    (UINT8 *)&ReadData           // ReadBuffer
                    );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Adc108s102 failed channel read, Status: %r\n", Status));
    goto Failure;
  }

  //
  // Correct the ADC value
  //
  Value = ReadData & 0x0fff;
  *AdcValue = Value >> 2;

Failure:
  return Status;
}

/**
  Shuts down the driver.

  This routine must be called at or below TPL_NOTIFY.

  This routine deallocates the resources supporting the SPI bus operation.

  @param[in]  Adc108s102        Pointer to a Adc108s102 data structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
STATIC
VOID
EFIAPI
AdcShutdownWorker (
  IN ADC108S102 *Adc108s102
  )
{
  TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL *Adc108s102Protocol;
  EFI_STATUS Status;

  //
  // Determine if the job is already done
  //
  if (Adc108s102 != NULL) {
    //
    // Release the SPI IO protocol
    //
    if (Adc108s102->SpiIo != NULL) {
      gBS->CloseProtocol (
             Adc108s102->ControllerHandle,
             (VOID *)&TexasInstruments_ADC108S102_Driver,
             Adc108s102->DriverBinding->DriverBindingHandle,
             Adc108s102->ControllerHandle
             );
    }

    //
    // Determine if the TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL is present
    //
    Status = gBS->OpenProtocol (
                    Adc108s102->ControllerHandle,
                    &gTexasInstrumentsAdc108s102ProtocolGuid,
                    (VOID **) &Adc108s102Protocol,
                    Adc108s102->DriverBinding->DriverBindingHandle,
                    Adc108s102->ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (Status == EFI_SUCCESS) {
      //
      // Remove the TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL
      //
      Status = gBS->UninstallProtocolInterface (
                       Adc108s102->ControllerHandle,
                       &gTexasInstrumentsAdc108s102ProtocolGuid,
                       Adc108s102Protocol
                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - ADC108S102 failed to remove TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL!\n"));
        ASSERT_EFI_ERROR(Status);
      }
    }

    //
    // Free the data structure
    //
    FreePool (Adc108s102);
  }
}

/**
  Shuts down the SPI bus layer.

  This routine must be called at or below TPL_NOTIFY.

  This routine determines if the bus layer is not busy.  Once it is idle, the
  supporting resources are deallocated.

  @param[in]  Adc108s102Protocol A pointer to the
                                 TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL instance

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
EFI_STATUS
EFIAPI
AdcShutdown (
  IN TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL *Adc108s102Protocol
  )
{
  EFI_STATUS Status;

DEBUG ((EFI_D_ERROR, "AdcShutdown entered\n"));
  Status = EFI_UNSUPPORTED;
DEBUG ((EFI_D_ERROR, "AdcShutdown exiting, Status: %r\n", Status));
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
AdcStartup (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN EFI_HANDLE ControllerHandle
  )
{
  
  ADC108S102 *Adc108s102;
  EFI_STATUS Status;

  //
  // Allocate the controller data structure
  //
  Adc108s102 = AllocateZeroPool (sizeof (*Adc108s102));
  if (Adc108s102 == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate SPI_BUS!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save the inputs for the shutdown operation
  //
  Adc108s102->Signature = ADC108S102_SIGNATURE;
  Adc108s102->DriverBinding = DriverBinding;
  Adc108s102->ControllerHandle = ControllerHandle;

  //
  // Get access to the SPI IO Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  (VOID *)&TexasInstruments_ADC108S102_Driver,
                  (VOID **)&Adc108s102->SpiIo,
                  DriverBinding->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Adc108s102 failed to open SPI IO protocol!\n"));
    goto Failure;
  }

  //
  // Initialize the ADC108S102 protocol
  //
  Adc108s102->NextChannel = 0xff;
  Adc108s102->Adc108s102Protocol.SpiPeripheral =
                   Adc108s102->SpiIo->SpiPeripheral;
  Adc108s102->Adc108s102Protocol.ReadChannel = AdcReadChannel;

  //
  // Install the TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL
  //
  Status = gBS->InstallProtocolInterface (
                   &ControllerHandle,
                   &gTexasInstrumentsAdc108s102ProtocolGuid,
                   EFI_NATIVE_INTERFACE,
                   &Adc108s102->Adc108s102Protocol
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Adc108s102 failed to install TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL!\n"));
    goto Failure;
  }
  return EFI_SUCCESS;

Failure:
  //
  // Release the SPI host controller resources
  //
  AdcShutdownWorker (Adc108s102);
  return Status;
}
