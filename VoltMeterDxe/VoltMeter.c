/** @file

  This module implements a voltmeter that uses the MAX9650 display controller.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Maxim/Protocol/Max6950.h>
#include <Protocol/SpiConfiguration.h>
#include <TexasInstruments/Protocol/ADC108S102.h>

EFI_GUID gMaximMax6950ProtocolGuid = MAXIM_MAX6950_PROTOCOL_GUID;
EFI_GUID gTexasInstrumentsAdc108s102ProtocolGuid =
           TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL_GUID;

UINT8
GetDigit (
  UINT16 *AdcValue,
  UINT16 Divisor,
  BOOLEAN *SuppressZero
  )
{
  UINTN Digit;

  //
  // Compute the digit value
  //
  Digit = *AdcValue / Divisor;
  *AdcValue -= Digit * Divisor;
  Digit += '0';
  if (Digit != '0') {
    *SuppressZero = FALSE;
  } else {
    if (*SuppressZero) {
      Digit = ' ';
    }
  }
  return Digit;
}

/**
  Update the value on the voltmeter

  @param[in] Event          Handle to the event being invoked.
  @param[in] Context        Pointer to the notification functions's context.
  
**/
VOID
EFIAPI
UpdateValue (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  STATIC TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL *Adc108s102;
  CONST ADC108S102_CONFIGURATION_DATA *Adc108s102Config;
  UINT16 AdcValue;
  UINT8 Display[6];
  UINTN Index;
  STATIC MAXIM_MAX6950_PROTOCOL *Max6950;
  UINT16 Millivolts;
  EFI_STATUS Status;
  BOOLEAN SuppressZero;

  //
  // Connect to the MAX6950 driver
  //
  if (Max6950 == NULL) {
    Status = gBS->LocateProtocol (
                    &gMaximMax6950ProtocolGuid,
                    NULL,
                    (VOID **)&Max6950
                    );
    if (EFI_ERROR (Status)) {
      Max6950 = NULL;
      goto Done;
    }
  }

  //
  // Connect to the MAX6950 driver
  //
  if (Adc108s102 == NULL) {
    Status = gBS->LocateProtocol (
                    &gTexasInstrumentsAdc108s102ProtocolGuid,
                    NULL,
                    (VOID **)&Adc108s102
                    );
    if (EFI_ERROR (Status)) {
      Adc108s102 = NULL;
      goto Done;
    }
  }

  //
  // Read ADC channel 0
  //
  Status = Adc108s102->ReadChannel(Adc108s102, 0, &AdcValue);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "VoltMeter: Failed to get the ADC value!\n"));
    goto Done;
  }

  //
  // Convert the ADC value into a displayable format
  //
  ASSERT (Adc108s102->SpiPeripheral != NULL);
  Adc108s102Config = Adc108s102->SpiPeripheral->ConfigurationData;
  Index = 0;
  if (Adc108s102Config == NULL) {
    SuppressZero = TRUE;
    Display[Index++] = GetDigit(&AdcValue, 1000, &SuppressZero);
    Display[Index++] = GetDigit(&AdcValue, 100, &SuppressZero);
    Display[Index++] = GetDigit(&AdcValue, 10, &SuppressZero);
    SuppressZero = FALSE;
    Display[Index++] = GetDigit(&AdcValue, 1, &SuppressZero);
  } else {
    Millivolts = (UINT16)((AdcValue * Adc108s102Config->ReferenceVoltage)
                 >> 10);
    SuppressZero = FALSE;
    Display[Index++] = GetDigit(&Millivolts, 1000, &SuppressZero);
    Display[Index++] = '.';
    Display[Index++] = GetDigit(&Millivolts, 100, &SuppressZero);
    Display[Index++] = GetDigit(&Millivolts, 10, &SuppressZero);
    Display[Index++] = GetDigit(&Millivolts, 1, &SuppressZero);
  }

  //
  // Display the ADC value
  //
  Status = Max6950->DisplayString (Max6950, &Display[0], Index);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "VoltMeter: Failed to display the ADC value!\n"));
  }

Done:
  return;
}

/**
  The entry point for the voltmeter background application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
VoltMeterEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_EVENT Timer;
  EFI_STATUS Status;

  //
  // Create the timer event
  //
  Status = gBS->CreateEvent(
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    UpdateValue,
                    NULL,
                    &Timer
                    );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - VoltMeter failed to initialize event, Status: %r\n",
            Status));
  } else {
    Status = gBS->SetTimer (
                    Timer,
                    TimerPeriodic,
                    1 * 1000 * 1000 * 10ULL
                    );
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - VoltMeter failed to start timer, Status: %r\n", Status));
    }
  }
  ASSERT_EFI_ERROR (Status);
  return Status;
}
