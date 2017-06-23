/** @file
Implementation of helper routines for DXE environment.

Copyright (c) 2013 - 2016 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/LegacySpiFlash.h>
#include <Protocol/VariableLock.h>

#include <Guid/MemoryConfigData.h>
#include <Guid/QuarkVariableLock.h>

#include "CommonHeader.h"

#define FLASH_BLOCK_SIZE            SIZE_4KB

//
// Global variables.
//
EFI_LEGACY_SPI_FLASH_PROTOCOL *mPlatHelpSpiProtocolRef = NULL;

//
// Routines defined in other source modules of this component.
//

//
// Routines local to this component.
//

EFI_LEGACY_SPI_FLASH_PROTOCOL *
LocateSpiProtocol (
  IN  EFI_SMM_SYSTEM_TABLE2             *Smst
  )
{
  if (mPlatHelpSpiProtocolRef == NULL) {
DEBUG ((EFI_D_ERROR, "      Calling gBS->LocateProtocol (gEfiSpiProtocolGuid)\n"));
      gBS->LocateProtocol (
             &gEfiLegacySpiFlashProtocolGuid,
             NULL,
             (VOID **) &mPlatHelpSpiProtocolRef
             );
    ASSERT (mPlatHelpSpiProtocolRef != NULL);
  }
  return mPlatHelpSpiProtocolRef;
}

EFI_STATUS
WriteFirstFreeSpiProtect (
  IN CONST UINT32                         PchRootComplexBar,
  IN CONST UINT32                         DirectValue,
  IN CONST UINT32                         BaseAddress,
  IN CONST UINT32                         Length,
  OUT UINT32                              *OffsetPtr
  )
{
  EFI_LEGACY_SPI_FLASH_PROTOCOL *SpiProtocol;

  SpiProtocol = LocateSpiProtocol (NULL);
  ASSERT (SpiProtocol != NULL);

  return SpiProtocol->ProtectNextRange (SpiProtocol,
                         BaseAddress, 
                         BaseAddress + ALIGN_VALUE (Length,SIZE_4KB) - 1
                         );
}

/**
  Clear SPI Protect registers.

  @retval EFI_SUCCESS        SPI protect registers cleared.
  @retval EFI_ACCESS_DENIED  Unable to clear SPI protect registers.
**/

EFI_STATUS
EFIAPI
PlatformClearSpiProtect (
  VOID
  )
{
  EFI_LEGACY_SPI_FLASH_PROTOCOL *SpiProtocol;

  SpiProtocol = LocateSpiProtocol (NULL);
  ASSERT (SpiProtocol != NULL);

  return SpiProtocol->ClearSpiProtect (SpiProtocol);
}

/**
  Determine if an SPI address range is protected.

  @param  SpiBaseAddress  Base of SPI range.
  @param  Length          Length of SPI range.

  @retval TRUE       Range is protected.
  @retval FALSE      Range is not protected.
**/
BOOLEAN
EFIAPI
PlatformIsSpiRangeProtected (
  IN CONST UINT32                         SpiBaseAddress,
  IN CONST UINT32                         Length
  )
{
  EFI_LEGACY_SPI_FLASH_PROTOCOL *SpiProtocol;

  SpiProtocol = LocateSpiProtocol (NULL);
  ASSERT (SpiProtocol != NULL);

  return SpiProtocol->IsRangeProtected (SpiProtocol, SpiBaseAddress,
                                        (Length + BIT12 - 1) >> 12);
}

//
// Routines exported by this source module.
//

/**
  Find pointer to RAW data in Firmware volume file.

  @param   FvNameGuid       Firmware volume to search. If == NULL search all.
  @param   FileNameGuid     Firmware volume file to search for.
  @param   SectionData      Pointer to RAW data section of found file.
  @param   SectionDataSize  Pointer to UNITN to get size of RAW data.

  @retval  EFI_SUCCESS            Raw Data found.
  @retval  EFI_INVALID_PARAMETER  FileNameGuid == NULL.
  @retval  EFI_NOT_FOUND          Firmware volume file not found.
  @retval  EFI_UNSUPPORTED        Unsupported in current enviroment (PEI or DXE).

**/
EFI_STATUS
EFIAPI
PlatformFindFvFileRawDataSection (
  IN CONST EFI_GUID                 *FvNameGuid OPTIONAL,
  IN CONST EFI_GUID                 *FileNameGuid,
  OUT VOID                          **SectionData,
  OUT UINTN                         *SectionDataSize
  )
{
  if (FileNameGuid == NULL || SectionData == NULL || SectionDataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (FvNameGuid != NULL) {
    return EFI_UNSUPPORTED;  // Searching in specific FV unsupported in DXE.
  }

  return GetSectionFromAnyFv (FileNameGuid, EFI_SECTION_RAW, 0, SectionData, SectionDataSize);
}

/**
  Find free spi protect register and write to it to protect a flash region.

  @param   DirectValue      Value to directly write to register.
                            if DirectValue == 0 the use Base & Length below.
  @param   BaseAddress      Base address of region in Flash Memory Map.
  @param   Length           Length of region to protect.

  @retval  EFI_SUCCESS      Free spi protect register found & written.
  @retval  EFI_NOT_FOUND    Free Spi protect register not found.
  @retval  EFI_DEVICE_ERROR Unable to write to spi protect register.
**/
EFI_STATUS
EFIAPI
PlatformWriteFirstFreeSpiProtect (
  IN CONST UINT32                         DirectValue,
  IN CONST UINT32                         BaseAddress,
  IN CONST UINT32                         Length
  )
{
  UINT32                            FreeOffset;
  UINT32                            PchRootComplexBar;
  EFI_STATUS                        Status;

  PchRootComplexBar = QNC_RCRB_BASE;

  Status = WriteFirstFreeSpiProtect (
             PchRootComplexBar,
             DirectValue,
             BaseAddress,
             Length,
             &FreeOffset
             );

  return Status;
}

/**
  Lock legacy SPI static configuration information.

  Function will assert if unable to lock config.

**/
VOID
EFIAPI
PlatformFlashLockConfig (
  VOID
  )
{
  EFI_STATUS        Status;
  EFI_LEGACY_SPI_FLASH_PROTOCOL  *SpiProtocol;

  //
  // Enable lock of legacy SPI static configuration information.
  //

DEBUG ((EFI_D_ERROR, "    Calling SpiProtocol->Lock\n"));
  SpiProtocol = LocateSpiProtocol (NULL);  // This routine will not be called in SMM.
  ASSERT_EFI_ERROR (SpiProtocol != NULL);
  if (SpiProtocol != NULL) {
    Status = SpiProtocol->LockController (SpiProtocol);

    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "Platform: Spi Config Locked Down\n"));
    } else if (Status == EFI_ACCESS_DENIED) {
      DEBUG ((EFI_D_INFO, "Platform: Spi Config already locked down\n"));
    } else {
      ASSERT_EFI_ERROR (Status);
    }
  }
}

/**
  Platform Variable Lock.

  @retval EFI_SUCCESS           Platform Variable Lock successful.
  @retval EFI_NOT_FOUND         No protocol instances were found that match Protocol and
                                Registration.

**/
VOID
EFIAPI
PlatformVariableLock (
  )
{
  EFI_STATUS                        Status;
  EDKII_VARIABLE_LOCK_PROTOCOL      *VariableLockProtocol;

  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **)&VariableLockProtocol);
  ASSERT_EFI_ERROR (Status);

  Status = VariableLockProtocol->RequestToLock (
                                   VariableLockProtocol,
                                   QUARK_VARIABLE_LOCK_NAME,
                                   &gQuarkVariableLockGuid
                                   );
  ASSERT_EFI_ERROR (Status);

  // Memory Config Data shouldn't be writable when Quark Variable Lock is enabled.
  Status = VariableLockProtocol->RequestToLock (
                                   VariableLockProtocol,
                                   EFI_MEMORY_CONFIG_DATA_NAME,
                                   &gEfiMemoryConfigDataGuid
                                   );
  ASSERT_EFI_ERROR (Status);
}

/**
  Lock regions and config of SPI flash given the policy for this platform.

  Function will assert if unable to lock regions or config.

  @param   PreBootPolicy    If TRUE do Pre Boot Flash Lock Policy.

**/
VOID
EFIAPI
PlatformFlashLockPolicy (
  IN CONST BOOLEAN                        PreBootPolicy
  )
{
  EFI_STATUS                        Status;
  UINT64                            CpuAddressNvStorage;
  UINT64                            CpuAddressFlashDevice;
  UINT64                            SpiAddress;
  EFI_BOOT_MODE                     BootMode;
  UINTN                             SpiFlashDeviceSize;

DEBUG ((EFI_D_ERROR, "--------------------------------------------------\n"));
DEBUG ((EFI_D_ERROR, "PlatformFlashLockPolicy called\n"));
  BootMode = GetBootModeHob ();

  SpiFlashDeviceSize = (UINTN) PcdGet32 (PcdSpiFlashDeviceSize);
  CpuAddressFlashDevice = SIZE_4GB - SpiFlashDeviceSize;
  DEBUG (
      (EFI_D_INFO,
      "Platform:FlashDeviceSize = 0x%08x Bytes\n",
      SpiFlashDeviceSize)
      );

  //
  // If not in update or recovery mode, lock stuff down
  //
  if ((BootMode != BOOT_IN_RECOVERY_MODE) && (BootMode != BOOT_ON_FLASH_UPDATE)) {

    //
    // Lock regions
    //
    CpuAddressNvStorage = (UINT64) PcdGet32 (PcdFlashNvStorageVariableBase);

    //
    // Lock from start of flash device up to Smi writable flash storage areas.
    //
    SpiAddress = 0;
DEBUG ((EFI_D_ERROR, "  Calling PlatformIsSpiRangeProtected\n"));
    if (!PlatformIsSpiRangeProtected ((UINT32) SpiAddress, (UINT32) (CpuAddressNvStorage - CpuAddressFlashDevice))) {
      DEBUG (
        (EFI_D_INFO,
        "Platform: Protect Region Base:Len 0x%08x:0x%08x\n",
        (UINTN) SpiAddress, (UINTN)(CpuAddressNvStorage - CpuAddressFlashDevice))
        );
DEBUG ((EFI_D_ERROR, "  Calling PlatformWriteFirstFreeSpiProtect\n"));
      Status = PlatformWriteFirstFreeSpiProtect (
                  0,
                  (UINT32) SpiAddress,
                  (UINT32) (CpuAddressNvStorage - CpuAddressFlashDevice)
                  );

      ASSERT_EFI_ERROR (Status);
    }
    //
    // Move Spi Address to after Smi writable flash storage areas.
    //
    SpiAddress = CpuAddressNvStorage - CpuAddressFlashDevice;
    SpiAddress += ((UINT64) PcdGet32 (PcdFlashNvStorageVariableSize));

    //
    // Lock from end of OEM area to end of flash part.
    //
DEBUG ((EFI_D_ERROR, "  Calling PlatformIsSpiRangeProtected\n"));
    if (!PlatformIsSpiRangeProtected ((UINT32) SpiAddress, SpiFlashDeviceSize - ((UINT32) SpiAddress))) {
      DEBUG (
        (EFI_D_INFO,
        "Platform: Protect Region Base:Len 0x%08x:0x%08x\n",
        (UINTN) SpiAddress,
        (UINTN) (SpiFlashDeviceSize - ((UINT32) SpiAddress)))
        );
      ASSERT (SpiAddress < ((UINT64) SpiFlashDeviceSize));
DEBUG ((EFI_D_ERROR, "  Calling PlatformWriteFirstFreeSpiProtect\n"));
      Status = PlatformWriteFirstFreeSpiProtect (
                  0,
                  (UINT32) SpiAddress,
                  SpiFlashDeviceSize - ((UINT32) SpiAddress)
                  );

      ASSERT_EFI_ERROR (Status);
    }
  }

  //
  // Always Lock flash config registers if about to boot a boot option
  // else lock depending on boot mode.
  //
  if (PreBootPolicy || (BootMode != BOOT_ON_FLASH_UPDATE)) {
DEBUG ((EFI_D_ERROR, "  Calling PlatformFlashLockConfig\n"));
    PlatformFlashLockConfig ();
  }

  //
  // Enable Quark Variable lock if PreBootPolicy.
  //
  if (PreBootPolicy) {
    PlatformVariableLock ();
  }
DEBUG ((EFI_D_ERROR, "--------------------------------------------------\n"));
}

/** Check if System booted with recovery Boot Stage1 image.

  @retval  TRUE    If system booted with recovery Boot Stage1 image.
  @retval  FALSE   If system booted with normal stage1 image.

**/
BOOLEAN
EFIAPI
PlatformIsBootWithRecoveryStage1 (
  VOID
  )
{
  ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
  return FALSE;
}

