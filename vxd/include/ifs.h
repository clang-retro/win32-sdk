/* ifs.h: The installable file system manager (IFSMgr):
   ioreq, the request a file system driver or a file system API hook
   gets, the IFSFN_* function numbers, the file and volume handle
   function tables, the error codes and the structures of the IFSMgr
   services.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_IFS_H
#define VXD_IFS_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define IFS_INC 1
#define DOS_VXD 1
#define WIN32_APIS 1
#define IFS_VERSION 0x030A
#define IFS_REVISION 0x10
#define MAX_PATH 260
#define LFNMAXNAMELEN 255
#define MAXIMUM_USERID 2
#define NULL_USER_ID 0
#define STATUS_PENDING (-1)
#define STATUS_RAWLOCK (-2)
#define STATUS_BUSY (-3)
#define ANYPROID (-1)
#define NF_PROCEXIT 0x111D
#define NF_DRIVEUSE 0x01
#define NF_DRIVEUNUSE 0x02
#define NF_GETPRINTJOBID 0x03
#define NF_PRINTERUSE 0x04
#define NF_PRINTERUNUSE 0x05
#define NF_NetSetUserName 0x1181
#define WIN32_CALLFLAG 0x04
#if VXD_WINVER >= 0x040A
#define FSTYPE_LOCAL_FSD 0
#define FSTYPE_NET_FSD 1
#define FSTYPE_MAILSLOT_FSD 2
#define FSTYPE_CHARACTER_FSD 3
#define FORCE_LEV_UNC 0
#define FORCE_LEV_USE 1
#define FORCE_LEV_CLOSE_FILES 2
#define FORCE_LEV_CLEANUP 3
#define FORCE_LEV_BLAST 4
#define FS_PRIORITY_LOWEST 0x00
#define FS_PRIORITY_LOW 0x20
#define FS_PRIORITY_DEFAULT 0x40
#define FS_PRIORITY_MEDIUM 0x80
#define FS_PRIORITY_HIGH 0xC0
#define FS_PRIORITY_HIGHEST 0x0100
#define FS_ATTRIB_WIN95COMPAT 0x00
#define FS_ATTRIB_LARGE_MEDIA 0x01
#define FS_ATTRIB_ACCESS_METHOD_HINTS 0x02
#define FS_SUPPRESS_MRU_MOUNT_ORDER 0x04
#endif
#define NUM_HNDLMISC 8
#define HM_SEEK 0
#define HM_CLOSE 1
#define HM_COMMIT 2
#define HM_FILELOCKS 3
#define HM_FILETIMES 4
#define HM_PIPEREQUEST 5
#define HM_HANDLEINFO 6
#define HM_ENUMHANDLE 7
#define NUM_VOLFUNC 15
#define VFN_DELETE 0
#define VFN_DIR 1
#define VFN_FILEATTRIB 2
#define VFN_FLUSH 3
#define VFN_GETDISKINFO 4
#define VFN_OPEN 5
#define VFN_RENAME 6
#define VFN_SEARCH 7
#define VFN_QUERY 8
#define VFN_DISCONNECT 9
#define VFN_UNCPIPEREQ 10
#define VFN_IOCTL16DRIVE 11
#define VFN_GETDISKPARMS 12
#define VFN_FINDOPEN 13
#define VFN_DASDIO 14
#define IFSFN_READ 0
#define IFSFN_WRITE 1
#define IFSFN_FINDNEXT 2
#define IFSFN_FCNNEXT 3
#define IFSFN_SEEK 10
#define IFSFN_CLOSE 11
#define IFSFN_COMMIT 12
#define IFSFN_FILELOCKS 13
#define IFSFN_FILETIMES 14
#define IFSFN_PIPEREQUEST 15
#define IFSFN_HANDLEINFO 16
#define IFSFN_ENUMHANDLE 17
#define IFSFN_FINDCLOSE 18
#define IFSFN_FCNCLOSE 19
#define IFSFN_CONNECT 30
#define IFSFN_DELETE 31
#define IFSFN_DIR 32
#define IFSFN_FILEATTRIB 33
#define IFSFN_FLUSH 34
#define IFSFN_GETDISKINFO 35
#define IFSFN_OPEN 36
#define IFSFN_RENAME 37
#define IFSFN_SEARCH 38
#define IFSFN_QUERY 39
#define IFSFN_DISCONNECT 40
#define IFSFN_UNCPIPEREQ 41
#define IFSFN_IOCTL16DRIVE 42
#define IFSFN_GETDISKPARMS 43
#define IFSFN_FINDOPEN 44
#define IFSFN_DASDIO 45
#define IFSFH_RES_UNC 0x01
#define IFSFH_RES_NETWORK 0x08
#define IFSFH_RES_LOCAL 0x10
#define IFSFH_RES_CFSD 0x80
#define RESOPT_UNC_REQUEST 0x01
#define RESOPT_DEV_ATTACH 0x02
#define RESOPT_UNC_CONNECT 0x04
#define RESOPT_DISCONNECTED 0x08
#define RESOPT_NO_CREATE 0x10
#define RESOPT_STATIC 0x20
#define RESTYPE_WILD 0
#define RESTYPE_DISK 1
#define RESTYPE_SPOOL 2
#define RESTYPE_CHARDEV 3
#define RESTYPE_IPC 4
#define FIRST_RESTYPE 0x01
#define LAST_RESTYPE 0x04
#define RESOPT_NO_IO 0x01
#define IR_FSD_MOUNT 0
#define IR_FSD_VERIFY 2
#define IR_FSD_UNLOAD 3
#define IR_FSD_MOUNT_CHILD 4
#define IR_FSD_MAP_DRIVE 5
#define IR_FSD_UNMAP_DRIVE 6
#define ERROR_IFSVOL_EXISTS 284
#define IR_TUNA_NOTUNNEL 0
#define IR_TUNA_FSDTUNNEL 0
#define IR_TUNA_IFSTUNNEL 1
#define PNPE_SUCCESS 0x00
#define PNPE_QUERY_ACCEPTED 0x00
#define PNPE_QUERY_REFUSED 0x01
#define PNPE_BAD_ARGS 0x02
#define PNPE_UNDEFINED 0xFF
#define PNPT_VOLUME 0x10000000
#define PNPT_NET 0x20000000
#define PNPT_MASK 0xF0000000
#define RESSTAT_OK 0
#define RESSTAT_PAUSED 1
#define RESSTAT_DISCONN 2
#define RESSTAT_ERROR 3
#define RESSTAT_CONN 4
#define RESSTAT_RECONN 5
#define CLOSE_HANDLE 0
#define CLOSE_FOR_PROCESS 1
#define CLOSE_FINAL 2
#define FILE_NO_LAST_ACCESS_DATE 0x01
#define FILE_CLOSE_FOR_LEVEL4_LOCK 0x02
#define FILE_COMMIT_ASYNC 0x04
#define FILE_FIND_RESTART 0x40
#define IOOPT_PRT_SPEC 0x80
#define CREATE_DIR 0
#define DELETE_DIR 1
#define CHECK_DIR 2
#define QUERY83_DIR 3
#define QUERYLONG_DIR 4
#define LOCK_REGION 0
#define UNLOCK_REGION 1
#define LOCKF_MASK_RDWR 0x01
#define LOCKF_WR 0x00
#define LOCKF_RD 0x01
#define LOCKF_MASK_DOS_NW 0x02
#define LOCKF_DOS 0x00
#define LOCKF_NW 0x02
#define LOCKF_MASK_INACTIVE 0x80
#define LOCKF_ACTIVE 0x00
#define LOCKF_INACTIVE 0x80
#define PIPE_QHandState 0x21
#define PIPE_SetHandState 0x01
#define PIPE_QInfo 0x22
#define PIPE_Peek 0x23
#define PIPE_RawRead 0x11
#define PIPE_RawWrite 0x31
#define PIPE_Wait 0x53
#define PIPE_Call 0x54
#define PIPE_Transact 0x26
#define HINFO_GET 0
#define HINFO_SETALL 1
#define HINFO_SETCHARTIME 2
#define HINFO_SETCHARCOUNT 3
#define ENUMH_GETFILEINFO 0
#define ENUMH_GETFILENAME 1
#define ENUMH_GETFINDINFO 2
#define ENUMH_RESUMEFIND 3
#define ENUMH_RESYNCFILEDIR 4
#if VXD_WINVER >= 0x040A
#define GETFILENAME_FLAG_FILEDATA 0x01
#endif
#define RESYNC_INVALIDATEMETACACHE 0x01
#define GET_ATTRIBUTES 0
#define SET_ATTRIBUTES 1
#define GET_ATTRIB_COMP_FILESIZE 2
#define SET_ATTRIB_MODIFY_DATETIME 3
#define GET_ATTRIB_MODIFY_DATETIME 4
#define SET_ATTRIB_LAST_ACCESS_DATETIME 5
#define GET_ATTRIB_LAST_ACCESS_DATETIME 6
#define SET_ATTRIB_CREATION_DATETIME 7
#define GET_ATTRIB_CREATION_DATETIME 8
#define GET_ATTRIB_FIRST_CLUST 9
#define GDF_NORMAL 0x00
#define GDF_NO_DISK_HIT 0x01
#if VXD_WINVER >= 0x040A
#define GDF_R0_EXT_FREESPACE 0x80
#endif
#define GET_MODIFY_DATETIME 0
#define SET_MODIFY_DATETIME 1
#define GET_LAST_ACCESS_DATETIME 4
#define SET_LAST_ACCESS_DATETIME 5
#define GET_CREATION_DATETIME 6
#define SET_CREATION_DATETIME 7
#define FILE_BEGIN 0
#define FILE_END 2
#define ACCESS_MODE_MASK 0x07
#define ACCESS_READONLY 0x00
#define ACCESS_WRITEONLY 0x01
#define ACCESS_READWRITE 0x02
#define ACCESS_EXECUTE 0x03
#define SHARE_MODE_MASK 0x70
#define SHARE_COMPATIBILITY 0x00
#define SHARE_DENYREADWRITE 0x10
#define SHARE_DENYWRITE 0x20
#define SHARE_DENYREAD 0x30
#define SHARE_DENYNONE 0x40
#define SHARE_FCB 0x70
#define ACTION_MASK 0xFF
#define ACTION_OPENEXISTING 0x01
#define ACTION_REPLACEEXISTING 0x02
#define ACTION_CREATENEW 0x10
#define ACTION_OPENALWAYS 0x11
#define ACTION_CREATEALWAYS 0x12
#define ACTION_EXISTS_OPEN 0x01
#define ACTION_TRUNCATE 0x02
#define ACTION_NEXISTS_CREATE 0x10
#define OPEN_FLAGS_NOINHERIT 0x80
#define OPEN_FLAGS_NO_CACHE 0x0100
#define OPEN_FLAGS_NO_COMPRESS 0x0200
#define OPEN_FLAGS_ALIAS_HINT 0x0400
#define OPEN_FLAGS_REOPEN 0x0800
#if VXD_WINVER >= 0x040A
#define OPEN_FLAGS_RSVD_1 0x1000
#endif
#define OPEN_FLAGS_NOCRITERR 0x2000
#define OPEN_FLAGS_COMMIT 0x4000
#if VXD_WINVER >= 0x040A
#define OPEN_FLAGS_RSVD_2 0x8000
#define OPEN_FLAGS_EXTENDED_SIZE 0x00010000
#define OPEN_FLAGS_RAND_ACCESS_HINT 0x00020000
#define OPEN_FLAGS_SEQ_ACCESS_HINT 0x00040000
#define OPEN_EXT_FLAGS_MASK 0x00FF0000
#endif
#define ACTION_OPENED 1
#define ACTION_CREATED 2
#define ACTION_REPLACED 3
#define SEARCH_FIRST 0
#define SEARCH_NEXT 1
#define DISCONNECT_NORMAL 0
#define DISCONNECT_NO_IO 1
#define DISCONNECT_SINGLE 2
#define VOL_DISCARD_CACHE 1
#define VOL_REMOUNT 2
#if VXD_WINVER >= 0x040A
#define GDF_EXTENDED_FREESPACE 0x01
#endif
#define IOCTL_PKT_V86_ADDRESS 0
#define IOCTL_PKT_LINEAR_ADDRESS 1
#if VXD_WINVER >= 0x040A
#define GDP_EXTENDED_PARMS 0x01
#endif
#define DIO_ABS_READ_SECTORS 0
#define DIO_ABS_WRITE_SECTORS 1
#define DIO_SET_LOCK_CACHE_STATE 2
#if VXD_WINVER >= 0x040A
#define DIO_SET_DPB_FOR_FORMAT 3
#define ABS_EXTENDED_DASDIO 0x01
#endif
#define DLC_LEVEL4LOCK_TAKEN 0x01
#define DLC_LEVEL4LOCK_RELEASED 0x02
#define DLC_LEVEL1LOCK_TAKEN 0x04
#define DLC_LEVEL1LOCK_RELEASED 0x08
#if VXD_WINVER >= 0x040A
#define DLC_LOCKS_TAKEN 0x10
#define DLC_LOCKS_RELEASED 0x20
#endif
#define R0_NO_CACHE 0x0100
#define R0_SWAPPER_CALL 0x1000
#if VXD_WINVER >= 0x040A
#define R0_LOADER_IO 0x2000
#endif
#define R0_MM_READ_WRITE 0x8000
#define R0_SPLOPT_MASK 0xFF00
#define FILE_ATTRIBUTE_READONLY 0x01
#define FILE_ATTRIBUTE_HIDDEN 0x02
#define FILE_ATTRIBUTE_SYSTEM 0x04
#define FILE_ATTRIBUTE_LABEL 0x08
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_ATTRIBUTE_ARCHIVE 0x20
#define FILE_ATTRIBUTE_DEVICE 0x40
#define FILE_ATTRIBUTE_MUSTMATCH 0x3F00
#define FILE_ATTRIBUTE_EVERYTHING 0x3F
#define FILE_ATTRIBUTE_INTERESTING 0x1E
#define BASIS_TRUNC 0x01
#define BASIS_LOSS 0x02
#define BASIS_UPCASE 0x04
#define BASIS_EXT 0x20
#define BASIS_NAME_COLL 0x08
#define BASIS_NUM_TAIL_COLL 0x10
#define FIND_FLAG_PRI_NAME_LOSS 0x01
#define FIND_FLAG_ALT_NAME_LOSS 0x02
#define MAP_FLAG_LOSS 0x01
#define MAP_FLAG_TRUNCATE 0x02
#define FILE_FLAG_WILDCARDS 0x80000000
#define FILE_FLAG_HAS_STAR 0x40000000
#define FILE_FLAG_LONG_PATH 0x20000000
#define FILE_FLAG_KEEP_CASE 0x10000000
#define FILE_FLAG_HAS_DOT 0x08000000
#define FILE_FLAG_IS_LFN 0x04000000
#define R0_OPENCREATFILE 0xD500
#define R0_OPENCREAT_IN_CONTEXT 0xD501
#define R0_READFILE 0xD600
#define R0_WRITEFILE 0xD601
#define R0_READFILE_IN_CONTEXT 0xD602
#define R0_WRITEFILE_IN_CONTEXT 0xD603
#define R0_CLOSEFILE 0xD700
#define R0_GETFILESIZE 0xD800
#define R0_FINDFIRSTFILE 0x4E00
#define R0_FINDNEXTFILE 0x4F00
#define R0_FINDCLOSEFILE 0xDC00
#define R0_FILEATTRIBUTES 0x4300
#define R0_RENAMEFILE 0x5600
#define R0_DELETEFILE 0x4100
#define R0_LOCKFILE 0x5C00
#define R0_GETDISKFREESPACE 0x3600
#define R0_READABSOLUTEDISK 0xDD00
#define R0_WRITEABSOLUTEDISK 0xDE00
#if VXD_WINVER >= 0x040A
#define R0_HANDLETOPATH 0xEC00
#define R0_MAPCACHEBLOCK 0xED00
#define R0_GETVOLLOCKLEVEL 0xEE00
#endif
#define IFS_DRV_RMM 0x01
#define IFS_DRV_DOS_DISK_INFO 0x02
#define FS_CASE_IS_PRESERVED 0x02
#define FS_UNICODE_STORED_ON_DISK 0x04
#define FS_VOL_IS_COMPRESSED 0x8000
#define FS_VOL_SUPPORTS_LONG_NAMES 0x4000
#define FDRV_INT13 0x01
#define FDRV_FASTDISK 0x02
#define FDRV_COMP 0x04
#define FDRV_RMM 0x08
#define FDRV_DOS 0x10
#define FDRV_USE_RMM 0x20
#define FDRV_COMPHOST 0x40
#define FDRV_NO_LAZY 0x80
#define TUNI_CONTAINS_ALTNAME 0x01
#define TUNI_CONTAINS_CREATIONT 0x02
#define TUNI_CONTAINS_LASTACCESST 0x04
#define TUNI_CONTAINS_LASTWRITET 0x08
#define BCS_WANSI 0
#define BCS_OEM 1
#define BCS_UNI 2
#define UFLG_META 0x01
#define UFLG_NT 0x02
#define UFLG_NT_DOS 0x04
#define UFLG_DOS 0x00
#define IFS_IOCTL_21 100
#define IFS_IOCTL_2F 101
#define IFS_IOCTL_GET_RES 102
#define IFS_IOCTL_GET_NETPRO_NAME_A 103
#define FSD_NETAPI_USEOEM 0x01
#define FSD_NETAPI_STATIC 0x02
#define FSD_NETAPI_USELFN 0x04
#define DUP_NORMAL_HANDLE 0x00
#define DUP_MEMORY_MAPPED 0x01
#define DUP_MEM_MAPPED_WRITE 0x02
#define NAMTRN_NORMALQUERY 0x00
#define NAMTRN_DO83QUERY 0x01
#define NAMTRN_DOLFNQUERY 0x02
#define ENUM_ALL_FILES 0x00
#define ENUM_UNMOVEABLE_FILES 0x01
#define WDUP_RMM_DRIVE 0x01
#define WDUP_NETWARE_HANDLE 0x02
typedef void *rh_t;
typedef void *fh_t;
typedef int fsdwork_t[16];
typedef unsigned short *string_t;
typedef unsigned short sfn_t;
typedef unsigned long pos_t;
typedef unsigned int pid_t;
typedef void *ubuffer_t;
typedef unsigned char uid_t;
struct PathElement;
typedef struct PathElement PathElement;
struct ParsedPath;
typedef struct ParsedPath ParsedPath;
typedef ParsedPath *path_t;
struct ioreq;
typedef struct ioreq ioreq;
typedef struct ioreq *pioreq;
struct dos_time;
typedef struct dos_time dos_time;
struct dos_time {
  unsigned short dt_time;
  unsigned short dt_date;
};
#if VXD_WINVER >= 0x040A
struct dos_time_rounded;
typedef struct dos_time_rounded dos_time_rounded;
struct dos_time_rounded {
  unsigned short dtr_time_rounded;
  unsigned short dtr_date_rounded;
  unsigned short dtr_time;
  unsigned short dtr_date;
  unsigned char dtr_time_msec;
};
#endif
struct volfunc;
typedef struct volfunc volfunc;
typedef struct volfunc *vfunc_t;
struct hndlfunc;
typedef struct hndlfunc hndlfunc;
typedef struct hndlfunc *hfunc_t;
union aux_t {
  ubuffer_t aux_buf;
  unsigned long aux_ul;
  dos_time aux_dt;
  vfunc_t aux_vf;
  hfunc_t aux_hf;
  void *aux_ptr;
  string_t aux_str;
  path_t aux_pp;
  unsigned int aux_ui;
};
typedef union aux_t aux_t;
struct event;
typedef struct event event;
typedef struct event *pevent;
struct ioreq {
  unsigned int ir_length;
  unsigned char ir_flags;
  uid_t ir_user;
  sfn_t ir_sfn;
  pid_t ir_pid;
  path_t ir_ppath;
  aux_t ir_aux1;
  ubuffer_t ir_data;
  unsigned short ir_options;
  short ir_error;
  rh_t ir_rh;
  fh_t ir_fh;
  pos_t ir_pos;
  aux_t ir_aux2;
  aux_t ir_aux3;
  pevent ir_pev;
  fsdwork_t ir_fsd;
};
typedef int IFSFunc(pioreq);
typedef IFSFunc *pIFSFunc;
typedef int IFSFileHookFunc(pIFSFunc, int, int, int, int, pioreq);
typedef IFSFileHookFunc *pIFSFileHookFunc;
typedef pIFSFileHookFunc *ppIFSFileHookFunc;
struct hndlmisc;
typedef struct hndlmisc hndlmisc;
struct hndlfunc {
  pIFSFunc hf_read;
  pIFSFunc hf_write;
  hndlmisc *hf_misc;
};
struct hndlmisc {
  short hm_version;
  char hm_revision;
  char hm_size;
  pIFSFunc hm_func[8];
};
struct volfunc {
  short vfn_version;
  char vfn_revision;
  char vfn_size;
  pIFSFunc vfn_func[15];
};
struct srch_key;
typedef struct srch_key srch_key;
struct srch_key {
  unsigned char sk_drive;
  unsigned char sk_pattern[11];
  unsigned char sk_attr;
  unsigned char sk_localFSD[4];
  unsigned char sk_netFSD[2];
  unsigned char sk_ifsmgr[2];
};
struct srch_entry;
typedef struct srch_entry srch_entry;
struct srch_entry {
  struct srch_key se_key;
  unsigned char se_attrib;
  unsigned short se_time;
  unsigned short se_date;
  unsigned long se_size;
  char se_name[13];
};
struct _FILETIME;
typedef struct _FILETIME _FILETIME;
struct _FILETIME {
  unsigned long dwLowDateTime;
  unsigned long dwHighDateTime;
};
struct _WIN32_FIND_DATA;
typedef struct _WIN32_FIND_DATA _WIN32_FIND_DATA;
struct _WIN32_FIND_DATA {
  unsigned long dwFileAttributes;
  struct _FILETIME ftCreationTime;
  struct _FILETIME ftLastAccessTime;
  struct _FILETIME ftLastWriteTime;
  unsigned long nFileSizeHigh;
  unsigned long nFileSizeLow;
  unsigned long dwReserved0;
  unsigned long dwReserved1;
  unsigned short cFileName[260];
  unsigned short cAlternateFileName[14];
};
struct _BY_HANDLE_FILE_INFORMATION;
typedef struct _BY_HANDLE_FILE_INFORMATION _BY_HANDLE_FILE_INFORMATION;
struct _BY_HANDLE_FILE_INFORMATION {
  unsigned long bhfi_dwFileAttributes;
  struct _FILETIME bhfi_ftCreationTime;
  struct _FILETIME bhfi_ftLastAccessTime;
  struct _FILETIME bhfi_ftLastWriteTime;
  unsigned long bhfi_dwVolumeSerialNumber;
  unsigned long bhfi_nFileSizeHigh;
  unsigned long bhfi_nFileSizeLow;
  unsigned long bhfi_nNumberOfLinks;
  unsigned long bhfi_nFileIndexHigh;
  unsigned long bhfi_nFileIndexLow;
};
struct TUNINFO;
typedef struct TUNINFO TUNINFO;
struct TUNINFO {
  unsigned long tuni_bfContents;
  short *tuni_pAltName;
  struct _FILETIME tuni_ftCreationTime;
  struct _FILETIME tuni_ftLastAccessTime;
  struct _FILETIME tuni_ftLastWriteTime;
};
struct _QWORD;
typedef struct _QWORD _QWORD;
struct _QWORD {
  unsigned long ddLower;
  unsigned long ddUpper;
};
struct PathElement {
  unsigned short pe_length;
  unsigned short pe_unichars[1];
};
struct ParsedPath {
  unsigned short pp_totalLength;
  unsigned short pp_prefixLength;
  struct PathElement pp_elements[1];
};
struct CPPtrs;
typedef struct CPPtrs CPPtrs;
struct CPPtrs {
  unsigned long AnsiPtr;
  unsigned long OEMPtr;
};
struct UnitoUpperTab;
typedef struct UnitoUpperTab UnitoUpperTab;
struct UnitoUpperTab {
  unsigned long delta;
  unsigned long TabPtr;
};
struct CPTablePtrs;
typedef struct CPTablePtrs CPTablePtrs;
struct CPTablePtrs {
  unsigned long CPT_Length;
  struct CPPtrs utbPtrTab;
  struct CPPtrs btuPtrTab;
  struct UnitoUpperTab UnitoUpperPtr;
};
struct win32apireq {
  unsigned long ar_proid;
  unsigned long ar_eax;
  unsigned long ar_ebx;
  unsigned long ar_ecx;
  unsigned long ar_edx;
  unsigned long ar_esi;
  unsigned long ar_edi;
  unsigned long ar_ebp;
  unsigned short ar_error;
  unsigned short ar_pad;
};
struct netuse_info;
typedef struct netuse_info netuse_info;
struct netuse_info {
  void *nu_data;
  int nu_info;
  unsigned long nu_flags;
  unsigned long nu_rsvd;
};
struct fmode_t {
  unsigned long fm_uid;
  void *fm_cookie0;
  void *fm_cookie1;
  unsigned short fm_mode;
  unsigned short fm_attr;
};
typedef struct fmode_t fmode_t;
struct SFTOpenInfo;
typedef struct SFTOpenInfo SFTOpenInfo;
typedef struct SFTOpenInfo *pSFTOpenInfo;
#if VXD_WINVER >= 0x040A
struct SFTOpenInfo {
  unsigned long soi_dirclus;
  unsigned short soi_dirind;
  unsigned char soi_dirname[11];
  unsigned char soi_pad[3];
};
#else
struct SFTOpenInfo {
  unsigned short soi_dirclus;
  unsigned short soi_dirind;
  unsigned char soi_dirname[11];
  unsigned char soi_pad;
};
#endif

#pragma pack(pop)

#endif
