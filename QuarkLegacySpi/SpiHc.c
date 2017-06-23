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

#include "QuarkLegacySpi.h"

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
  LEGACY_SPI_CONFIG *LegacySpiConfig;
  SPI_HC *SpiHc;
  EFI_STATUS Status;

  //
  // Get the SPI controller context structure
  //
  SpiHc = SPI_HC_CONTEXT_FROM_PROTOCOL(This);

  //
  // Set the chip select
  //
  ASSERT (SpiPeripheral->ChipSelectParameter != NULL);
  LegacySpiConfig = SpiPeripheral->ChipSelectParameter;
  if (PinValue == 0) {
    SpiHc->ChipSelect = LegacySpiConfig->ChipSelect & SPIADDR_CSC;
  } else {
    SpiHc->ChipSelect = SPIADDR_CSC;
  }
  Status = EFI_SUCCESS;
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

  //
  // Don't exceed the maximum frequency of the clock controller
  //
  ClockFrequency = *ClockHz;
  if (ClockFrequency > SPI_INPUT_CLOCK) {
    ClockFrequency = SPI_INPUT_CLOCK;
  }
  if ((ClockFrequency != SPI_INPUT_CLOCK) && (ClockFrequency != 0)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - SpiHc does not support %d Hz, must be >= %d Hz\n",
            ClockFrequency, SPI_INPUT_CLOCK));
    return EFI_UNSUPPORTED;
  }
  *ClockHz = ClockFrequency;
  return EFI_SUCCESS;
}

/**
  Update an entry in the flash controller's prefix table.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  SpiHc             Pointer to the SPI_HC data structure.
  @param[in]  Index             Index into the prefix table
  @param[in]  Prefix            Prefix opcode value

**/
VOID
SpiHcPrefix (
  SPI_HC *SpiHc,
  UINTN  Index,
  UINT8  Prefix
  )
{
  union {
    volatile UINT8 *Reg;
    UINT32 U32;
  } Controller;

  //
  // Set the prefix byte
  //
  ASSERT (Index <= 1);
  Controller.U32 = SpiHc->BaseAddress + PREOP + Index;
  *Controller.Reg = Prefix;
}

/**
  Return the entry in the flash controller's prefix table.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  SpiHc             Pointer to the SPI_HC data structure.
  @param[in]  Index             Index into the prefix table

**/
UINT8
SpiHcReadPrefix (
  SPI_HC *SpiHc,
  UINTN  Index
  )
{
  union {
    volatile UINT8 *Reg;
    UINT32 U32;
  } Controller;

  //
  // Get the prefix byte
  //
  ASSERT (Index <= 1);
  Controller.U32 = SpiHc->BaseAddress + PREOP + Index;
  return *Controller.Reg;
}

/**
  Update an entry in the flash controller's opcode table.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  SpiHc             Pointer to the SPI_HC data structure.
  @param[in]  Index             Index into the prefix table
  @param[in]  Type              Type of opcode
  @param[in]  Opcode            pcode value

**/
VOID
SpiHcOpcode (
  SPI_HC *SpiHc,
  UINTN  Index,
  UINTN  Type,
  UINT8  Opcode
  )
{
  union {
    volatile UINT8 *Reg8;
    volatile UINT16 *Reg16;
    UINT32 U32;
  } Controller;
  UINT16 OpcodeTypes;
  UINT16 OpcodeTypeShift;

  //
  // Update the opcode type
  //
  ASSERT (Index <= 7);
  Controller.U32 = SpiHc->BaseAddress + OPTYPE;
  OpcodeTypeShift = (UINT16)Index * 2;
  OpcodeTypes = *Controller.Reg16;
  OpcodeTypes &= ~(OPTYPE_MASK << OpcodeTypeShift);
  OpcodeTypes |= Type << OpcodeTypeShift;
  *Controller.Reg16 = OpcodeTypes;

  //
  // Update the opcode
  //
  Controller.U32 = SpiHc->BaseAddress + OPMENU_1 + Index;
  *Controller.Reg8 = Opcode;
}

/**
  Return the entry in the flash controller's opcode table.

  This routine must be called at or below TPL_NOTIFY.

  @param[in]  SpiHc             Pointer to the SPI_HC data structure.
  @param[in]  Index             Index into the prefix table

**/
UINT8
SpiHcReadOpcode (
  SPI_HC *SpiHc,
  UINTN  Index
  )
{
  union {
    volatile UINT8 *Reg;
    UINT32 U32;
  } Controller;

  //
  // Get the opcode byte
  //
  ASSERT (Index <= 7);
  Controller.U32 = SpiHc->BaseAddress + OPMENU_1 + Index;
  return *Controller.Reg;
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
  @retval EFI_ACCESS_DENIED     The SPI controller is locked
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
  UINTN Address;
  UINT32 BaseAddress;
  UINT32 BiosControl;
  UINT32 BiosControlSaved;
  UINT16 Control;
  union {
    volatile UINT8 *Reg8;
    volatile UINT16 *Reg16;
    volatile UINT32 *Reg32;
    UINT32 U32;
  } Controller;
  UINT8 Data;
  UINT32 Data32;
  UINT32 FlashAddress;
  UINT32 FrameSize;
  UINTN Index;
  UINT8 Opcode;
  UINT8 *ReadBuffer;
  UINTN ReadBytes;
  UINTN Type;
  SPI_HC *SpiHc;
  UINT16 SpiStatus;
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
  FlashAddress = 0;
  Status = EFI_SUCCESS;

  //
  // Verify the input parameters based upon the transaction type
  //
  switch (BusTransaction->TransactionType) {
  case SPI_TRANSACTION_FULL_DUPLEX:
    //
    // Data flowing in both direction between the host and SPI peripheral.
    // ReadBytes must equal WriteBytes and both ReadBuffer and WriteBuffer
    // must be provided.
    //
  case SPI_TRANSACTION_READ_ONLY:
    //
    // Data flowing from the SPI peripheral to the host.  WriteBytes must be
    // zero.  ReadBytes must be non-zero and ReadBuffer must be provided.
    //
  default:
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
    ASSERT (ReadBytes <= 64);
    ASSERT (ReadBuffer != NULL);

    //
    // Assume a read operation
    //
    SpiHc->Flags &= ~SPI_HC_FLAG_PREFIX_SENT;
    Opcode = *WriteBuffer;
    if (Opcode == OPCODE_READ_DATA) {
      ///
      /// Read data from the SPI NOR flash part
      /// One command byte and 3 address bytes to send followed by one or more
      /// bytes of data to receive
      ///
      ASSERT (WriteBytes == 4);
      Index = OPCODE_READ_DATA_INDEX;
      FlashAddress = (WriteBuffer[1] << 16) | (WriteBuffer[2] << 8)
                   | WriteBuffer[3] | SpiHc->ChipSelect;
    } else if (Opcode == OPCODE_READ_STATUS) {
      ///
      /// Read status register
      /// One command byte followed by one or two bytes to receive
      ///
      ASSERT (WriteBytes == 1);
      Index = OPCODE_READ_STATUS_INDEX;
    } else if (Opcode == OPCODE_READ_ID) {
      ///
      /// Read the three bytes of manufacture and device ID
      /// One command byte to send followed by 3 bytes of data to receive
      ///
      ASSERT (WriteBytes == 1);
      Index = OPCODE_READ_ID_INDEX;
    } else {
      //
      // Unknown opcode, map it to slot 0 if the controller is unlocked
      //
      if (SpiHc->ControllerLocked) {
        if (BusTransaction->DebugTransaction) {
          DEBUG ((EFI_D_ERROR,
                  "ERROR - SpiHc controller is locked!\n"));
        }
        Status = EFI_ACCESS_DENIED;
        break;
      }
      Type = OPTYPE_READ_NO_ADDR;
      if (WriteBytes == 4) {
        FlashAddress = (WriteBuffer[1] << 16) | (WriteBuffer[2] << 8)
                     | WriteBuffer[3] | SpiHc->ChipSelect;
        Type = OPTYPE_READ_ADDR;
      } else if (WriteBytes != 1) {
        if (BusTransaction->DebugTransaction) {
          DEBUG ((EFI_D_ERROR,
                  "ERROR - SpiHc could not properly map transaction!\n"));
        }
        Status = EFI_DEVICE_ERROR;
        break;
      }
      Index = 0;
      SpiHcOpcode (SpiHc, Index, Type, Opcode);
    }

    //
    // Initiate the read operation
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR,
              "SpiHc: Starting the write-then-read SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiHc: Sending data from 0x%08x, 0x%08x bytes\n",
              WriteBuffer, WriteBytes));
      DEBUG ((EFI_D_ERROR, "SpiHc: Receiving data into 0x%08x, 0x%08x bytes\n",
              ReadBuffer, ReadBytes));
    }
    Controller.U32 = BaseAddress + SPIADDR;
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "0x%08x <-- 0x%08x\n", Controller.U32,
              FlashAddress));
    }
    *Controller.Reg32 = FlashAddress;
    *Controller.Reg32;
    Controller.U32 = BaseAddress + SPICTL;
    Control = (UINT16)(SPICTL_DC | SPICTL_ACS | SPICTL_AR | (Index << SPICTL_COPTR_SHIFT)
            | ((ReadBytes - 1) << SPICTL_DBCNT_SHIFT) | SPICTL_CG);
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "0x%08x <-- 0x%04x\n", Controller.U32, Control));
    }
    *Controller.Reg16 = Control;
    *Controller.Reg16;

    //
    // Wait for the operation to complete
    //
    Controller.U32 = BaseAddress + SPISTS;
    do {
      SpiStatus = *Controller.Reg16;
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "0x%08x --> 0x%04x\n", Controller.U32, SpiStatus));
      }
    } while ((SpiStatus & SPISTS_CIP) != 0);
    if ((SpiStatus & SPISTS_BA) != 0) {
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - SpiHc blocked access, transaction failed!\n"));
      }
      Status = EFI_ACCESS_DENIED;
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "0x%08x <-- 0x%04x\n", Controller.U32,
              SPISTS_BA | SPISTS_CD));
    }
    *Controller.Reg16 = SPISTS_BA | SPISTS_CD;
    *Controller.Reg16;
    if (EFI_ERROR(Status)) {
      break;
    }

    //
    // Return the data
    //
    ASSERT (ReadBytes <= 64);
    Controller.U32 = BaseAddress + SPID0_1;
    while (ReadBytes >= 4) {
      Data32 = *Controller.Reg32;
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "0x%08x --> 0x%08x\n", Controller.U32, Data32));
      }
      *(UINT32 *)ReadBuffer = Data32;
      ReadBuffer += 4;
      ReadBytes -= 4;
      Controller.U32 += 4;
    }
    while (ReadBytes--) {
      Data = *Controller.Reg8;
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "0x%08x --> 0x%02x\n", Controller.U32, Data));
      }
      *ReadBuffer++ = Data;
      Controller.U32 += 1;
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

    //
    // This is a hardware flash controller.  As such, write operations must
    // send the prefix byte as a separate command prior to any write data.
    // Consume single byte transactions which match one of the prefix opcodes.
    // Flag which opcode should be used on the next SPI transaction.
    //
    Opcode = *WriteBuffer;
    if (Opcode == SpiHcReadPrefix (SpiHc, 0)) {
      SpiHc->Flags = SPI_HC_FLAG_PREFIX_SENT;
      break;
    } else if (Opcode == SpiHcReadPrefix (SpiHc, 1)) {
      SpiHc->Flags = SPI_HC_FLAG_USE_PREFIX_1 | SPI_HC_FLAG_PREFIX_SENT;
      break;
    } else if ((SpiHc->Flags & SPI_HC_FLAG_PREFIX_SENT) == 0) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - SpiHc prefix not sent, transaction failed!\n"));
      Status = EFI_DEVICE_ERROR;
      break;
    }
    SpiHc->Flags &= ~SPI_HC_FLAG_PREFIX_SENT;

    //
    // Determine the type of write operation that is being performed.
    //
    if (Opcode == OPCODE_WRITE_DATA) {
      ///
      /// Page program: Write data to the SPI NOR flash part
      /// One prefix byte, one command byte and 3 address bytes to send
      /// followed by up to 256 bytes of data
      ///
      ASSERT (WriteBytes > 4);
      ASSERT (WriteBytes <= (4 + 64));
      Index = OPCODE_WRITE_DATA_INDEX;
      FlashAddress = (WriteBuffer[1] << 16) | (WriteBuffer[2] << 8)
                   | WriteBuffer[3] | SpiHc->ChipSelect;
      WriteBuffer += 4;
      WriteBytes -= 4;
    } else if (Opcode == OPCODE_ERASE_4KB) {
      ///
      /// Erase 4 KBytes
      /// One prefix byte, one command byte and 3 address bytes to send
      ///
      ASSERT (WriteBytes == 4);
      Index = OPCODE_ERASE_4KB_INDEX;
      WriteBuffer += 1;
      WriteBytes -= 1;
    } else if (Opcode == SpiHcReadOpcode (SpiHc, OPCODE_ERASE_BLOCK_INDEX)) {
      ///
      /// Erase block
      /// One prefix byte, one command byte and 3 address bytes to send
      ///
      ASSERT (WriteBytes == 4);
      Index = OPCODE_ERASE_BLOCK_INDEX;
      WriteBuffer += 1;
      WriteBytes -= 1;
    } else if (Opcode == OPCODE_WRITE_STATUS) {
      ///
      /// Write status
      /// One prefix byte, one command byte and one or two bytes of data to send
      ///
      ASSERT (WriteBytes > 1);
      ASSERT (WriteBytes <= (1 + 64));
      Index = OPCODE_WRITE_STATUS_INDEX;
      WriteBuffer += 1;
      WriteBytes -= 1;
    } else {
      //
      // Unknown opcode, map it to slot 0 if the controller is unlocked
      //
      if (SpiHc->ControllerLocked) {
        if (BusTransaction->DebugTransaction) {
          DEBUG ((EFI_D_ERROR,
                  "ERROR - SpiHc controller is locked!\n"));
        }
        Status = EFI_ACCESS_DENIED;
        break;
      }
      WriteBuffer += 1;
      WriteBytes -= 1;
      Type = OPTYPE_WRITE_NO_ADDR;
      if (WriteBytes > 64) {
        FlashAddress = (WriteBuffer[0] << 16) | (WriteBuffer[1] << 8)
                     | WriteBuffer[2] | SpiHc->ChipSelect;
        WriteBuffer += 3;
        WriteBytes -= 3;
        Type = OPTYPE_WRITE_ADDR;
      }
      Index = 0;
      SpiHcOpcode (SpiHc, Index, Type, Opcode);
    }

    //
    // Initiate the write operation
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "SpiHc: Starting the write-only SPI transaction\n"));
      DEBUG ((EFI_D_ERROR, "SpiHc: Sending data from 0x%08x, 0x%08x bytes\n",
              WriteBuffer, WriteBytes));
    }

    //
    // Disable prefetch and caching of the SPI flash
    // Enable writes to the SPI flash
    //
    Address = PCI_LIB_ADDRESS (0, 31, 0, BC);
    BiosControlSaved = PciRead32 (Address);
    BiosControl = (BiosControlSaved & ~BC_PFE) | BC_CD | BC_WPD;
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "BIOS Control --> 0x%08x\n", BiosControlSaved));
      DEBUG ((EFI_D_ERROR, "BIOS Control <-- 0x%08x\n", BiosControl));
    }
    PciWrite32 (Address, BiosControl);

    //
    // Setup the hardware flash controller for the transfer
    //
    Controller.U32 = BaseAddress + SPIADDR;
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "0x%08x <-- 0x%08x\n", Controller.U32,
              FlashAddress));
    }
    *Controller.Reg32 = FlashAddress;
    *Controller.Reg32;
    Control = (UINT16)(SPICTL_DC | SPICTL_ACS | SPICTL_AR | ((UINT16)Index << SPICTL_COPTR_SHIFT)
            | ((SpiHc->Flags & SPI_HC_FLAG_USE_PREFIX_1) ? SPICTL_SOPTR : 0)
            | ((WriteBytes - 1) << SPICTL_DBCNT_SHIFT) | SPICTL_CG);

    //
    // Move the write data into the hardware flash controller
    //
    ASSERT (WriteBytes <= 64);
    Controller.U32 = BaseAddress + SPID0_1;
    while (WriteBytes >= 4) {
      Data32 = *(UINT32 *)WriteBuffer;
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "0x%08x <-- 0x%08x\n", Controller.U32, Data32));
      }
      *Controller.Reg32 = Data32;
      *Controller.Reg32;
      WriteBuffer += 4;
      WriteBytes -= 4;
      Controller.U32 += 4;
    }
    while (WriteBytes--) {
      Data = *WriteBuffer++;
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "0x%08x <-- 0x%02x\n", Controller.U32, Data));
      }
      *Controller.Reg8 = Data;
      *Controller.Reg8;
      Controller.U32 += 1;
    }

    //
    // Start the write operation
    //
    Controller.U32 = BaseAddress + SPICTL;
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "0x%08x <-- 0x%04x\n", Controller.U32, Control));
    }
    *Controller.Reg16 = Control;
    *Controller.Reg16;

    //
    // Wait for the operation to complete
    //
    Controller.U32 = BaseAddress + SPISTS;
    do {
      SpiStatus = *Controller.Reg16;
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR, "0x%08x --> 0x%04x\n", Controller.U32, SpiStatus));
      }
    } while ((SpiStatus & SPISTS_CIP) != 0);
    if ((SpiStatus & SPISTS_BA) != 0) {
      if (BusTransaction->DebugTransaction) {
        DEBUG ((EFI_D_ERROR,
                "ERROR - SpiHc blocked access, transaction failed!\n"));
      }
      Status = EFI_ACCESS_DENIED;
    }
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "0x%08x <-- 0x%04x\n", Controller.U32,
              SPISTS_BA | SPISTS_CD));
    }
    *Controller.Reg16 = SPISTS_BA | SPISTS_CD;
    *Controller.Reg16;

    //
    // Restore the BIOS control
    //
    if (BusTransaction->DebugTransaction) {
      DEBUG ((EFI_D_ERROR, "BIOS Control <-- 0x%08x\n", BiosControlSaved));
    }
    PciWrite32 (Address, BiosControlSaved);
    break;
  }

  //
  // Return the transaction status
  //
  return Status;
}

/**
  Set the erase block opcode.

  This routine must be called at or below TPL_NOTIFY.

  The menu table contains SPI transaction opcodes which are accessible after
  the legacy SPI flash controller's configuration is locked.  The board layer
  specifies the erase block size for the SPI NOR flash part.  The SPI NOR flash
  peripheral driver selects the erase block opcode which matches the erase
  block size and uses this API to load the opcode into the opcode menu table.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  EraseBlockOpcode  Erase block opcode to be placed into the opcode
                                menu table.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The opcode menu table was updated
  @retval EFI_ACCESS_DENIED     The SPI controller is locked

**/
EFI_STATUS
EFIAPI SpiHcEraseBlockOpcode (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT8 EraseBlockOpcode
  )
{
  SPI_HC *SpiHc;

  //
  // Validate the input parameters
  //
  SpiHc = SPI_HC_CONTEXT_FROM_LEGACY_PROTOCOL(This);
  if (SpiHc->ControllerLocked) {
    DEBUG ((EFI_D_ERROR, "ERROR - SPI controller is locked!\n"));
    return EFI_ACCESS_DENIED;
  }

  //
  // Update the opcode menu table with the erase block opcode
  //
  SpiHcOpcode (SpiHc,
               OPCODE_ERASE_BLOCK_INDEX,
               OPCODE_ERASE_BLOCK_TYPE,
               EraseBlockOpcode);
  return EFI_SUCCESS;
}

/**
  Set the write status prefix opcode.

  This routine must be called at or below TPL_NOTIFY.

  The prefix table contains SPI transaction write prefix opcodes which are
  accessible after the legacy SPI flash controller's configuration is locked.
  The board layer specifies the write status prefix opcode for the SPI NOR
  flash part.  The SPI NOR flash peripheral driver uses this API to load the
  opcode into the prefix table.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  WriteStatusPrefix Prefix opcode for the write status command.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The prefix table was updated
  @retval EFI_ACCESS_DENIED     The SPI controller is locked

**/
EFI_STATUS
EFIAPI
SpiHcWriteStatusPrefix (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT8 WriteStatusPrefix
  )
{
  SPI_HC *SpiHc;

  //
  // Validate the input parameters
  //
  SpiHc = SPI_HC_CONTEXT_FROM_LEGACY_PROTOCOL(This);
  if (SpiHc->ControllerLocked) {
    DEBUG ((EFI_D_ERROR, "ERROR - SPI controller is locked!\n"));
    return EFI_ACCESS_DENIED;
  }

  //
  // Update the prefix table with the write status prefix
  //
  SpiHcPrefix (SpiHc, PREFIX_STATUS_WRITE_INDEX, WriteStatusPrefix);
  return EFI_SUCCESS;
}

/**
  Set the BIOS base address.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS base address works with the protect range registers to protect
  portions of the SPI NOR flash from erase and write operations.  The BIOS
  calls this API prior to passing control to the OS loader.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  BiosBaseAddress   The BIOS base address.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The BIOS base address was properly set
  @retval EFI_ACCESS_DENIED     The SPI controller is locked
  @retval EFI_INVALID_PARAMETER The BIOS base address is greater than
                                This->MaximumOffset
  @retval EFI_UNSUPPORTED       The BIOS base address was already set

**/
EFI_STATUS
EFIAPI
SpiHcBiosBaseAddress (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT32 BiosBaseAddress
  )
{
  union {
    volatile UINT32 *Reg32;
    UINT32 U32;
  } Controller;
  SPI_HC *SpiHc;

  //
  // Validate the input parameters
  //
  SpiHc = SPI_HC_CONTEXT_FROM_LEGACY_PROTOCOL(This);
  if (SpiHc->ControllerLocked) {
    DEBUG ((EFI_D_ERROR, "ERROR - SPI controller is locked!\n"));
    return EFI_ACCESS_DENIED;
  }
  if (BiosBaseAddress > SpiHc->MaximumOffset) {
    DEBUG ((EFI_D_ERROR, "ERROR - BiosBaseAddress > 0x%08x!\n",
            SpiHc->MaximumOffset));
    return EFI_INVALID_PARAMETER;
  }
  if (SpiHc->BiosBaseAddress != 0xffffffff) {
    DEBUG ((EFI_D_ERROR, "ERROR - BIOS base address is already set!\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Set the BIOS base address
  //
  DEBUG ((EFI_D_INFO, "Setting BiosBaseAddress: 0x%08x\n", BiosBaseAddress));
  SpiHc->BiosBaseAddress = BiosBaseAddress & BBAR_BOSF;
  Controller.U32 = SpiHc->BaseAddress + BBAR;
  *Controller.Reg32 = SpiHc->BiosBaseAddress;
  return EFI_SUCCESS;
}

/**
  Clear the SPI protect range registers.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS uses this routine to set an initial condition on the SPI protect
  range registers.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The register was successfully updated
  @retval EFI_ACCESS_DENIED     The SPI controller is locked

**/
EFI_STATUS
EFIAPI
SpiHcClearSpiProtect (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This
  )
{
  union {
    volatile UINT32 *Reg32;
    UINT32 U32;
  } Controller;
  UINT32 Index;
  SPI_HC *SpiHc;

  //
  // Attempt to clear the registers
  //
  SpiHc = SPI_HC_CONTEXT_FROM_LEGACY_PROTOCOL(This);
  for (Index = 0; Index < SpiHc->RangeRegisterCount; Index++) {
    Controller.U32 = SpiHc->BaseAddress + PBR0 + (Index * 4);
    *Controller.Reg32 = 0;
  }

  //
  // Verify that the registers are clear
  //
  for (Index = 0; Index < SpiHc->RangeRegisterCount; Index++) {
    Controller.U32 = SpiHc->BaseAddress + PBR0 + (Index * 4);
    if (*Controller.Reg32 != 0) {
      return EFI_ACCESS_DENIED;
    }
  }
  return EFI_SUCCESS;
}

/**
  Determine if the SPI range is protected.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS uses this routine to verify a range in the SPI is protected.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  BiosAddress       Address within a 4 KiB block to start
                                protecting.
  @param[in]  BlocksToProtect   The number of 4 KiB blocks to protect.

  @returns  TRUE if the range is protected and FALSE if it is not protected

**/
BOOLEAN
EFIAPI
SpiHcIsRangeProtected (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT32 BiosAddress,
  IN UINT32 BlocksToProtect
  )
{
  UINT32 BiosStart;
  UINT32 BiosEnd;
  union {
    volatile UINT32 *Reg32;
    UINT32 U32;
  } Controller;
  UINT32 Data;
  UINT32 Index;
  UINT32 Offset;
  SPI_HC *SpiHc;

  //
  // Validate the input parameters
  //
  SpiHc = SPI_HC_CONTEXT_FROM_LEGACY_PROTOCOL(This);
  if (SpiHc->BiosBaseAddress != 0xffffffff) {
    DEBUG ((EFI_D_ERROR, "ERROR - BIOS base address is already set!\n"));
    return FALSE;
  }
  if (BiosAddress < SpiHc->BiosBaseAddress) {
    DEBUG ((EFI_D_ERROR, "ERROR - BiosAddress < 0x%08x!\n",
            SpiHc->BiosBaseAddress));
    return FALSE;
  }
  BiosAddress &= 4096 - 1;
  Offset = BiosAddress - SpiHc->BiosBaseAddress;
  if ((Offset + (BlocksToProtect << 12)) > SpiHc->MaximumRangeBytes) {
    DEBUG ((EFI_D_ERROR,
         "ERROR - BiosAddress - 0x%08x + (BlocksToProtect * 4096) > 0x%08x!\n",
         SpiHc->BiosBaseAddress, SpiHc->MaximumRangeBytes));
    return FALSE;
  }

  //
  // Walk the protection registers
  //
  BiosStart = BiosAddress >> 12;
  BiosEnd = (BiosAddress + (BlocksToProtect << 12) - 1) & ~PBR_PRB;
  for (Index = 0; Index < SpiHc->RangeRegisterCount; Index++) {
    Controller.U32 = SpiHc->BaseAddress + PBR0 + (Index * 4);
    Data = *Controller.Reg32;

    //
    // Determine if the register is enabled
    //
    if ((Data & PBR_WPE) != 0) {
      //
      // The register is enabled, determine if it covers the specified range
      //
      if ((BiosStart >= (Data & PBR_PRB)) && (BiosEnd <= (Data & PBR_PRL))) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**
  Set the next protect range register.

  This routine must be called at or below TPL_NOTIFY.

  The BIOS sets the protect range register to prevent write and erase
  operations to a portion of the SPI NOR flash device.

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.
  @param[in]  BiosAddress       Address within a 4 KiB block to start
                                protecting.
  @param[in]  BlocksToProtect   The number of 4 KiB blocks to protect.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The register was successfully updated
  @retval EFI_ACCESS_DENIED     The SPI controller is locked
  @retval EFI_INVALID_PARAMETER BiosAddress < This->BiosBaseAddress
  @retval EFI_INVALID_PARAMETER BlocksToProtect * 4 KiB 
                                > This->MaximumRangeBytes
  @retval EFI_INVALID_PARAMETER BiosAddress - This->BiosBaseAddress
                                + (BlocksToProtect * 4 KiB)
                                > This->MaximumRangeBytes
  @retval EFI_OUT_OF_RESOURCES  No protect range register available
  @retval EFI_UNSUPPORTED       Call This->SetBaseAddress because the BIOS base
                                address is not set

**/
EFI_STATUS
EFIAPI
SpiHcProtectNextRange (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This,
  IN UINT32 BiosAddress,
  IN UINT32 BlocksToProtect
  )
{
  union {
    volatile UINT32 *Reg32;
    UINT32 U32;
  } Controller;
  UINT32 Index;
  UINT32 Offset;
  SPI_HC *SpiHc;
  UINT32 Value;

  //
  // Validate the input parameters
  //
  SpiHc = SPI_HC_CONTEXT_FROM_LEGACY_PROTOCOL(This);
  if (SpiHc->ControllerLocked) {
    DEBUG ((EFI_D_ERROR, "ERROR - SPI controller is locked!\n"));
    return EFI_ACCESS_DENIED;
  }
  if (SpiHc->BiosBaseAddress != 0xffffffff) {
    DEBUG ((EFI_D_ERROR, "ERROR - BIOS base address is already set!\n"));
    return EFI_UNSUPPORTED;
  }
  if (BiosAddress < SpiHc->BiosBaseAddress) {
    DEBUG ((EFI_D_ERROR, "ERROR - BiosAddress < 0x%08x!\n",
            SpiHc->BiosBaseAddress));
    return EFI_INVALID_PARAMETER;
  }
  BiosAddress &= 4096 - 1;
  Offset = BiosAddress - SpiHc->BiosBaseAddress;
  if ((Offset + (BlocksToProtect << 12)) > SpiHc->MaximumRangeBytes) {
    DEBUG ((EFI_D_ERROR,
         "ERROR - BiosAddress - 0x%08x + (BlocksToProtect * 4096) > 0x%08x!\n",
         SpiHc->BiosBaseAddress, SpiHc->MaximumRangeBytes));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set the BIOS range protection
  //
  Index = 0;
  while (1) {
    if (Index >= SpiHc->RangeRegisterCount) {
      DEBUG ((EFI_D_ERROR, "ERROR - Index >= %d!\n",
              SpiHc->RangeRegisterCount));
      return EFI_OUT_OF_RESOURCES;
    }
    Controller.U32 = SpiHc->BaseAddress + PBR0 + (Index * 4);
    Value = *Controller.Reg32;
    if ((Value & PBR_WPE) != 0) {
      Index += 1;
      continue;
    }

    DEBUG ((EFI_D_INFO, "%d: Protecting BIOS flash 0x%08x - 0x%08x\n",
            Index, BiosAddress, BiosAddress + (BlocksToProtect << 12) -1));
    Value = PBR_WPE
          | ((BiosAddress + (BlocksToProtect << 12) - 1) & PBR_PRL)
          | (BiosAddress >> PBR_PRB_SHIFT);
    *Controller.Reg32 = Value;
    break;
  }
  return EFI_SUCCESS;
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

  @param[in]  This              Pointer to an
                                EFI_LEGACY_SPI_CONTROLLER_PROTOCOL structure.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI controller was successfully locked
  @retval EFI_ALREADY_STARTED   The SPI controller was already locked
**/
EFI_STATUS
EFIAPI
SpiHcLockController (
  IN CONST EFI_LEGACY_SPI_CONTROLLER_PROTOCOL *This
  )
{
  union {
    volatile UINT16 *Reg16;
    UINT32 U32;
  } Controller;
  SPI_HC *SpiHc;

  //
  // Validate the input parameters
  //
  SpiHc = SPI_HC_CONTEXT_FROM_LEGACY_PROTOCOL(This);
  if (SpiHc->ControllerLocked) {
    DEBUG ((EFI_D_ERROR, "SPI controller is already locked!\n"));
    return EFI_ALREADY_STARTED;
  }

  //
  // Lock the SPI controller
  //
  DEBUG ((EFI_D_INFO, "Locking the SPI controller\n"));
  Controller.U32 = SpiHc->BaseAddress + SPISTS;
  *Controller.Reg16 = SPISTS_CLD;
  return EFI_SUCCESS;
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
SpiHcShutdown (
  IN SPI_HC *SpiHc
  )
{
  EFI_STATUS Status;

  //
  // Determine if the job is already done
  //
  if ((SpiHc != NULL) && (SpiHc->ControllerHandle != NULL)) {
    //
    // Remove the SPI host controller protocol
    //
    Status = gBS->UninstallProtocolInterface (
                     SpiHc->ControllerHandle,
                     SpiHc->SpiHcGuid,
                     &SpiHc->SpiHcProtocol
                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - SpiHc failed to remove SPI HC protocol!\n"));
      ASSERT_EFI_ERROR(Status);
    }

    //
    // Free the data structure
    //
    FreePool (SpiHc);
  }
}

/**
  Set up the necessary pieces to start SPI host controller for this device.

  This routine must be called at or below TPL_NOTIFY.

  Initialize the context data structure to support SPI host controller.  Gain
  access to the PCI root bridge IO protocol.  Upon successful completion,
  install the SPI host controller protocol.

  @param[in]  SpiHcPtr          A pointer to return the SPI_HC instance.
  @param[in]  SpiHcGuid         A pointer to the GUID to register for the host
                                controller.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The SPI host controller was started successfully
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
**/
EFI_STATUS
EFIAPI
SpiHcInitialize (
  SPI_HC **SpiHcPtr,
  EFI_GUID *SpiHcGuid
  )
{
  UINTN Address;
  SPI_HC *SpiHc;
  EFI_STATUS Status;

  //
  // Allocate the SPI host controller data structure
  //
  SpiHc = AllocateZeroPool (sizeof (SPI_HC));
  *SpiHcPtr = SpiHc;
  if (SpiHc == NULL) {
    DEBUG ((EFI_D_ERROR, "ERROR - Failed to allocate SPI_HC!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set the default values for the legacy support
  //
  SpiHc->BiosBaseAddress = 0xffffffff;
  SpiHc->MaximumRangeBytes = BIT24;
  SpiHc->RangeRegisterCount = 3;
  SpiHc->MaximumOffset = SpiHc->MaximumRangeBytes - 1;
  SpiHc->SpiHcGuid = SpiHcGuid;

  //
  // Get the SPI host controller base address
  //
  Address = PCI_LIB_ADDRESS (0, 31, 0, RCBA);
  SpiHc->BaseAddress = PciRead32(Address);
  DEBUG ((EFI_D_INFO, "Root Complex: %a\n",
          (SpiHc->BaseAddress & RCBA_EN) ? "Enabled" : "Disabled"));
  if ((SpiHc->BaseAddress & RCBA_EN) == 0) {
    DEBUG ((EFI_D_ERROR, "ERROR - SpiHc (root complex) is disabled!\n"));
    Status = EFI_DEVICE_ERROR;
    goto Failure;
  }
  SpiHc->BaseAddress &= RCBA_BA;
  DEBUG ((EFI_D_INFO, "0x%08x: SPI HC Base Address\n", SpiHc->BaseAddress));

  //
  // Initialize the prefix table
  //
  SpiHcPrefix (SpiHc, PREFIX_WRITE_ERASE_INDEX, PREFIX_WRITE_ERASE);
  SpiHcPrefix (SpiHc, PREFIX_STATUS_WRITE_INDEX, PREFIX_STATUS_WRITE);

  //
  // Initialize the opcode menu
  // Opcode menu slot 0 is used for random operations while the SPI controller
  // is unlocked.
  //
  SpiHcOpcode (SpiHc,
               OPCODE_READ_ID_INDEX,
               OPCODE_READ_ID_TYPE,
               OPCODE_READ_ID);
  SpiHcOpcode (SpiHc,
               OPCODE_READ_STATUS_INDEX,
               OPCODE_READ_STATUS_TYPE,
               OPCODE_READ_STATUS);
  SpiHcOpcode (SpiHc,
               OPCODE_READ_DATA_INDEX,
               OPCODE_READ_DATA_TYPE,
               OPCODE_READ_DATA);
  SpiHcOpcode (SpiHc,
               OPCODE_WRITE_DATA_INDEX,
               OPCODE_WRITE_DATA_TYPE,
               OPCODE_WRITE_DATA);
  SpiHcOpcode (SpiHc,
               OPCODE_WRITE_STATUS_INDEX,
               OPCODE_WRITE_STATUS_TYPE,
               OPCODE_WRITE_STATUS);
  SpiHcOpcode (SpiHc,
               OPCODE_ERASE_4KB_INDEX,
               OPCODE_ERASE_4KB_TYPE,
               OPCODE_ERASE_4KB);
  SpiHcOpcode (SpiHc,
               OPCODE_ERASE_BLOCK_INDEX,
               OPCODE_ERASE_BLOCK_TYPE,
               OPCODE_ERASE_32KB);

  //
  // Initialize the SPI host controller protocol
  //
  SpiHc->Signature = SPI_HC_SIGNATURE;
  SpiHc->SpiHcProtocol.Attributes = HC_SUPPORTS_WRITE_ONLY_OPERATIONS
                                  | HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS;
  SpiHc->SpiHcProtocol.FrameSizeSupportMask = SUPPORT_FRAME_SIZE_BITS (8);
  SpiHc->SpiHcProtocol.MaximumTransferBytes = 64;

  SpiHc->SpiHcProtocol.ChipSelect = SpiHcChipSelect;
  SpiHc->SpiHcProtocol.Clock = SpiHcClock;
  SpiHc->SpiHcProtocol.Transaction = SpiHcTransaction;

  //
  // Initialize the legacy SPI controller protocol
  //
  SpiHc->LegacySpiProtocol.MaximumOffset = SpiHc->MaximumOffset;
  SpiHc->LegacySpiProtocol.MaximumRangeBytes = SpiHc->MaximumRangeBytes;
  SpiHc->LegacySpiProtocol.RangeRegisterCount = SpiHc->RangeRegisterCount;

  SpiHc->LegacySpiProtocol.EraseBlockOpcode = SpiHcEraseBlockOpcode;
  SpiHc->LegacySpiProtocol.WriteStatusPrefix = SpiHcWriteStatusPrefix;
  SpiHc->LegacySpiProtocol.BiosBaseAddress = SpiHcBiosBaseAddress;
  SpiHc->LegacySpiProtocol.ClearSpiProtect = SpiHcClearSpiProtect;
  SpiHc->LegacySpiProtocol.IsRangeProtected = SpiHcIsRangeProtected;
  SpiHc->LegacySpiProtocol.ProtectNextRange = SpiHcProtectNextRange;
  SpiHc->LegacySpiProtocol.LockController = SpiHcLockController;

  return EFI_SUCCESS;

Failure:
  //
  // Release the SPI host controller resources
  //
  SpiHcShutdown (SpiHc);
  return Status;
}
