/** @file

  This module implements a clock that uses the MAX9650 display controller.

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

EFI_GUID gMaximMax6950ProtocolGuid = MAXIM_MAX6950_PROTOCOL_GUID;

/**
  The entry point for the clock background application.

  @param[in] Event          Handle to the event being invoked.
  @param[in] Context        Pointer to the notification functions's context.
  
**/
VOID
EFIAPI
UpdateTime (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  UINT8 Display[5];
  UINTN Index;
  STATIC MAXIM_MAX6950_PROTOCOL *Max6950;
  EFI_STATUS Status;
  EFI_TIME Time;

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
  // Get the time
  //
  Status = gRT->GetTime (&Time, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Clock: Failed to get the current time!\n"));
    goto Done;
  }

  //
  // Convert the time into a displayable format
  //
  Index = 0;
  Display[Index++] = (Time.Hour >= 10) ? (Time.Hour / 10) + '0' : ' ';
  Display[Index++] = (Time.Hour % 10) | '0';
  if ((Time.Second & 1) != 0) {
    Display[Index++] = '.';
  }
  Display[Index++] = (Time.Minute / 10) | '0';
  Display[Index++] = (Time.Minute % 10) | '0';

  //
  // Display the time
  //
  Status = Max6950->DisplayString (Max6950, &Display[0], Index);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Clock: Failed to display the current time!\n"));
  }

Done:
  return;
}

/**
  The entry point for the clock background application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
ClockEntryPoint (
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
                    UpdateTime,
                    NULL,
                    &Timer
                    );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,
            "ERROR - Clock failed to initialize event, Status: %r\n", Status));
  } else {
    Status = gBS->SetTimer (
                    Timer,
                    TimerPeriodic,
                    1 * 1000 * 1000 * 10ULL
                    );
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR,
              "ERROR - Clock failed to start timer, Status: %r\n", Status));
    }
  }
  ASSERT_EFI_ERROR (Status);
  return Status;
}
