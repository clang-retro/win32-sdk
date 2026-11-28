/* scsi.h: the SCSI definitions of a SCSI miniport for Windows 95, 98 and
 * ME: the CDB layouts, the SCSI operation codes (SCSIOP_*), sense data and
 * inquiry data.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) clang-retro contributors.
 *
 * Written for clang-retro. No text of a Microsoft header is used. The
 * definitions are mingw-w64's <ddk/scsi.h> (public domain). This file
 * includes this directory's <srb.h> first, which has the Windows 9x
 * layouts and the same include guard as mingw-w64's, so that
 * <ddk/scsi.h> uses it. */
#ifndef MINIPORT9X_SCSI_H
#define MINIPORT9X_SCSI_H

#include <srb.h>
#include <ddk/scsi.h>

#endif
