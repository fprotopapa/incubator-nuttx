/****************************************************************************
 * boards/arm/lpc43xx/bambino-200e/src/lpc43_appinit.c
 *
 *   Copyright (C) 2016, 2020 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Alan Carvalho de Assis acassis@gmail.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <syslog.h>
#include <errno.h>

#include <nuttx/board.h>

#include "chip.h"

#ifdef CONFIG_LPC43_SPIFI
#  include <nuttx/mtd/mtd.h>
#  include "lpc43_spifi.h"

#  ifdef CONFIG_SPFI_NXFFS
#    include <sys/mount.h>
#    include <nuttx/fs/nxffs.h>
#  endif
#endif

#ifdef CONFIG_LPC43_SDMMC
#  include <nuttx/sdio.h>
#  include <nuttx/mmcsd.h>
#  include "lpc43_sdmmc.h"
#endif

#include "bambino-200e.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_SPIFI_DEVNO
#  define CONFIG_SPIFI_DEVNO 0
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_spifi_initialize
 *
 * Description:
 *   Make the SPIFI (or part of it) into a block driver that can hold a
 *   file system.
 *
 ****************************************************************************/

#ifdef CONFIG_LPC43_SPIFI
static int nsh_spifi_initialize(void)
{
  FAR struct mtd_dev_s *mtd;
  int ret;

  /* Initialize the SPIFI interface and create the MTD driver instance */

  mtd = lpc43_spifi_initialize();
  if (!mtd)
    {
      ferr("ERROR: lpc43_spifi_initialize failed\n");
      return -ENODEV;
    }

#ifndef CONFIG_SPFI_NXFFS
  /* And finally, use the FTL layer to wrap the MTD driver as a block driver */

  ret = ftl_initialize(CONFIG_SPIFI_DEVNO, mtd);
  if (ret < 0)
    {
      ferr("ERROR: Initializing the FTL layer: %d\n", ret);
      return ret;
    }
#else
  /* Initialize to provide NXFFS on the MTD interface */

  ret = nxffs_initialize(mtd);
  if (ret < 0)
    {
      ferr("ERROR: NXFFS initialization failed: %d\n", ret);
      return ret;
    }

  /* Mount the file system at /mnt/spifi */

  ret = mount(NULL, "/mnt/spifi", "nxffs", 0, NULL);
  if (ret < 0)
    {
      ferr("ERROR: Failed to mount the NXFFS volume: %d\n", errno);
      return ret;
    }
#endif

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_app_initialize
 *
 * Description:
 *   Perform architecture specific initialization
 *
 * Input Parameters:
 *   arg - The boardctl() argument is passed to the board_app_initialize()
 *         implementation without modification.  The argument has no
 *         meaning to NuttX; the meaning of the argument is a contract
 *         between the board-specific initialization logic and the
 *         matching application logic.  The value could be such things as a
 *         mode enumeration value, a set of DIP switch switch settings, a
 *         pointer to configuration data read from a file or serial FLASH,
 *         or whatever you would like to do with it.  Every implementation
 *         should accept zero/NULL as a default configuration.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   any failure to indicate the nature of the failure.
 *
 ****************************************************************************/

int board_app_initialize(uintptr_t arg)
{
#ifdef HAVE_MMCSD
  struct sdio_dev_s *sdmmc;
#endif
  int ret = 0;

#ifdef CONFIG_LPC43_SPIFI
  /* Initialize the SPIFI block device */

  ret = nsh_spifi_initialize();
  if (ret != OK)
    {
      syslog(LOG_ERR, "ERROR: nsh_spifi_initialize() failed: %d\n", ret);
    }
#endif

#ifdef HAVE_MMCSD
  /* Get an instance of the SDIO interface */

  sdmmc = lpc43_sdmmc_initialize(0);
  if (!sdmmc)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize SD/MMC\n");
    }
  else
    {
      /* Bind the SDIO interface to the MMC/SD driver */

      ret = mmcsd_slotinitialize(MMCSD_MINOR, sdmmc);
      if (ret != OK)
        {
          syslog(LOG_ERR,
                 "ERROR: Failed to bind SDIO to the MMC/SD driver: %d\n",
                 ret);
        }
    }
#endif

#ifdef CONFIG_TIMER
  /* Registers the timers */

  lpc43_timerinitialize();
#endif

  return ret;
}
