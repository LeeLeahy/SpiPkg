/** @file

  This module implements the 16-bit transfer routines.

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

#include "QuarkSpiDxe.h"

/**
  Perform a full-duplex SPI transaction with the SPI peripheral using the SPI
  host controller.

  This routine is called at TPL_NOTIFY.

  @param[in]  BaseAddress       Address of the SPI host controller registers
  @param[in]  WriteBytes        Number of bytes to send to the SPI peripheral
  @param[in]  WriteBuffer       Pointer to the data to send to the SPI
                                peripheral
  @param[in]  ReadBytes         Number of bytes to receive from the SPI
                                peripheral
  @param[in]  ReadBuffer        Pointer to the receive data buffer
**/
VOID
EFIAPI
SpiHc16BitFullDuplexTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT16* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT16* ReadBuffer
  )
{
  union {
    volatile UINT32 *Reg;
    UINT32 U32;
  } Controller;
  UINT32 Sssr;

  //
  // Start the SPI transaction
  // Send the data to the SPI peripheral
  //
  WriteBytes /= 2;
  ReadBytes /= 2;
  while (WriteBytes) {
    //
    // Determine if space is available in the FIFO
    //
    Controller.U32 = BaseAddress + SSSR;
    Sssr = *Controller.Reg;
    if ((Sssr & SSSR_TNF) != 0) {
      //
      // Send the next data byte to the SPI peripheral
      //
      WriteBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg = *WriteBuffer++;
    }

    //
    // Save any receive data
    // Determine if a receive byte is in the FIFO
    //
    if ((Sssr & SSSR_RNE) != 0) {
      //
      // Place the receive data byte into the buffer
      //
      ReadBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *ReadBuffer++ = (UINT16)*Controller.Reg;
    }
  }

  //
  //  Finish receiving the data
  //
  while (ReadBytes) {
    //
    // Save any receive data
    // Determine if a receive byte is in the FIFO
    //
    Controller.U32 = BaseAddress + SSSR;
    Sssr = *Controller.Reg;
    if ((Sssr & SSSR_RNE) != 0) {
      //
      // Place the receive data byte into the buffer
      //
      ReadBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *ReadBuffer++ = (UINT16)*Controller.Reg;
    }
  }
}

/**
  Perform a write-only SPI transaction with the SPI peripheral using the SPI
  host controller.

  This routine is called at TPL_NOTIFY.

  @param[in]  BaseAddress       Address of the SPI host controller registers
  @param[in]  WriteBytes        Number of bytes to send to the SPI peripheral
  @param[in]  WriteBuffer       Pointer to the data to send to the SPI
                                peripheral
  @param[in]  ReadBytes         Number of bytes to receive from the SPI
                                peripheral
  @param[in]  ReadBuffer        Pointer to the receive data buffer
**/
VOID
EFIAPI
SpiHc16BitWriteOnlyTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT16* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT16* ReadBuffer
  )
{
  union {
    volatile UINT32 *Reg;
    UINT32 U32;
  } Controller;
  UINT32 Sssr;

  //
  // Start the SPI transaction
  // Send the data to the SPI peripheral
  //
  WriteBytes /= 2;
  ReadBytes /= 2;
  while (WriteBytes) {
    //
    // Determine if space is available in the FIFO
    //
    Controller.U32 = BaseAddress + SSSR;
    Sssr = *Controller.Reg;
    if ((Sssr & SSSR_TNF) != 0) {
      //
      // Send the next data byte to the SPI peripheral
      //
      WriteBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg = *WriteBuffer++;
    }

    //
    // Discard receive data
    // Determine if a receive byte is in the FIFO
    //
    if ((Sssr & SSSR_RNE) != 0) {
      //
      // Discard the receive data byte
      //
      ReadBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg;
    }
  }

  //
  //  Finish discarding the receiving the data
  //
  while (ReadBytes) {
    //
    // Save any receive data
    // Determine if a receive byte is in the FIFO
    //
    Controller.U32 = BaseAddress + SSSR;
    Sssr = *Controller.Reg;
    if ((Sssr & SSSR_RNE) != 0) {
      //
      // Place the receive data byte into the buffer
      //
      ReadBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg;
    }
  }
}

/**
  Perform a write-then-read SPI transaction with the SPI peripheral using the
  SPI host controller.

  This routine is called at TPL_NOTIFY.

  @param[in]  BaseAddress       Address of the SPI host controller registers
  @param[in]  WriteBytes        Number of bytes to send to the SPI peripheral
  @param[in]  WriteBuffer       Pointer to the data to send to the SPI
                                peripheral
  @param[in]  ReadBytes         Number of bytes to receive from the SPI
                                peripheral
  @param[in]  ReadBuffer        Pointer to the receive data buffer
**/
VOID
EFIAPI
SpiHc16BitWriteThenReadTransaction (
  IN UINT32 BaseAddress,
  IN UINTN WriteBytes,
  IN UINT16* WriteBuffer,
  IN UINTN ReadBytes,
  IN UINT16* ReadBuffer
  )
{
  union {
    volatile UINT32 *Reg;
    UINT32 U32;
  } Controller;
  UINTN DiscardBytes;
  UINT32 Sssr;
  UINTN ZeroBytes;

  //
  // Start the SPI transaction
  // Send the data to the SPI peripheral
  //
  WriteBytes /= 2;
  ReadBytes /= 2;
  DiscardBytes = WriteBytes;
  ZeroBytes = ReadBytes;
  while (WriteBytes) {
    //
    // Determine if space is available in the FIFO
    //
    Controller.U32 = BaseAddress + SSSR;
    Sssr = *Controller.Reg;
    if ((Sssr & SSSR_TNF) != 0) {
      //
      // Send the next data byte to the SPI peripheral
      //
      WriteBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg = *WriteBuffer++;
    }

    //
    // Discard the initial receive data
    // Determine if a receive byte is in the FIFO
    //
    if ((Sssr & SSSR_RNE) != 0) {
      //
      // Discard this receive data byte
      //
      DiscardBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg;
    }
  }

  //
  // Finish discarding the receive bytes
  //
  while (DiscardBytes) {
    //
    // Determine if space is available in the FIFO
    //
    Controller.U32 = BaseAddress + SSSR;
    Sssr = *Controller.Reg;
    if (ZeroBytes && ((Sssr & SSSR_TNF) != 0)) {
      //
      // Send zeros to the SPI peripheral
      //
      ZeroBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg = 0;
    }

    //
    // Determine if a receive byte is in the FIFO
    //
    if ((Sssr & SSSR_RNE) != 0) {
      //
      // Discard this receive data byte
      //
      DiscardBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg;
    }
  }

  //
  // Start receiving the data
  //
  while (ZeroBytes) {
    //
    // Determine if space is available in the FIFO
    //
    Controller.U32 = BaseAddress + SSSR;
    Sssr = *Controller.Reg;
    if ((Sssr & SSSR_TNF) != 0) {
      //
      // Send zeros to the SPI peripheral
      //
      ZeroBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *Controller.Reg = 0;
    }

    //
    // Determine if a receive byte is in the FIFO
    //
    if ((Sssr & SSSR_RNE) != 0) {
      //
      // Place the receive data into the receive buffer
      //
      ReadBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *ReadBuffer++ = (UINT16)*Controller.Reg;
    }
  }

  //
  //  Finish receiving the data
  //
  while (ReadBytes) {
    //
    // Save any receive data
    // Determine if a receive byte is in the FIFO
    //
    Controller.U32 = BaseAddress + SSSR;
    Sssr = *Controller.Reg;
    if ((Sssr & SSSR_RNE) != 0) {
      //
      // Place the receive data byte into the buffer
      //
      ReadBytes -= 1;
      Controller.U32 = BaseAddress + SSDR;
      *ReadBuffer++ = (UINT16)*Controller.Reg;
    }
  }
}
