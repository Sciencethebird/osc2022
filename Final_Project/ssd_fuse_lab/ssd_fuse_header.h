/*
  FUSE-ioctl: ioctl support for FUSE
  Copyright (C) 2008       SUSE Linux Products GmbH
  Copyright (C) 2008       Tejun Heo <teheo@suse.de>
  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#define PHYSICAL_NAND_NUM (13)
#define LOGICAL_NAND_NUM (10)
#define NAND_SIZE_KB (50)

// L2P status
#define INVALID_PCA     (0xFFFFFFFF)
#define STALE_PCA       (0xFFFFFFF0)

// P2L status
#define INVALID_LBA     (0xFFFFFFFF)
#define STALE_LBA     (0xFFFFFFF0)

#define FREE_BLOCK     (0xFFFFFFFF)
#define OUT_OF_BLOCK     (0xFFFF)
#define FULL_PCA     (0xFFFFFFFE)
#define PAGE_PER_BLOCK     (10)
#define NAND_LOCATION  "/home/sciencethebird/Desktop/osc2022/Final_Project/ssd_fuse_lab/ssd"

#define TOTAL_LOGICAL_PAGE (PHYSICAL_NAND_NUM * PAGE_PER_BLOCK)

enum
{
    SSD_GET_LOGIC_SIZE    = _IOR('E', 0, size_t),
    SSD_GET_PHYSIC_SIZE   = _IOR('E', 1, size_t),
    SSD_GET_WA            = _IOR('E', 2, size_t),
};
