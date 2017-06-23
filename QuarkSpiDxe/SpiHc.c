/** @file

  This module implements the SPI host controller protocol.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
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

#include "QuarkSpiDxe.h"

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
EFI_STATUS
EFIAPI
SpiHcChipSelect (
  IN CONST EFI_SPI_HC_PROTOCOL *This,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  IN BOOLEAN PinValue
  )
{
  EFI_STATUS Status;

DEBUG ((EFI_D_ERROR, "SpiHcChipSelect entered\n"));
  Status = EFI_UNSUPPORTED;
DEBUG ((EFI_D_ERROR, "SpiHcChipSelect exiting, Status: %r\n", Status));
  return Status;
}

/**
  Set up the clock generator to produce the correct clock frequency, phase and
  polarity for a SPI chip.

  This routine is called at TPL_NOTIFY.

  This routine updates the clock generator to generate the correct frequency and
  polarity for the SPI clock.

  @param[in]  This              Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in]  SpiPeripheral     Pointer to a EFI_SPI_PERIPHERAL data structure
                                from which the routine can access the
                                ClockParameter and ClockHalves01 fields.  The
                                routine also has access to the names for the SPI
                                bus and chip which can be used during debugging.
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
EFI_STATUS
EFIAPI
SpiHcClock (
  IN CONST EFI_SPI_HC_PROTOCOL *This,
  IN CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  IN UINT32 *ClockHz
  )
{
  UINT32 ClockFrequency;
  SPI_HC *SpiHc;
  UINT64 Temp;

  //
  // Get the host controller context
  //
  SpiHc = SPI_HC_CONTEXT_FROM_PROTOCOL(This);

  //
  // Don't exceed the maximum frequency of the clock controller
  //
  ClockFrequency = *ClockHz;
  if (ClockFrequency > MHz(100)) {
    ClockFrequency = MHz(100);
  }

  //
  // Determine the clock divider for this SPI transaction
  //
  //    Data sections 20.5.5 and 20.5.1
  //    SYS_FREQ = 200 MHz (Input clock)
  //    DDS_FREQ = SYS_FREQ * DDS_CLOCK_RATE / 0x1000000;
  //    SCLK = DDS_FREQ/(2 * (SCR + 1))
  //
  // Using fixed SCR of zero (0)
  //
  Temp = MultU64x32(ClockFrequency, BIT24);
  Temp = MultU64x32(Temp, 2);
  SpiHc->ClockRate = (UINT32)DivU64x32(Temp, SPI_INPUT_CLOCK);
  SpiHc->Sscr0 = 0 << SSCR0_SCR_SHIFT;

  //
  // Determine the clock frequency for this SPI transaction
  //
  Temp = MultU64x32(SPI_INPUT_CLOCK, SpiHc->ClockRate);
  *ClockHz = (UINT32)DivU64x32(Temp, BIT24 * 2);

  //
  // Determine the clock phase and polarity
  //
  SpiHc->Sscr1 = (SpiPeripheral->ClockPhase ? SSCR1_SPH : 0)
               | (SpiPeripheral->ClockPolarity ? 0 : SSCR1_SPO);
  return EFI_SUCCESS;
}

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
EFI_STATUS
EFIAPI
SpiHcTransaction (
  IN CONST EFI_SPI_HC_PROTOCOL *This,
  IN EFI_SPI_BUS_TRANSACTION *BusTransaction
  )
{
  UINT32 BaseAddress;
  union {
    volatile UINT32 *Reg;
    UINT32 U32;
  } Controller;
  UINT32 FrameSize;
  UINT8 *ReadBuffer;
  UINTN ReadBytes;
  SPI_HC *SpiHc;
  SPI_TRANSACTION SpiTransaction;
  EFI_STATUS Status;
  UINT8 *WriteBuffer;
  UINTN WriteBytes;

  //
  // Get the SPI controller context structure
  //
  SpiHc = SPI_HC_CONTEXT_FROM_PROTOCOL(This);
  BaseAddress = SpiHc->BaseAddress;

  //
  // Verify the transaction type independent input parameters
  //
  FrameSize = BusTransaction->FrameSize;
  ASSERT (FrameSize <= 32);
  ASSERT ((This->FrameSizeSupportMask & (1 << (FrameSize - 1))) != 0);

  WriteBytes = BusTransaction->WriteBytes;
  WriteBuffer = BusTransaction->WriteBuffer;
  ReadBytes = BusTransaction->ReadBytes;
  ReadBuffer = BusTransaction->ReadBuffer;
  SpiTransaction = NULL;
  Status = EFI_SUCCESS;

  //
  // Verify the input parameters based upon the transaction type
  //
  switch (BusTransaction->TransactionType) {
  default:
  case SPI_TRANSACTION_READ_ONLY:
    //
    // Data flowing from the SPI peripheral to the host.  WriteBytes must be
    // zero.  ReadBytes must be non-zero and ReadBuffer must be provided.
    //
    Status = EFI_UNSUPPORTED;
    break;

  case SPI_TRANSACTION_WRITE_THEN_READ:
    //
    // Data first flowing from the host to the SPI peripheral and then data
    // flows from the SPI peripheral to the host.  These types of operations
    // get used for SPI flash devices when control data (opcode, address) must
    // be passed to the SPI peripheral to specify the data to be read.
    //
    ASSERT (WriteBytes != 0);
    ASSERT (WriteBuffer != NULL);
    ASSERT (ReadBytes != 0);
    ASSERT (ReadBuffer != NULL);
    if (BusTransaction->FrameSize <= 8) {
      SpiTransaction = (SPI_TRANSACTION)SpiHc8BitWriteThenReadTransaction;
    } else if (BusTransaction->FrameSize <= 16) {
      SpiTransaction = (SPI_TRANSACTION)SpiHc16BitWriteThenReadTransaction;
    } else {
      SpiTransaction = (SPI_TRANSACTION)SpiHc32BitWriteThenReadTransaction;
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiHc: Starting the write-then-read SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiHc: Sending data from 0x%08x, 0x%08x bytes\n",
              WriteBuffer, WriteBytes));
      DEBUG ((EFI_D_ERROR, "SpiHc: Receiving data into 0x%08x, 0x%08x bytes\n",
              ReadBuffer, ReadBytes));
    }
    break;

  case SPI_TRANSACTION_WRITE_ONLY:
    //
    // Data flowing from the host to the SPI peripheral.  ReadBytes must be
    // zero.  WriteBytes must be non-zero and WriteBuffer must be provided.
    //
    ASSERT (WriteBytes != 0);
    ASSERT (WriteBuffer != NULL);
    ASSERT (ReadBytes == 0);
    ReadBytes = WriteBytes;
    if (BusTransaction->FrameSize <= 8) {
      SpiTransaction = (SPI_TRANSACTION)SpiHc8BitWriteOnlyTransaction;
    } else if (BusTransaction->FrameSize <= 16) {
      SpiTransaction = (SPI_TRANSACTION)SpiHc16BitWriteOnlyTransaction;
    } else {
      SpiTransaction = (SPI_TRANSACTION)SpiHc32BitWriteOnlyTransaction;
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiHc: Starting the write-only SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiHc: Sending data from 0x%08x, 0x%08x bytes\n",
              WriteBuffer, WriteBytes));
    }
    break;

  case SPI_TRANSACTION_FULL_DUPLEX:
    //
    // Data flowing in both direction between the host and SPI peripheral.
    // ReadBytes must equal WriteBytes and both ReadBuffer and WriteBuffer
    // must be provided.
    //
    ASSERT (WriteBytes != 0);
    ASSERT (WriteBuffer != NULL);
    ASSERT (ReadBytes != 0);
    ASSERT (ReadBuffer != NULL);
    if (BusTransaction->FrameSize <= 8) {
      SpiTransaction = (SPI_TRANSACTION)SpiHc8BitFullDuplexTransaction;
    } else if (BusTransaction->FrameSize <= 16) {
      SpiTransaction = (SPI_TRANSACTION)SpiHc16BitFullDuplexTransaction;
    } else {
      SpiTransaction = (SPI_TRANSACTION)SpiHc32BitFullDuplexTransaction;
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiHc: Starting the full-duplex SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiHc: Sending data from 0x%08x, 0x%08x bytes\n",
              WriteBuffer, WriteBytes));
      DEBUG ((EFI_D_ERROR, "SpiHc: Receiving data into 0x%08x, 0x%08x bytes\n",
              ReadBuffer, ReadBytes));
    }
    break;
  }

  if (!EFI_ERROR(Status)) {
    //
    // Set-up the clock and enable the SPI controller
    //
    Controller.U32 = BaseAddress + DDS_RATE;
    *Controller.Reg = SpiHc->ClockRate;

    Controller.U32 = BaseAddress + SSCR1;
    *Controller.Reg = SpiHc->Sscr1;
    MemoryFence ();

    Controller.U32 = BaseAddress + SSCR0;
    *Controller.Reg = SpiHc->Sscr0 | SSCR0_SSE | (FrameSize - 1);
    MemoryFence ();

    //
    // Start the SPI transaction
    //
    SpiTransaction (BaseAddress,
                    WriteBytes,
                    WriteBuffer,
                    ReadBytes,
                    ReadBuffer);

    //
    // Disable the SPI controller
    //
    Controller.U32 = BaseAddress + SSCR0;
    *Controller.Reg = 0;
    MemoryFence ();
  }

  //
  // Return the transaction status
  //
  return Status;
}

/**
  Shuts down the SPI host controller.

  This routine must be called at or below TPL_NOTIFY.

  This routine determines if the device is not busy.  Once it is idle, the
  supporting resources are deallocated and the protocol is removed from the
  system.

  @param[in]  SpiHc             Pointer to a SPI_HC data structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
STATIC
VOID
EFIAPI
SpiHcShutdownWorker (
  IN SPI_HC *SpiHc
  )
{
  EFI_SPI_HC_PROTOCOL *SpiHcProtocol;
  EFI_STATUS Status;

  //
  // Determine if the job is already done
  //
  if (SpiHc != NULL) {
    //
    // Release the PCI IO protocol
    //
    if (SpiHc->PciIo != NULL) {
      gBS->CloseProtocol (
             SpiHc->ControllerHandle,
             &gEfiPciIoProtocolGuid,
             SpiHc->DriverBinding->DriverBindingHandle,
             SpiHc->ControllerHandle
             );
    }

    //
    // Determine if the SPI host controller protocol is present
    //
    Status = gBS->OpenProtocol (
                    SpiHc->ControllerHandle,
                    &gEfiSpiHcProtocolGuid,
                    (VOID **) &SpiHcProtocol,
                    SpiHc->DriverBinding->DriverBindingHandle,
                    SpiHc->ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (Status == EFI_SUCCESS) {
      //
      // Remove the SPI host controller protocol
      //
      Status = gBS->UninstallProtocolInterface (
                       SpiHc->ControllerHandle,
                       &gEfiSpiHcProtocolGuid,
                       &SpiHc->SpiHcProtocol
                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - SpiHc failed to remove SPI HC protocol!\n"));
        ASSERT_EFI_ERROR(Status);
      }
    }

    //
    // Free the data structure
    //
    FreePool (SpiHc);
  }
}

/**
  Shuts down the SPI host controller.

  This routine must be called at or below TPL_NOTIFY.

  This routine determines if the device is not busy.  Once it is idle, the
  supporting resources are deallocated and the protocol is removed from the
  system.

  @param[in]  DriverBinding     A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                instance.
  @param[in]  SpiHcProtocol     Pointer to an EFI_SPI_HC_PROTOCOL structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The shutdown operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device is still busy.
**/
EFI_STATUS
EFIAPI
SpiHcShutdown (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN CONST EFI_SPI_HC_PROTOCOL *SpiHcProtocol
  )
{
  EFI_STATUS Status;

DEBUG ((EFI_D_ERROR, "SpiHcShutdown entered\n"));
  Status = EFI_UNSUPPORTED;
DEBUG ((EFI_D_ERROR, "SpiHcShutdown exiting, Status: %r\n", Status));
  return Status;
}

/**
  Set up the necessary pieces to start SPI host controller for this device.

  This routine must be called at or below TPL_NOTIFY.

  Initialize the context data structure to support SPI host controller.  Gain
  access to the PCI IO protocol.  Upon successful completion, install the SPI
  host controller protocol.

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
SpiHcStartup (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN EFI_HANDLE ControllerHandle
  )
{
  UINT16 Command;
  SPI_HC *SpiHc;
  EFI_STATUS Status;

  //
  // Allocate the SPI host controller data structure
  //
  SpiHc = AllocateZeroPool (sizeof (SPI_HC));
  if (SpiHc == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate SPI_HC!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save the inputs for the shutdown operation
  //
  SpiHc->DriverBinding = DriverBinding;
  SpiHc->ControllerHandle = ControllerHandle;

  //
  // Open the PCI I/O Protocol on ControllerHandle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&SpiHc->PciIo,
                  DriverBinding->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiHc failed to open PCI IO protocol!\n"));
    goto Failure;
  }

  //
  // Get the SPI host controller base address
  //
  Status = SpiHc->PciIo->Pci.Read(
                  SpiHc->PciIo,
                  EfiPciIoWidthUint32,
                  PCI_BASE_ADDRESSREG_OFFSET,
                  1,
                  &SpiHc->BaseAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiHc failed to get SPI HC base address!\n"));
    goto Failure;
  }
  SpiHc->BaseAddress &= ~0xf;
  DEBUG ((EFI_D_INFO, "0x%08x: SPI HC Base Address\n", SpiHc->BaseAddress));

  //
  // Enable the SPI host controller if necessary
  //
  Status = SpiHc->PciIo->Pci.Read(
                  SpiHc->PciIo,
                  EfiPciIoWidthUint16,
                  PCI_COMMAND_OFFSET,
                  1,
                  &Command
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiHc failed to get SPI HC base address!\n"));
    goto Failure;
  }
  if ((Command & (EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER))
    != (EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER)) {
    Command |= EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER;
    Status = SpiHc->PciIo->Pci.Write(
                    SpiHc->PciIo,
                    EfiPciIoWidthUint16,
                    PCI_COMMAND_OFFSET,
                    1,
                    &Command
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "ERROR - SpiHc failed to get SPI HC base address!\n"));
      goto Failure;
    }
    DEBUG ((EFI_D_INFO, "Enabled SPI host controller\n"));
  }

  //
  // Initialize the SPI host controller protocol
  //
  SpiHc->Signature = SPI_HC_SIGNATURE;
  SpiHc->SpiHcProtocol.ChipSelect = SpiHcChipSelect;
  SpiHc->SpiHcProtocol.Clock = SpiHcClock;
  SpiHc->SpiHcProtocol.Transaction = SpiHcTransaction;
  SpiHc->SpiHcProtocol.Attributes = HC_SUPPORTS_WRITE_ONLY_OPERATIONS
                                  | HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS
                                  | HC_TRANSFER_SIZE_INCLUDES_OPCODE
                                  | HC_TRANSFER_SIZE_INCLUDES_ADDRESS;
  SpiHc->SpiHcProtocol.FrameSizeSupportMask =
           (UINT32)( SUPPORT_FRAME_SIZE_BITS (4)
                   | SUPPORT_FRAME_SIZE_BITS (5)
                   | SUPPORT_FRAME_SIZE_BITS (6)
                   | SUPPORT_FRAME_SIZE_BITS (7)
                   | SUPPORT_FRAME_SIZE_BITS (8)
                   | SUPPORT_FRAME_SIZE_BITS (9)
                   | SUPPORT_FRAME_SIZE_BITS (10)
                   | SUPPORT_FRAME_SIZE_BITS (11)
                   | SUPPORT_FRAME_SIZE_BITS (12)
                   | SUPPORT_FRAME_SIZE_BITS (13)
                   | SUPPORT_FRAME_SIZE_BITS (14)
                   | SUPPORT_FRAME_SIZE_BITS (15)
                   | SUPPORT_FRAME_SIZE_BITS (16)
                   | SUPPORT_FRAME_SIZE_BITS (17)
                   | SUPPORT_FRAME_SIZE_BITS (18)
                   | SUPPORT_FRAME_SIZE_BITS (19)
                   | SUPPORT_FRAME_SIZE_BITS (20)
                   | SUPPORT_FRAME_SIZE_BITS (21)
                   | SUPPORT_FRAME_SIZE_BITS (22)
                   | SUPPORT_FRAME_SIZE_BITS (23)
                   | SUPPORT_FRAME_SIZE_BITS (24)
                   | SUPPORT_FRAME_SIZE_BITS (25)
                   | SUPPORT_FRAME_SIZE_BITS (26)
                   | SUPPORT_FRAME_SIZE_BITS (27)
                   | SUPPORT_FRAME_SIZE_BITS (28)
                   | SUPPORT_FRAME_SIZE_BITS (29)
                   | SUPPORT_FRAME_SIZE_BITS (30)
                   | SUPPORT_FRAME_SIZE_BITS (31)
                   | SUPPORT_FRAME_SIZE_BITS (32)
                   );
  SpiHc->SpiHcProtocol.MaximumTransferBytes = 0xffffffff;

  //
  // Install the SPI host controller protocol
  //
  Status = gBS->InstallProtocolInterface (
                   &ControllerHandle,
                   &gEfiSpiHcProtocolGuid,
                   EFI_NATIVE_INTERFACE,
                   &SpiHc->SpiHcProtocol
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiHc failed to install SPI HC protocol!\n"));
    goto Failure;
  }
  return EFI_SUCCESS;

Failure:
  //
  // Release the SPI host controller resources
  //
  SpiHcShutdownWorker (SpiHc);
  return Status;
}
