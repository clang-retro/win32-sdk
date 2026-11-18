/* ior.h: IOS's I/O request (IOR) and its IOR_* functions
   and flags.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_IOR_H
#define VXD_IOR_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define IOR_READ 0x00
#define IOR_WRITE 0x01
#define IOR_VERIFY 0x02
#define IOR_CANCEL 0x03
#define IOR_WRITEV 0x04
#define IOR_MEDIA_CHECK 0x05
#define IOR_MEDIA_CHECK_RESET 0x06
#define IOR_LOAD_MEDIA 0x07
#define IOR_EJECT_MEDIA 0x08
#define IOR_LOCK_MEDIA 0x09
#define IOR_UNLOCK_MEDIA 0x0A
#define IOR_REQUEST_SENSE 0x0B
#define IOR_COMPUTE_GEOM 0x0C
#define IOR_GEN_IOCTL 0x0D
#define IOR_FORMAT 0x0E
#define IOR_SCSI_PASS_THROUGH 0x0F
#define IOR_CLEAR_QUEUE 0x10
#define IOR_DOS_RESET 0x11
#define IOR_SCSI_REQUEST 0x12
#define IOR_SET_WRITE_STATUS 0x22
#define IOR_RESTART_QUEUE 0x23
#define IOR_ABORT_QUEUE 0x24
#define IOR_SPIN_DOWN 0x25
#define IOR_SPIN_UP 0x26
#define IOR_FLUSH_DRIVE 0x27
#define IOR_FLUSH_DRIVE_AND_DISCARD 0x28
#define IOR_FSD_EXT 0x29
#define IOR_FLOPTICAL_MODE_SENSE 0x2A
#if VXD_WINVER >= 0x040A
#define IOR_IDE_PASS_THROUGH 0x2B
#define IOR_FLOPTICAL_INQUIRY 0x2C
#define IOR_ENUMERATE_DEVICES 0x2D
#define IOR_ASYNCHRONOUS_DRIVE_SPINUP 0x2E
#endif
#define IORS_SUCCESS 0x00
#define IORS_SUCCESS_WITH_RETRY 0x01
#define IORS_SUCCESS_WITH_ECC 0x02
#define IORS_INVALID_SECTOR 0x10
#define IORS_CANCELED 0x11
#define IORS_CMD_IN_PROGRESS 0x12
#define IORS_INVALID_CMD_PTR 0x13
#define IORS_MEDIA_ERROR 0x14
#define IORS_DEVICE_ERROR 0x15
#define IORS_INVALID_COMMAND 0x16
#define IORS_NO_DEVICE 0x17
#define IORS_NO_MEDIA 0x18
#define IORS_UNCERTAIN_MEDIA 0x19
#define IORS_UNREC_ERROR 0x1A
#define IORS_HW_FAILURE 0x1B
#define IORS_UNFORMATTED_MEDIA 0x1C
#define IORS_MEMORY_ERROR 0x1D
#define IORS_TIME_OUT 0x1E
#define IORS_WRITE_PROTECT 0x1F
#define IORS_NOT_READY 0x20
#define IORS_BUSY 0x21
#define IORS_VOL_NOT_LOCKED 0x22
#define IORS_VOL_LOCKED 0x23
#define IORS_VOL_NOT_REMOVABLE 0x24
#define IORS_VOL_IN_USE 0x25
#define IORS_LOCK_COUNT_EXCEEDED 0x26
#define IORS_VALID_EJECT_FAILED 0x27
#define IORS_ILLEGAL_ACCESS_MODE 0x28
#define IORS_LOCK_VIOLATION 0x29
#define IORS_WRONG_MEDIA 0x2A
#define IORS_OUT_OF_SPACE 0x2B
#define IORS_BIG_IO_BREAKUP_FAILED 0x2C
#define IORS_ERROR_DESIGNTR 0x10
#define IORS_INVALID_PARM 0x10
#define IORF_HIGH_PRIORITY_BIT 0
#define IORF_HIGH_PRIORITY 0x01
#define IORF_SCATTER_GATHER_BIT 1
#define IORF_SCATTER_GATHER 0x02
#define IORF_DONT_CACHE_BIT 2
#define IORF_DONT_CACHE 0x04
#define IORF_BYPASS_VOLTRK 0x08
#define IORF_BYPASS_VOLTRK_BIT 3
#define IORF_16BIT_IOCTL 0x10
#define IORF_16BIT_IOCTL_BIT 4
#define IORF_SWAPPER_IO 0x20
#define IORF_SWAPPER_IO_BIT 5
#define IORF_DOUBLE_BUFFER 0x40
#define IORF_DOUBLE_BUFFER_BIT 6
#define IORF_SYNC_CMD_DONE 0x80
#define IORF_SYNC_CMD_DONE_BIT 7
#define IORF_SYNC_COMMAND 0x0100
#define IORF_SYNC_COMMAND_BIT 8
#define IORF_CHAR_COMMAND 0x0200
#define IORF_CHAR_COMMAND_BIT 9
#define IORF_VERSION_002 0x0400
#define IORF_VERSION_002_BIT 10
#define IORF_BYPASS_QUEUE 0x0800
#define IORF_BYPASS_QUEUE_BIT 11
#define IORF_BLOCKDEV_EMULATE 0x1000
#define IORF_BLOCKDEV_EMULATE_BIT 12
#define IORF_POSTPONED_VOL_OPS 0x2000
#define IORF_POSTPONED_VOL_OPS_BIT 13
#define IORF_INHIBIT_GEOM_RECOMPUTE 0x4000
#define IORF_INHIBIT_GEOM_RECOMPUTE_BIT 14
#define IORF_SRB_VALID 0x8000
#define IORF_SRB_VALID_BIT 15
#define IORF_BYPASS_A_B 0x00010000
#define IOFR_BYPASS_A_B_BIT 16
#define IORF_QUIET_VOLTRK 0x00020000
#define IORF_QUIET_VOLTRL_BIT 17
#define IORF_AUDIO_DATA_READ 0x00040000
#define IORF_AUDIO_DATA_READ_BIT 18
#define IORF_LOGICAL_START_SECTOR 0x00080000
#define IORF_LOGICAL_START_SECTOR_BIT 19
#define IORF_PARTITION_BIAS_ADDED 0x00100000
#define IORF_PARTITION_BIAS_ADDED_BIT 20
#define IORF_DATA_IN_BIT 21
#define IORF_DATA_IN 0x00200000
#define IORF_DATA_OUT_BIT 22
#define IORF_DATA_OUT 0x00400000
#define IORF_VOL_RETRY_BIT 23
#define IORF_VOL_RETRY 0x00800000
#define IORF_NO_COMPRESS_BIT 24
#define IORF_NO_COMPRESS 0x01000000
#define IORF_DIRECT_IO_BIT 25
#define IORF_DIRECT_IO 0x02000000
#define IORF_PHYS_SGDS_BIT 26
#define IORF_PHYS_SGDS 0x04000000
#define IORF_IO_TOO_BIG_BIT 27
#define IORF_IO_TOO_BIG 0x08000000
#define IORF_WIN32_BIT 28
#define IORF_WIN32 0x10000000
#define IORF_CHAR_DEVICE_BIT 29
#define IORF_CHAR_DEVICE 0x20000000
#define IORF_PHYS_CMD_BIT 30
#define IORF_PHYS_CMD 0x40000000
#define IORF_IDE_RESERVED_BIT 31
#define IORF_IDE_RESERVED (-2147483648)
struct type_sdeffsd_req_usage {
  USHORT _IOR_ioctl_drive;
  USHORT _IOR_ioctl_function;
  ULONG _IOR_ioctl_control_param;
  ULONG _IOR_ioctl_buffer_ptr;
  ULONG _IOR_ioctl_client_params;
  ULONG _IOR_ioctl_return;
};
typedef struct type_sdeffsd_req_usage _type_sdeffsd_req_usage;
union urequestor_usage {
  ULONG _IOR_requestor_usage[5];
  struct type_sdeffsd_req_usage sdeffsd_req_usage;
};
typedef union urequestor_usage _urequestor_usage;
typedef void (*CMDCPLT)(void);
struct _IOR {
  ULONG IOR_next;
  USHORT IOR_func;
  USHORT IOR_status;
  ULONG IOR_flags;
  CMDCPLT IOR_callback;
  ULONG IOR_start_addr[2];
  ULONG IOR_xfer_count;
  ULONG IOR_buffer_ptr;
  ULONG IOR_private_client;
  ULONG IOR_private_IOS;
  ULONG IOR_private_port;
  union urequestor_usage _ureq;
  ULONG IOR_req_req_handle;
  ULONG IOR_req_vol_handle;
  ULONG IOR_sgd_lin_phys;
  UCHAR IOR_num_sgds;
  UCHAR IOR_vol_designtr;
  USHORT IOR_ios_private_1;
  ULONG IOR_reserved_2[2];
};
typedef struct _IOR IOR;
typedef struct _IOR *PIOR;

#pragma pack(pop)

#endif
