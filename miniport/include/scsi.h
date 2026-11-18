/* scsi.h: SCSI itself: the CDB
   layouts, the SCSI operation codes (SCSIOP_*), sense data and inquiry
   data.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   _WIN32_WINDOWS whose DDK has it. */
#ifndef MINIPORT9X_SCSI_H
#define MINIPORT9X_SCSI_H

#include <miniport.h>
#include <srb.h>
#define CDB6GENERIC_LENGTH 6
#define CDB10GENERIC_LENGTH 10
#define CDB_6_byte 6
#define CDB_10_byte 10
#define SETBITON 1
#define SETBITOFF 0
#define MODE_PAGE_ERROR_RECOVERY 0x01
#define MODE_PAGE_DISCONNECT 0x02
#define MODE_PAGE_FORMAT_DEVICE 0x03
#define MODE_PAGE_RIGID_GEOMETRY 0x04
#define MODE_PAGE_FLEXIBILE 0x05
#define MODE_PAGE_VERIFY_ERROR 0x07
#define MODE_PAGE_CACHING 0x08
#define MODE_PAGE_PERIPHERAL 0x09
#define MODE_PAGE_CONTROL 0x0A
#define MODE_PAGE_MEDIUM_TYPES 0x0B
#define MODE_PAGE_NOTCH_PARTITION 0x0C
#define MODE_SENSE_RETURN_ALL 0x3F
#define MODE_SENSE_CURRENT_VALUES 0x00
#define MODE_SENSE_CHANGEABLE_VALUES 0x40
#define MODE_SENSE_DEFAULT_VAULES 0x80
#define MODE_SENSE_SAVED_VALUES 0xC0
#define MODE_PAGE_DEVICE_CONFIG 0x10
#define MODE_PAGE_MEDIUM_PARTITION 0x11
#if _WIN32_WINDOWS >= 0x0410
#define MODE_PAGE_CDVD_CAPS 0x2A
#endif
#define SCSIOP_TEST_UNIT_READY 0x00
#define SCSIOP_REZERO_UNIT 0x01
#define SCSIOP_REWIND 0x01
#define SCSIOP_REQUEST_BLOCK_ADDR 0x02
#define SCSIOP_REQUEST_SENSE 0x03
#define SCSIOP_FORMAT_UNIT 0x04
#define SCSIOP_READ_BLOCK_LIMITS 0x05
#define SCSIOP_REASSIGN_BLOCKS 0x07
#define SCSIOP_READ6 0x08
#define SCSIOP_RECEIVE 0x08
#define SCSIOP_WRITE6 0x0A
#define SCSIOP_PRINT 0x0A
#define SCSIOP_SEND 0x0A
#define SCSIOP_SEEK6 0x0B
#define SCSIOP_TRACK_SELECT 0x0B
#define SCSIOP_SLEW_PRINT 0x0B
#define SCSIOP_SEEK_BLOCK 0x0C
#define SCSIOP_PARTITION 0x0D
#define SCSIOP_READ_REVERSE 0x0F
#define SCSIOP_WRITE_FILEMARKS 0x10
#define SCSIOP_FLUSH_BUFFER 0x10
#define SCSIOP_SPACE 0x11
#define SCSIOP_INQUIRY 0x12
#define SCSIOP_VERIFY6 0x13
#define SCSIOP_RECOVER_BUF_DATA 0x14
#define SCSIOP_MODE_SELECT 0x15
#define SCSIOP_RESERVE_UNIT 0x16
#define SCSIOP_RELEASE_UNIT 0x17
#define SCSIOP_COPY 0x18
#define SCSIOP_ERASE 0x19
#define SCSIOP_MODE_SENSE 0x1A
#define SCSIOP_START_STOP_UNIT 0x1B
#define SCSIOP_STOP_PRINT 0x1B
#define SCSIOP_LOAD_UNLOAD 0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC 0x1C
#define SCSIOP_SEND_DIAGNOSTIC 0x1D
#define SCSIOP_MEDIUM_REMOVAL 0x1E
#define SCSIOP_READ_CAPACITY 0x25
#define SCSIOP_READ 0x28
#define SCSIOP_WRITE 0x2A
#define SCSIOP_SEEK 0x2B
#define SCSIOP_LOCATE 0x2B
#define SCSIOP_WRITE_VERIFY 0x2E
#define SCSIOP_VERIFY 0x2F
#define SCSIOP_SEARCH_DATA_HIGH 0x30
#define SCSIOP_SEARCH_DATA_EQUAL 0x31
#define SCSIOP_SEARCH_DATA_LOW 0x32
#define SCSIOP_SET_LIMITS 0x33
#define SCSIOP_READ_POSITION 0x34
#define SCSIOP_COMPARE 0x39
#define SCSIOP_COPY_COMPARE 0x3A
#define SCSIOP_WRITE_DATA_BUFF 0x3B
#define SCSIOP_READ_DATA_BUFF 0x3C
#define SCSIOP_READ_LONG 0x3E
#define SCSIOP_CHANGE_DEFINITION 0x40
#define SCSIOP_READ_SUB_CHANNEL 0x42
#define SCSIOP_READ_TOC 0x43
#define SCSIOP_READ_HEADER 0x44
#define SCSIOP_PLAY_AUDIO_10 0x45
#define SCSIOP_PLAY_AUDIO_MSF 0x47
#define SCSIOP_PLAY_TRACK_INDEX 0x48
#define SCSIOP_PLAY_TRACK_RELATIVE 0x49
#define SCSIOP_PAUSE_RESUME 0x4B
#define SCSIOP_LOG_SELECT 0x4C
#define SCSIOP_LOG_SENSE 0x4D
#if _WIN32_WINDOWS >= 0x0410
#define SCSIOP_MODE_SELECT10 0x55
#define SCSIOP_MODE_SENSE10 0x5A
#endif
#define SCSIOP_PLAY_AUDIO 0xA5
#if _WIN32_WINDOWS >= 0x0410
#define SCSIOP_READ12 0xA8
#define SCSIOP_READCD 0xBE
#endif
#define SCSIOP_READ_CDDA 0xD8
#define CDB_RETURN_ON_COMPLETION 0
#define CDB_RETURN_IMMEDIATE 1
#define CDB_FORCE_MEDIA_ACCESS 0x08
#define SCSIOP_DENON_EJECT_DISC 0xE6
#define SCSIOP_DENON_STOP_AUDIO 0xE7
#define SCSIOP_DENON_PLAY_AUDIO 0xE8
#define SCSIOP_DENON_READ_TOC 0xE9
#define SCSIOP_DENON_READ_SUBCODE 0xEB
#define SCSIMESS_ABORT 0x06
#define SCSIMESS_ABORT_WITH_TAG 0x0D
#define SCSIMESS_BUS_DEVICE_RESET 0x0C
#define SCSIMESS_CLEAR_QUEUE 0x0E
#define SCSIMESS_COMMAND_COMPLETE 0x00
#define SCSIMESS_DISCONNECT 0x04
#define SCSIMESS_EXTENDED_MESSAGE 0x01
#define SCSIMESS_IDENTIFY 0x80
#define SCSIMESS_IDENTIFY_WITH_DISCON 0xC0
#define SCSIMESS_IGNORE_WIDE_RESIDUE 0x23
#define SCSIMESS_INITIATE_RECOVERY 0x0F
#define SCSIMESS_INIT_DETECTED_ERROR 0x05
#define SCSIMESS_LINK_CMD_COMP 0x0A
#define SCSIMESS_LINK_CMD_COMP_W_FLAG 0x0B
#define SCSIMESS_MESS_PARITY_ERROR 0x09
#define SCSIMESS_MESSAGE_REJECT 0x07
#define SCSIMESS_NO_OPERATION 0x08
#define SCSIMESS_HEAD_OF_QUEUE_TAG 0x21
#define SCSIMESS_ORDERED_QUEUE_TAG 0x22
#define SCSIMESS_SIMPLE_QUEUE_TAG 0x20
#define SCSIMESS_RELEASE_RECOVERY 0x10
#define SCSIMESS_RESTORE_POINTERS 0x03
#define SCSIMESS_SAVE_DATA_POINTER 0x02
#define SCSIMESS_TERMINATE_IO_PROCESS 0x11
#define SCSIMESS_MODIFY_DATA_POINTER 0x00
#define SCSIMESS_SYNCHRONOUS_DATA_REQ 0x01
#define SCSIMESS_WIDE_DATA_REQUEST 0x03
#define SCSIMESS_MODIFY_DATA_LENGTH 5
#define SCSIMESS_SYNCH_DATA_LENGTH 3
#define SCSIMESS_WIDE_DATA_LENGTH 2
#define SCSISTAT_GOOD 0x00
#define SCSISTAT_CHECK_CONDITION 0x02
#define SCSISTAT_CONDITION_MET 0x04
#define SCSISTAT_BUSY 0x08
#define SCSISTAT_INTERMEDIATE 0x10
#define SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define SCSISTAT_RESERVATION_CONFLICT 0x18
#define SCSISTAT_COMMAND_TERMINATED 0x22
#define SCSISTAT_QUEUE_FULL 0x28
#define CDB_INQUIRY_EVPD 0x01
#define LUN0_FORMAT_SAVING_DEFECT_LIST 0
#define USE_DEFAULTMSB 0
#define USE_DEFAULTLSB 0
#define START_UNIT_CODE 0x01
#define STOP_UNIT_CODE 0x00
#define INQUIRYDATABUFFERSIZE 36
#define MAXIMUM_NUMBER_LUNS 8
#define DIRECT_ACCESS_DEVICE 0x00
#define SEQUENTIAL_ACCESS_DEVICE 0x01
#define PRINTER_DEVICE 0x02
#define PROCESSOR_DEVICE 0x03
#define WRITE_ONCE_READ_MULTIPLE_DEVICE 0x04
#define READ_ONLY_DIRECT_ACCESS_DEVICE 0x05
#define SCANNER_DEVICE 0x06
#define LOGICAL_UNIT_NOT_PRESENT_DEVICE 0x7F
#define REMOVABLE_MASK 0x80
#define NOT_REMOVABLE 0x00
#define REMOVABLE 0x80
#define SENSE_BUFFER_SIZE 18
#define SCSI_SENSE_NO_SENSE 0x00
#define SCSI_SENSE_RECOVERED_ERROR 0x01
#define SCSI_SENSE_NOT_READY 0x02
#define SCSI_SENSE_MEDIUM_ERROR 0x03
#define SCSI_SENSE_HARDWARE_ERROR 0x04
#define SCSI_SENSE_ILLEGAL_REQUEST 0x05
#define SCSI_SENSE_UNIT_ATTENTION 0x06
#define SCSI_SENSE_DATA_PROTECT 0x07
#define SCSI_SENSE_BLANK_CHECK 0x08
#define SCSI_SENSE_UNIQUE 0x09
#define SCSI_SENSE_COPY_ABORTED 0x0A
#define SCSI_SENSE_ABORTED_COMMAND 0x0B
#define SCSI_SENSE_EQUAL 0x0C
#define SCSI_SENSE_VOL_OVERFLOW 0x0D
#define SCSI_SENSE_MISCOMPARE 0x0E
#define SCSI_SENSE_RESERVED 0x0F
#define SCSI_ILLEGAL_LENGTH 0x20
#define SCSI_EOM 0x40
#define SCSI_FILE_MARK 0x80
#define SCSI_ADSENSE_NO_SENSE 0x00
#define SCSI_ADSENSE_MAN_INTERV 0x03
#define SCSI_ADSENSE_LUN_NOT_READY 0x04
#define SCSI_ADSENSE_ILLEGAL_COMMAND 0x20
#define SCSI_ADSENSE_ILLEGAL_BLOCK 0x21
#define SCSI_ADSENSE_INVALID_LUN 0x25
#define SCSI_ADSENSE_SELECT_TIMEOUT 0x45
#if _WIN32_WINDOWS >= 0x0410
#define SCSI_ADSENSE_TIMEOUT 0x5E
#endif
#define SCSI_ADSENSE_MUSIC_AREA 0xA0
#define SCSI_ADSENSE_DATA_AREA 0xA1
#define SCSI_ADSENSE_VOLUME_OVERFLOW 0xA7
#define SCSI_ADSENSE_NO_MEDIA_IN_DEVICE 0x3A
#define SCSI_ADWRITE_PROTECT 0x27
#define SCSI_ADSENSE_MEDIUM_CHANGED 0x28
#define SCSI_ADSENSE_BUS_RESET 0x29
#define SCSI_ADSENSE_TRACK_ERROR 0x14
#define SCSI_ADSENSE_SEEK_ERROR 0x15
#define SCSI_ADSENSE_REC_DATA_NOECC 0x17
#define SCSI_ADSENSE_REC_DATA_ECC 0x18
#define SCSI_ADSENSE_ILLEGAL_MODE 0x64
#define SCSI_ADSENSE_BAD_CDB 0x24
#define SCSI_ADSENSE_BAD_PARM_LIST 0x26
#define SCSI_ADSENSE_CANNOT_READ_MEDIUM 0x30
#define SCSI_SENSEQ_FORMAT_IN_PROGRESS 0x04
#define SCSI_SENSEQ_INIT_COMMAND_REQUIRED 0x02
#define SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED 0x03
#define SCSI_SENSEQ_BECOMING_READY 0x01
#define SCSI_SENSEQ_INCOMP_FORMAT 0x02
#define FILE_DEVICE_SCSI 0x1B
#define IOCTL_SCSI_EXECUTE_IN 0x001B0011
#define IOCTL_SCSI_EXECUTE_OUT 0x001B0012
#define IOCTL_SCSI_EXECUTE_NONE 0x001B0013
#define MODE_FD_SINGLE_SIDE 0x01
#define MODE_FD_DOUBLE_SIDE 0x02
#define MODE_FD_MAXIMUM_TYPE 0x1E
#define LOGSENSEPAGE0 0x00
#define LOGSENSEPAGE2 0x02
#define LOGSENSEPAGE3 0x03
#define LOGSENSEPAGE30 0x30
#define LOGSENSEPAGE31 0x31
#define CDB_AUDIO_PAUSE 0
#define CDB_AUDIO_RESUME 1
#define CDB_DEVICE_START 0x11
#define CDB_DEVICE_STOP 0x10
#define CDB_EJECT_MEDIA 0x10
#define CDB_LOAD_MEDIA 0x01
#define CDB_SUBCHANNEL_HEADER 0x00
#define CDB_SUBCHANNEL_BLOCK 0x01
#define CDROM_AUDIO_CONTROL_PAGE 0x0E
#define CDROM_AUDIO_CURRENT_VALUES 0x00
#define MODE_SELECT_IMMEDIATE 0x04
#define CDB_USE_MSF 0x01
#if _WIN32_WINDOWS >= 0x0410
#define RC_READ_CDR 0x01
#define RC_READ_CDE 0x02
#define RC_READ_CDRFIXEDPKT 0x04
#define RC_READ_DVD 0x08
#define RC_READ_DVDR 0x10
#define RC_READ_DVDRAM 0x20
#define WC_WRITE_CDR 0x01
#define WC_WRITE_CDE 0x02
#define WC_WRITE_TESTMODE 0x04
#define WC_WRITE_DVDR 0x10
#define WC_WRITE_DVDRAM 0x20
#define CDC1_PLAY_AUDIO 0x01
#define CDC2_CD_DA 0x01
#define CDC2_DAACCU 0x02
#endif
#define CDAUDIO_LEAD_OUT_TRACK 0xAA
#define SESSION_INFO_MAX_BUFFER_SIZE 0x0800
#define TOC_CONTROL_READ_SESSION_INFO 0x40
#define TOC_CONTROL_READ_ALL_SUBCODE 0x80
#define CTRL_DATA_TRACK 0x04
#define SESSION_ADR_TRACK_INFO 0x10
#define SESSION_DESC_FIRST_TRACK 0xA0
#define SESSION_DESC_LAST_TRACK 0xA1
#define SESSION_DESC_LEAD_OUT_ADDR 0xA2
#define SESSION_DESC_END_OF_TRACKS 0x99
#define GET_LAST_SESSION 0x01
#define CD_READ_ALL_SUBQ 0x00
#define CD_READ_POSITION_SUBQ 0x01
#define CD_READ_UPC_CODE_SUBQ 0x02
#define CD_READ_ISRC_SUBQ 0x03
#define CDROM_STANDARD_XFER 0x96
#define CDROM_AUDIO_SUB_CHAN_FORMAT_ALL 0x00
#define CDROM_AUDIO_SUB_CHAN_CUR_POS 0x01
#define CDROM_AUDIO_SUB_CHAN_UPC 0x02
#define CDROM_AUDIO_SUB_CHAN_ISRC 0x03
#define CDROM_Q_SUB_CHAN_DATA_LEN 12
#define CDROM_UPC_ISRC_DATA_LEN 14
#define CDROM_Q_INFO_LEN 9
#define CDROM_AUDIO_STATUS_NOT_SUPPORTED 0x00
#define CDROM_AUDIO_STATUS_PLAYING 0x11
#define CDROM_AUDIO_STATUS_PAUSED 0x12
#define CDROM_AUDIO_STATUS_DONE 0x13
#define CDROM_AUDIO_STATUS_ERROR 0x14
#define CDROM_AUDIO_STATUS_NO_STATUS 0x15
#define CDROM_COOKED_SECTOR_SIZE 2048
#define CDROM_RAW_SECTOR_SIZE 2340
#define CD_DEFAULT_BLOCK_SIZE 0x0800
#define FRAMES_PER_SECOND 75
#define CDROM_AUDIO_LBA_INVALID 0x00
#define CDROM_AUDIO_IMED_RETURN 0x04
#define CDROM_AUDIO_SOTC 0x02
#pragma pack(push, 1)
struct _SUBCHAN_REC {
  UCHAR Reserved0 : 1;
  UCHAR Msf : 1;
  UCHAR Reserved1 : 3;
  UCHAR LogicalUnitNumber : 3;
};
#pragma pack(pop)
typedef struct _SUBCHAN_REC SUBCHAN_REC;
#pragma pack(push, 1)
struct _SUBCHAN_SUBQ_REC {
  UCHAR Reserved2 : 6;
  UCHAR SubQ : 1;
  UCHAR Reserved3 : 1;
};
#pragma pack(pop)
typedef struct _SUBCHAN_SUBQ_REC SUBCHAN_SUBQ_REC;
#pragma pack(push, 1)
struct _MODE_SNS_DBD_REC {
  UCHAR Reserved1 : 3;
  UCHAR Dbd : 1;
  UCHAR Reserved2 : 1;
  UCHAR LogicalUnitNumber : 3;
};
#pragma pack(pop)
typedef struct _MODE_SNS_DBD_REC MODE_SNS_DDB_REC;
#pragma pack(push, 1)
struct _MODE_SNS_PC_REC {
  UCHAR PageCode : 6;
  UCHAR Pc : 2;
};
#pragma pack(pop)
typedef struct _MODE_SNS_PC_REC MODE_SNS_PC_REC;
#pragma pack(push, 1)
struct _MODE_SEL_PF_REC {
  UCHAR Reserved1 : 4;
  UCHAR MS_PF : 1;
  UCHAR MSEL_LogicalUnitNumber : 3;
};
#pragma pack(pop)
typedef struct _MODE_SEL_PF_REC MODE_SEL_PF_REC;
#pragma pack(push, 1)
struct _CDB6GENERIC {
  UCHAR OperationCode;
  UCHAR Immediate : 1;
  UCHAR CommandUniqueBits : 4;
  UCHAR LogicalUnitNumber : 3;
  UCHAR CommandUniqueBytes[3];
  UCHAR Link : 1;
  UCHAR Flag : 1;
  UCHAR Reserved : 4;
  UCHAR VendorUnique : 2;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _CDB6READWRITE {
  UCHAR OperationCode;
  UCHAR LogicalBlockMsb1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR LogicalBlockMsb0;
  UCHAR LogicalBlockLsb;
  UCHAR TransferBlocks;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _CDB6INQUIRY {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR PageCode;
  UCHAR IReserved;
  UCHAR AllocationLength;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _CDB6FORMAT {
  UCHAR OperationCode;
  UCHAR FormatControl : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR FReserved1;
  UCHAR InterleaveMsb;
  UCHAR InterleaveLsb;
  UCHAR FReserved2;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _CDB6SENDDIAG {
  UCHAR OperationCode;
  UCHAR UnitOfl : 1;
  UCHAR DevOfl : 1;
  UCHAR SelfTest : 1;
  UCHAR Reserved1 : 1;
  UCHAR PF : 1;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2;
  UCHAR ParamListLenMsb;
  UCHAR ParamListLenLsb;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _CDB6RECDIAG {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved;
  UCHAR AllocationLenMsb;
  UCHAR AllocationLenLsb;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _CDB10 {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR LogicalBlockByte0;
  UCHAR LogicalBlockByte1;
  UCHAR LogicalBlockByte2;
  UCHAR LogicalBlockByte3;
  UCHAR Reserved2;
  UCHAR TransferBlocksMsb;
  UCHAR TransferBlocksLsb;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _READ_CAPACITY {
  UCHAR OperationCode;
  UCHAR RelAdr : 1;
  UCHAR Reserved1 : 4;
  UCHAR LogicalUnitNumber : 3;
  UCHAR LogicalBlockByte0;
  UCHAR LogicalBlockByte1;
  UCHAR LogicalBlockByte2;
  UCHAR LogicalBlockByte3;
  UCHAR Reserved2;
  UCHAR Reserved3;
  UCHAR PMI : 1;
  UCHAR Reserved4 : 7;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _CDB10READWRITE {
  UCHAR OperationCode;
  UCHAR RelAdr : 1;
  UCHAR Reserved1 : 2;
  UCHAR FUA : 1;
  UCHAR DPO : 1;
  UCHAR LogicalUnitNumber : 3;
  UCHAR LogicalBlockByte0;
  UCHAR LogicalBlockByte1;
  UCHAR LogicalBlockByte2;
  UCHAR LogicalBlockByte3;
  UCHAR Reserved2;
  UCHAR TransferBlocksMSB;
  UCHAR TransferBlocksLSB;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _REQ_SENSE {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2;
  UCHAR Reserved3;
  UCHAR TransferLength;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _PAUSE_RESUME {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2[6];
  UCHAR Action;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _READ_TOC {
  UCHAR OperationCode;
  UCHAR Reserved0 : 1;
  UCHAR Msf : 1;
  UCHAR Reserved1 : 3;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2[4];
  UCHAR StartingTrack;
  UCHAR AllocationLength[2];
  UCHAR Control : 6;
  UCHAR Format : 2;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _READ_CD_DA {
  UCHAR OperationCode;
  UCHAR CD_DA_Lun;
  UCHAR LogicalStartAddress[4];
  UCHAR CD_DA_TransferLength[4];
  UCHAR CD_DA_SubCode;
  UCHAR CD_DA_Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _PLAY_AUDIO_MSF {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2;
  UCHAR StartingM;
  UCHAR StartingS;
  UCHAR StartingF;
  UCHAR EndingM;
  UCHAR EndingS;
  UCHAR EndingF;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _PLAY_AUDIO_LBA {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR LbaStart[4];
  UCHAR Extent[4];
  UCHAR LBA_Reserved2;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _PLAY_LBA10 {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Lba10_Start[4];
  UCHAR Lba10_Reserved2;
  UCHAR Lba10_Extent[2];
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _SUBCHANNEL {
  UCHAR OperationCode;
  UCHAR Reserved0 : 1;
  UCHAR Msf : 1;
  UCHAR Reserved1 : 3;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2 : 6;
  UCHAR SubQ : 1;
  UCHAR Reserved3 : 1;
  UCHAR Format;
  UCHAR Reserved4[2];
  UCHAR TrackNumber;
  UCHAR AllocationLength[2];
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _MODE_SENSE {
  UCHAR OperationCode;
  UCHAR Reserved1 : 3;
  UCHAR Dbd : 1;
  UCHAR Reserved2 : 1;
  UCHAR LogicalUnitNumber : 3;
  UCHAR PageCode : 6;
  UCHAR Pc : 2;
  UCHAR Reserved3;
  UCHAR AllocationLength;
  UCHAR Control;
};
#pragma pack(pop)
#if _WIN32_WINDOWS >= 0x0410
#pragma pack(push, 1)
struct _MODE_SENSE10 {
  UCHAR OperationCode;
  UCHAR MS10Lun;
  UCHAR PageCode;
  UCHAR MS10Reserved[4];
  UCHAR MS10AllocationLen[2];
  UCHAR MS10Reserved2;
};
#pragma pack(pop)
#endif
#pragma pack(push, 1)
struct _MODE_SELECT {
  UCHAR OperationCode;
  UCHAR SPBit : 1;
  UCHAR Reserved1 : 3;
  UCHAR PFBit : 1;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2[2];
  UCHAR ParameterListLength;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _LOCATE {
  UCHAR OperationCode;
  UCHAR Reserved1 : 1;
  UCHAR CPBit : 1;
  UCHAR Reserved2 : 6;
  UCHAR Reserved3;
  UCHAR LogicalBlockAddress[4];
  UCHAR Reserved4;
  UCHAR Partition;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _LOGSENSE {
  UCHAR OperationCode;
  UCHAR Unknown1;
  UCHAR PageCode : 6;
  UCHAR PCBit : 2;
  UCHAR Unknown2;
  UCHAR Unknown3;
  UCHAR ParameterPointer[2];
  UCHAR AllocationLength[2];
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _PRINT {
  UCHAR OperationCode;
  UCHAR Reserved : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR TransferLength[3];
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _SEEK {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR LogicalBlockAddress[4];
  UCHAR Reserved2[3];
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _START_STOP {
  UCHAR OperationCode;
  UCHAR Immediate : 1;
  UCHAR Reserved1 : 4;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2[2];
  UCHAR Start : 1;
  UCHAR LoadEject : 1;
  UCHAR Reserved3 : 6;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _MEDIA_REMOVAL {
  UCHAR OperationCode;
  UCHAR Reserved1 : 5;
  UCHAR LogicalUnitNumber : 3;
  UCHAR Reserved2[2];
  UCHAR Prevent;
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _PARTITION {
  UCHAR OperationCode;
  UCHAR Immediate : 1;
  UCHAR Sel : 1;
  UCHAR PartitionSelect : 6;
  UCHAR Reserved1[3];
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _WRITE_TAPE_MARKS {
  UCHAR OperationCode;
  UCHAR Immediate : 1;
  UCHAR WriteSetMarks : 1;
  UCHAR Reserved : 3;
  UCHAR LogicalUnitNumber : 3;
  UCHAR TransferLength[3];
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _SPACE_TAPE_MARKS {
  UCHAR OperationCode;
  UCHAR Control : 3;
  UCHAR Reserved : 5;
  UCHAR NumMarksMSB;
  UCHAR NumMarks;
  UCHAR NumMarksLSB;
  union {
    UCHAR value;
    struct {
      UCHAR Link : 1;
      UCHAR Flag : 1;
      UCHAR Reserved : 4;
      UCHAR VendorUnique : 2;
    } Fields;
  } Byte6;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _READ_POSITION {
  UCHAR Operation;
  UCHAR BlockType : 1;
  UCHAR Reserved1 : 4;
  UCHAR Lun : 3;
  UCHAR Reserved2[7];
  UCHAR Control;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _CDB6READWRITETAPE {
  UCHAR OperationCode;
  UCHAR VendorSpecific : 5;
  UCHAR Reserved : 3;
  UCHAR TransferLenMSB;
  UCHAR TransferLen;
  UCHAR TransferLenLSB;
  UCHAR Link : 1;
  UCHAR Flag : 1;
  UCHAR Reserved1 : 4;
  UCHAR VendorUnique : 2;
};
#pragma pack(pop)
#pragma pack(push, 1)
#if _WIN32_WINDOWS >= 0x0410
union _CDB {
  struct _CDB6GENERIC CDB6GENERIC;
  struct _CDB6GENERIC *PCDB6GENERIC;
  struct _CDB6READWRITE CDB6READWRITE;
  struct _CDB6READWRITE *PCDB6READWRITE;
  struct _CDB6INQUIRY CDB6INQUIRY;
  struct _CDB6INQUIRY *PCDB6INQUIRY;
  struct _CDB6FORMAT CDB6FORMAT;
  struct _CDB6FORMAT *PCDB6FORMAT;
  struct _CDB6SENDDIAG CDB6SENDDIAG;
  struct _CDB6SENDDIAG *PCDB6SENDDIAG;
  struct _CDB6RECDIAG CDB6RECDIAG;
  struct _CDB6RECDIAG *PCDB6RECDIAG;
  struct _CDB10 CDB10;
  struct _CDB10 *PCDB10;
  struct _READ_CAPACITY CDB10READ_CAPACITY;
  struct _READ_CAPACITY *PCDB10READ_CAPACITY;
  struct _CDB10READWRITE CDB10READWRITE;
  struct _CDB10READWRITE *PCDB10READWRITE;
  struct _REQ_SENSE REQ_SENSE;
  struct _REQ_SENSE *PREQ_SENSE;
  struct _PAUSE_RESUME PAUSE_RESUME;
  struct _PAUSE_RESUME *PPAUSE_RESUME;
  struct _READ_TOC READ_TOC;
  struct _READ_TOC *PREAD_TOC;
  struct _READ_CD_DA READ_CD_DA;
  struct _READ_CD_DA *PREAD_CD_DA;
  struct _PLAY_AUDIO_MSF PLAY_AUDIO_MSF;
  struct _PLAY_AUDIO_MSF *PPLAY_AUDIO_MSF;
  struct _PLAY_AUDIO_LBA PLAY_AUDIO_LBA;
  struct _PLAY_AUDIO_LBA *PPLAY_AUDIO_LBA;
  struct _PLAY_LBA10 PLAY_LBA10;
  struct _PLAY_LBA10 *PPLAY_LBA10;
  struct _SUBCHANNEL SUBCHANNEL;
  struct _SUBCHANNEL *PSUBCHANNEL;
  struct _MODE_SENSE MODE_SENSE;
  struct _MODE_SENSE *PMODE_SENSE;
  struct _MODE_SENSE10 MODE_SENSE10;
  struct _MODE_SENSE10 *PMODE_SENSE10;
  struct _MODE_SELECT MODE_SELECT;
  struct _MODE_SELECT *PMODE_SELECT;
  struct _LOCATE LOCATE;
  struct _LOCATE *PLOCATE;
  struct _LOGSENSE LOGSENSE;
  struct _LOGSENSE *PLOGSENSE;
  struct _PRINT PRINT;
  struct _PRINT *PPRINT;
  struct _SEEK SEEK;
  struct _SEEK *PSEEK;
  struct _START_STOP START_STOP;
  struct _START_STOP *PSTART_STOP;
  struct _MEDIA_REMOVAL MEDIA_REMOVAL;
  struct _MEDIA_REMOVAL *PMEDIA_REMOVAL;
  struct _PARTITION PARTITION;
  struct _PARTITION *PPARTITION;
  struct _WRITE_TAPE_MARKS WRITE_TAPE_MARKS;
  struct _WRITE_TAPE_MARKS *PWRITE_TAPE_MARKS;
  struct _SPACE_TAPE_MARKS SPACE_TAPE_MARKS;
  struct _SPACE_TAPE_MARKS *PSPACE_TAPE_MARKS;
  struct _READ_POSITION READ_POSITION;
  struct _READ_POSITION *PREAD_POSITION;
  struct _CDB6READWRITETAPE CDB6READWRITETAPE;
  struct _CDB6READWRITETAPE *PCDB6READWRITETAPE;
};
#else
union _CDB {
  struct _CDB6GENERIC CDB6GENERIC;
  struct _CDB6GENERIC *PCDB6GENERIC;
  struct _CDB6READWRITE CDB6READWRITE;
  struct _CDB6READWRITE *PCDB6READWRITE;
  struct _CDB6INQUIRY CDB6INQUIRY;
  struct _CDB6INQUIRY *PCDB6INQUIRY;
  struct _CDB6FORMAT CDB6FORMAT;
  struct _CDB6FORMAT *PCDB6FORMAT;
  struct _CDB6SENDDIAG CDB6SENDDIAG;
  struct _CDB6SENDDIAG *PCDB6SENDDIAG;
  struct _CDB6RECDIAG CDB6RECDIAG;
  struct _CDB6RECDIAG *PCDB6RECDIAG;
  struct _CDB10 CDB10;
  struct _CDB10 *PCDB10;
  struct _READ_CAPACITY CDB10READ_CAPACITY;
  struct _READ_CAPACITY *PCDB10READ_CAPACITY;
  struct _CDB10READWRITE CDB10READWRITE;
  struct _CDB10READWRITE *PCDB10READWRITE;
  struct _REQ_SENSE REQ_SENSE;
  struct _REQ_SENSE *PREQ_SENSE;
  struct _PAUSE_RESUME PAUSE_RESUME;
  struct _PAUSE_RESUME *PPAUSE_RESUME;
  struct _READ_TOC READ_TOC;
  struct _READ_TOC *PREAD_TOC;
  struct _READ_CD_DA READ_CD_DA;
  struct _READ_CD_DA *PREAD_CD_DA;
  struct _PLAY_AUDIO_MSF PLAY_AUDIO_MSF;
  struct _PLAY_AUDIO_MSF *PPLAY_AUDIO_MSF;
  struct _PLAY_AUDIO_LBA PLAY_AUDIO_LBA;
  struct _PLAY_AUDIO_LBA *PPLAY_AUDIO_LBA;
  struct _PLAY_LBA10 PLAY_LBA10;
  struct _PLAY_LBA10 *PPLAY_LBA10;
  struct _SUBCHANNEL SUBCHANNEL;
  struct _SUBCHANNEL *PSUBCHANNEL;
  struct _MODE_SENSE MODE_SENSE;
  struct _MODE_SENSE *PMODE_SENSE;
  struct _MODE_SELECT MODE_SELECT;
  struct _MODE_SELECT *PMODE_SELECT;
  struct _LOCATE LOCATE;
  struct _LOCATE *PLOCATE;
  struct _LOGSENSE LOGSENSE;
  struct _LOGSENSE *PLOGSENSE;
  struct _PRINT PRINT;
  struct _PRINT *PPRINT;
  struct _SEEK SEEK;
  struct _SEEK *PSEEK;
  struct _START_STOP START_STOP;
  struct _START_STOP *PSTART_STOP;
  struct _MEDIA_REMOVAL MEDIA_REMOVAL;
  struct _MEDIA_REMOVAL *PMEDIA_REMOVAL;
  struct _PARTITION PARTITION;
  struct _PARTITION *PPARTITION;
  struct _WRITE_TAPE_MARKS WRITE_TAPE_MARKS;
  struct _WRITE_TAPE_MARKS *PWRITE_TAPE_MARKS;
  struct _SPACE_TAPE_MARKS SPACE_TAPE_MARKS;
  struct _SPACE_TAPE_MARKS *PSPACE_TAPE_MARKS;
  struct _READ_POSITION READ_POSITION;
  struct _READ_POSITION *PREAD_POSITION;
  struct _CDB6READWRITETAPE CDB6READWRITETAPE;
  struct _CDB6READWRITETAPE *PCDB6READWRITETAPE;
};
#endif
#pragma pack(pop)
typedef union _CDB CDB;
typedef union _CDB *PCDB;
#pragma pack(push, 1)
union _EXTENDED_ARGUMENTS {
  struct {
    UCHAR Modifier[4];
  } Modify;
  struct {
    UCHAR TransferPeriod;
    UCHAR ReqAckOffset;
  } Synchronous;
  struct {
    UCHAR Width;
  } Wide;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _SCSI_EXTENDED_MESSAGE {
  UCHAR InitialMessageCode;
  UCHAR MessageLength;
  UCHAR MessageType;
  union _EXTENDED_ARGUMENTS ExtendedArguments;
};
#pragma pack(pop)
typedef struct _SCSI_EXTENDED_MESSAGE SCSI_EXTENDED_MESSAGE;
typedef struct _SCSI_EXTENDED_MESSAGE *PSCSI_EXTENDED_MESSAGE;
#pragma pack(push, 1)
struct _INQUIRYDATA {
  UCHAR DeviceType : 5;
  UCHAR DeviceTypeQualifier : 3;
  UCHAR DeviceTypeModifier : 7;
  UCHAR RemovableMedia : 1;
  UCHAR Versions;
  UCHAR ResponseDataFormat;
  UCHAR AdditionalLength;
  UCHAR Reserved[2];
  UCHAR SoftReset : 1;
  UCHAR CommandQueue : 1;
  UCHAR Reserved2 : 1;
  UCHAR LinkedCommands : 1;
  UCHAR Synchronous : 1;
  UCHAR Wide16Bit : 1;
  UCHAR Wide32Bit : 1;
  UCHAR RelativeAddressing : 1;
  UCHAR VendorId[8];
  UCHAR ProductId[16];
  UCHAR ProductRevisionLevel[4];
  UCHAR VendorSpecific[20];
  UCHAR Reserved3[40];
};
#pragma pack(pop)
typedef struct _INQUIRYDATA INQUIRYDATA;
typedef struct _INQUIRYDATA *PINQUIRYDATA;
#pragma pack(push, 1)
struct _READCAPACITYDATA {
  UCHAR BlockCountByte0;
  UCHAR BlockCountByte1;
  UCHAR BlockCountByte2;
  UCHAR BlockCountByte3;
  UCHAR BlockLengthByte0;
  UCHAR BlockLengthByte1;
  UCHAR BlockLengthByte2;
  UCHAR BlockLengthByte3;
};
#pragma pack(pop)
typedef struct _READCAPACITYDATA READCAPACITYDATA;
typedef struct _READCAPACITYDATA *PREADCAPACITYDATA;
#pragma pack(push, 1)
struct _DIAGPAGE4 {
  UCHAR PageCode : 6;
  UCHAR Reserved1 : 1;
  UCHAR PS : 1;
  UCHAR NumCylindersMSB;
  UCHAR NumCylindersLSB;
  UCHAR NumHeads;
  UCHAR StartingPreCompMSB;
  UCHAR StartingPreCompLSB;
  UCHAR StartingCylRedWrCurrentMSB;
  UCHAR StartingCylRedWrCurrentLSB;
  UCHAR StepRateMSB;
  UCHAR StepRateLSB;
  UCHAR LandingZoneMSB;
  UCHAR LandingZoneLSB;
  UCHAR RPL : 2;
  UCHAR Reserved2 : 6;
  UCHAR RotationalOffset;
  UCHAR Reserved;
  UCHAR RotationRateMSB;
  UCHAR RotationRateLSB;
  UCHAR Reserved3;
  UCHAR Reserved4;
};
#pragma pack(pop)
typedef struct _DIAGPAGE4 DIAGPAGE4;
typedef struct _DIAGPAGE4 *PDIAGPAGE4;
#pragma pack(push, 1)
struct _SENSE_DATA {
  UCHAR ErrorCode : 7;
  UCHAR Valid : 1;
  UCHAR SegmentNumber;
  UCHAR SenseKey : 4;
  UCHAR Reserved : 1;
  UCHAR IncorrectLength : 1;
  UCHAR EndOfMedia : 1;
  UCHAR FileMark : 1;
  UCHAR Information[4];
  UCHAR AdditionalSenseLength;
  UCHAR CommandSpecificInformation[4];
  UCHAR AdditionalSenseCode;
  UCHAR AdditionalSenseCodeQualifier;
  UCHAR FieldReplaceableUnitCode;
  UCHAR SenseKeySpecific[3];
};
#pragma pack(pop)
typedef struct _SENSE_DATA SENSE_DATA;
typedef struct _SENSE_DATA *PSENSE_DATA;
#pragma pack(push, 1)
struct _READ_CAPACITY_DATA {
  ULONG LogicalBlockAddress;
  ULONG BytesPerBlock;
};
#pragma pack(pop)
typedef struct _READ_CAPACITY_DATA READ_CAPACITY_DATA;
typedef struct _READ_CAPACITY_DATA *PREAD_CAPACITY_DATA;
#pragma pack(push, 1)
struct _READ_BLOCK_LIMITS {
  UCHAR Reserved;
  UCHAR BlockMaximumSize[3];
  UCHAR BlockMinimumSize[2];
};
#pragma pack(pop)
typedef struct _READ_BLOCK_LIMITS READ_BLOCK_LIMITS_DATA;
typedef struct _READ_BLOCK_LIMITS *PREAD_BLOCK_LIMITS_DATA;
#pragma pack(push, 1)
struct _MODE_PARAMETER_HEADER {
  UCHAR ModeDataLength;
  UCHAR MediumType;
  UCHAR DeviceSpecificParameter;
  UCHAR BlockDescriptorLength;
};
#pragma pack(pop)
typedef struct _MODE_PARAMETER_HEADER MODE_PARAMETER_HEADER;
typedef struct _MODE_PARAMETER_HEADER *PMODE_PARAMETER_HEADER;
#if _WIN32_WINDOWS >= 0x0410
#pragma pack(push, 1)
struct _MODE10_PARAMETER_HEADER {
  UCHAR Mode10DataLength[2];
  UCHAR Mode10MediumType;
  UCHAR Reserved[3];
  UCHAR Mode10BlockDescriptorLength[2];
};
#pragma pack(pop)
typedef struct _MODE10_PARAMETER_HEADER MODE10_PARAMETER_HEADER;
typedef struct _MODE10_PARAMETER_HEADER *PMODE10_PARAMETER_HEADER;
#endif
#pragma pack(push, 1)
struct _MODE_PARAMETER_BLOCK {
  UCHAR DensityCode;
  UCHAR NumberOfBlocks[3];
  UCHAR Reserved;
  UCHAR BlockLength[3];
};
#pragma pack(pop)
typedef struct _MODE_PARAMETER_BLOCK MODE_PARAMETER_BLOCK;
typedef struct _MODE_PARAMETER_BLOCK *PMODE_PARAMETER_BLOCK;
#pragma pack(push, 1)
struct _MODE_ERROR_RECOVERY_PAGE {
  UCHAR PageCode : 6;
  UCHAR Reserved : 1;
  UCHAR PageSavable : 1;
  UCHAR PageLength;
  UCHAR ErrorRecoveryParam;
  UCHAR ReadRetry;
  UCHAR MERP_Reserved[4];
};
#pragma pack(pop)
typedef struct _MODE_ERROR_RECOVERY_PAGE MODE_ERROR_RECOVERY_PAGE;
typedef struct _MODE_ERROR_RECOVERY_PAGE *PMODE_ERROR_RECOVERY_PAGE;
#pragma pack(push, 1)
struct _MODE_DISCONNECT_PAGE {
  UCHAR PageCode : 6;
  UCHAR Reserved : 1;
  UCHAR PageSavable : 1;
  UCHAR PageLength;
  UCHAR BufferFullRatio;
  UCHAR BufferEmptyRatio;
  UCHAR BusInactivityLimit[2];
  UCHAR BusDisconnectTime[2];
  UCHAR BusConnectTime[2];
  UCHAR MaximumBurstSize[2];
  UCHAR DataTransferDisconnect : 2;
  UCHAR Reserved2[3];
};
#pragma pack(pop)
typedef struct _MODE_DISCONNECT_PAGE MODE_DISCONNECT_PAGE;
typedef struct _MODE_DISCONNECT_PAGE *PMODE_DISCONNECT_PAGE;
#pragma pack(push, 1)
struct _MODE_CACHING_PAGE {
  UCHAR PageCode : 6;
  UCHAR Reserved : 1;
  UCHAR PageSavable : 1;
  UCHAR PageLength;
  UCHAR ReadDisableCache : 1;
  UCHAR MultiplicationFactor : 1;
  UCHAR WriteCacheEnable : 1;
  UCHAR Reserved2 : 5;
  UCHAR WriteRetensionPriority : 4;
  UCHAR ReadRetensionPriority : 4;
  UCHAR DisablePrefetchTransfer[2];
  UCHAR MinimumPrefectch[2];
  UCHAR MaximumPrefectch[2];
  UCHAR MaximumPrefectchCeil[2];
};
#pragma pack(pop)
typedef struct _MODE_CACHING_PAGE MODE_CACHING_PAGE;
typedef struct _MODE_CACHING_PAGE *PMODE_CACHING_PAGE;
#pragma pack(push, 1)
struct _MODE_FLEXIBLE_DISK_PAGE {
  UCHAR PageCode : 6;
  UCHAR Reserved : 1;
  UCHAR PageSavable : 1;
  UCHAR PageLength;
  UCHAR TransferRate[2];
  UCHAR NumberOfHeads;
  UCHAR SectorsPerTrack;
  UCHAR BytesPerSector[2];
  UCHAR NumberOfCylinders[2];
  UCHAR StartWritePrecom[2];
  UCHAR StartReducedCurrent[2];
  UCHAR StepRate[2];
  UCHAR StepPluseWidth;
  UCHAR HeadSettleDelay[2];
  UCHAR MotorOnDelay;
  UCHAR MotorOffDelay;
  UCHAR Reserved2 : 5;
  UCHAR MotorOnAsserted : 1;
  UCHAR StartSectorNumber : 1;
  UCHAR TrueReadySignal : 1;
  UCHAR StepPlusePerCyclynder : 4;
  UCHAR Reserved3 : 4;
  UCHAR WriteCompenstation;
  UCHAR HeadLoadDelay;
  UCHAR HeadUnloadDelay;
  UCHAR Pin2Usage : 4;
  UCHAR Pin34Usage : 4;
  UCHAR Pin1Usage : 4;
  UCHAR Pin4Usage : 4;
  UCHAR MediumRotationRate[2];
  UCHAR Reserved4[2];
};
#pragma pack(pop)
typedef struct _MODE_FLEXIBLE_DISK_PAGE MODE_FLEXIBLE_DISK_PAGE;
typedef struct _MODE_FLEXIBLE_DISK_PAGE *PMODE_FLEXIBLE_DISK_PAGE;
#pragma pack(push, 1)
struct _MODE_FORMAT_PAGE {
  UCHAR PageCode : 6;
  UCHAR Reserved : 1;
  UCHAR PageSavable : 1;
  UCHAR PageLength;
  UCHAR TracksPerZone[2];
  UCHAR AlternetSectorsPerZone[2];
  UCHAR AlternetTracksPerZone[2];
  UCHAR SectorsPerTrack[2];
  UCHAR BytesPerPhysicalSector[2];
  UCHAR Interleave[2];
  UCHAR TrackSkewFactor[2];
  UCHAR CylinderSkewFactor[2];
  UCHAR Reserved2 : 4;
  UCHAR SurfaceFirst : 1;
  UCHAR RemovableMedia : 1;
  UCHAR HardSectorFormating : 1;
  UCHAR SoftSectorFormating : 1;
  UCHAR Reserved3[2];
};
#pragma pack(pop)
typedef struct _MODE_FORMAT_PAGE MODE_FORMAT_PAGE;
typedef struct _MODE_FORMAT_PAGE *PMODE_FORMAT_PAGE;
#pragma pack(push, 1)
struct _MODE_RIGID_GEOMETRY_PAGE {
  UCHAR PageCode : 6;
  UCHAR Reserved : 1;
  UCHAR PageSavable : 1;
  UCHAR PageLength;
  UCHAR NumberOfCylinders[2];
  UCHAR NumberOfHeads;
  UCHAR StartWritePrecom[2];
  UCHAR StartReducedCurrent[2];
  UCHAR DriveStepRate[2];
  UCHAR LandZoneCyclinder[2];
  UCHAR RotationalPositionLock : 2;
  UCHAR Reserved2 : 6;
  UCHAR RotationOffset;
  UCHAR Reserved3;
  UCHAR RoataionRate[2];
  UCHAR Reserved4[2];
};
#pragma pack(pop)
typedef struct _MODE_RIGID_GEOMETRY_PAGE MODE_RIGID_GEOMETRY_PAGE;
typedef struct _MODE_RIGID_GEOMETRY_PAGE *PMODE_RIGID_GEOMETRY_PAGE;
#pragma pack(push, 1)
struct _LOG_SENSE_PAGE_HEADER {
  UCHAR PageCode : 6;
  UCHAR Reserved1 : 2;
  UCHAR Reserved2;
  UCHAR Length[2];
};
#pragma pack(pop)
typedef struct _LOG_SENSE_PAGE_HEADER LOG_SENSE_PAGE_HEADER;
typedef struct _LOG_SENSE_PAGE_HEADER *PLOG_SENSE_PAGE_HEADER;
#pragma pack(push, 1)
struct _LOG_SENSE_PARAMETER_HEADER {
  UCHAR ParameterCode[2];
  UCHAR LPBit : 1;
  UCHAR Reserved1 : 1;
  UCHAR TMCBit : 2;
  UCHAR ETCBit : 1;
  UCHAR TSDBit : 1;
  UCHAR DSBit : 1;
  UCHAR DUBit : 1;
  UCHAR ParameterLength;
};
#pragma pack(pop)
typedef struct _LOG_SENSE_PARAMETER_HEADER LOG_SENSE_PARAMETER_HEADER;
typedef struct _LOG_SENSE_PARAMETER_HEADER *PLOG_SENSE_PARAMETER_HEADER;
#pragma pack(push, 1)
struct _LOG_SENSE_PAGE_INFORMATION {
  union {
    struct {
      UCHAR Page0;
      UCHAR Page2;
      UCHAR Page3;
      UCHAR Page30;
      UCHAR Page31;
    } PageData;
    struct {
      LOG_SENSE_PARAMETER_HEADER Parm1;
      UCHAR TotalRewrites[2];
      LOG_SENSE_PARAMETER_HEADER Parm2;
      UCHAR TotalErrorCorrected[3];
      LOG_SENSE_PARAMETER_HEADER Parm3;
      UCHAR NotApplicable[2];
      LOG_SENSE_PARAMETER_HEADER Parm4;
      UCHAR TotalBytesProcessed[4];
      LOG_SENSE_PARAMETER_HEADER Parm5;
      UCHAR TotalUnrecoverableErrors[2];
      LOG_SENSE_PARAMETER_HEADER Parm6;
      UCHAR RewritesLastReadOp[2];
    } Page2;
    struct {
      LOG_SENSE_PARAMETER_HEADER Parm1;
      UCHAR TotalRereads[2];
      LOG_SENSE_PARAMETER_HEADER Parm2;
      UCHAR TotalErrorCorrected[3];
      LOG_SENSE_PARAMETER_HEADER Parm3;
      UCHAR TotalCorrectableECCC3[2];
      LOG_SENSE_PARAMETER_HEADER Parm4;
      UCHAR TotalBytesProcessed[4];
      LOG_SENSE_PARAMETER_HEADER Parm5;
      UCHAR TotalUnrecoverableErrors[2];
      LOG_SENSE_PARAMETER_HEADER Parm6;
      UCHAR RereadsLastWriteOp[2];
    } Page3;
    struct {
      LOG_SENSE_PARAMETER_HEADER Parm1;
      UCHAR CurrentGroupsWritten[3];
      LOG_SENSE_PARAMETER_HEADER Parm2;
      UCHAR CurrentRewrittenFrames[2];
      LOG_SENSE_PARAMETER_HEADER Parm3;
      UCHAR CurrentGroupsRead[3];
      LOG_SENSE_PARAMETER_HEADER Parm4;
      UCHAR CurrentECCC3Corrections[2];
      LOG_SENSE_PARAMETER_HEADER Parm5;
      UCHAR PreviousGroupsWritten[3];
      LOG_SENSE_PARAMETER_HEADER Parm6;
      UCHAR PreviousRewrittenFrames[2];
      LOG_SENSE_PARAMETER_HEADER Parm7;
      UCHAR PreviousGroupsRead[3];
      LOG_SENSE_PARAMETER_HEADER Parm8;
      UCHAR PreviousECCC3Corrections[2];
      LOG_SENSE_PARAMETER_HEADER Parm9;
      UCHAR TotalGroupsWritten[4];
      LOG_SENSE_PARAMETER_HEADER Parm10;
      UCHAR TotalRewritteFrames[3];
      LOG_SENSE_PARAMETER_HEADER Parm11;
      UCHAR TotalGroupsRead[4];
      LOG_SENSE_PARAMETER_HEADER Parm12;
      UCHAR TotalECCC3Corrections[3];
      LOG_SENSE_PARAMETER_HEADER Parm13;
      UCHAR LoadCount[2];
    } Page30;
    struct {
      LOG_SENSE_PARAMETER_HEADER Parm1;
      UCHAR RemainingCapacityPart0[4];
      LOG_SENSE_PARAMETER_HEADER Parm2;
      UCHAR RemainingCapacityPart1[4];
      LOG_SENSE_PARAMETER_HEADER Parm3;
      UCHAR MaximumCapacityPart0[4];
      LOG_SENSE_PARAMETER_HEADER Parm4;
      UCHAR MaximumCapacityPart1[4];
    } Page31;
  } LogSensePage;
};
#pragma pack(pop)
typedef struct _LOG_SENSE_PAGE_INFORMATION LOG_SENSE_PAGE_INFORMATION;
typedef struct _LOG_SENSE_PAGE_INFORMATION *PLOG_SENSE_PAGE_INFORMATION;
#pragma pack(push, 1)
struct _LOG_SENSE_PARAMETER_FORMAT {
  LOG_SENSE_PAGE_HEADER LogSenseHeader;
  LOG_SENSE_PAGE_INFORMATION LogSensePageInfo;
};
#pragma pack(pop)
typedef struct _LOG_SENSE_PARAMETER_FORMAT LOG_SENSE_PARAMETER_FORMAT;
typedef struct _LOG_SENSE_PARAMETER_FORMAT *PLOG_SENSE_PARAMETER_FORMAT;
#pragma pack(push, 1)
struct _MODE_READ_WRITE_RECOVERY_PAGE {
  UCHAR PageCode : 6;
  UCHAR Unknown1 : 2;
  UCHAR PageLength;
  UCHAR Unknown2;
  UCHAR ReadRetryCount;
  UCHAR Unknown3;
  UCHAR Unknown4;
  UCHAR Unknown5;
  UCHAR Unknown6;
  UCHAR WriteRetryCount;
  UCHAR Unknown7;
  UCHAR Unknown8;
  UCHAR Unknown9;
};
#pragma pack(pop)
typedef struct _MODE_READ_WRITE_RECOVERY_PAGE MODE_READ_WRITE_RECOVERY_PAGE;
typedef struct _MODE_READ_WRITE_RECOVERY_PAGE *PMODE_READ_WRITE_RECOVERY_PAGE;
#pragma pack(push, 1)
struct _MODE_DEVICE_CONFIGURATION_PAGE {
  UCHAR PageCode : 6;
  UCHAR Reserved1 : 1;
  UCHAR PS : 1;
  UCHAR PageLength;
  UCHAR ActiveFormat : 5;
  UCHAR CAFBit : 1;
  UCHAR CAPBit : 1;
  UCHAR Reserved2 : 1;
  UCHAR ActivePartition;
  UCHAR WriteBufferFullRatio;
  UCHAR ReadBufferEmptyRatio;
  UCHAR WriteDelayTime[2];
  UCHAR REW : 1;
  UCHAR RBO : 1;
  UCHAR SOCF : 2;
  UCHAR AVC : 1;
  UCHAR RSmk : 1;
  UCHAR BIS : 1;
  UCHAR DBR : 1;
  UCHAR GapSize;
  UCHAR Reserved3 : 3;
  UCHAR SEW : 1;
  UCHAR EEG : 1;
  UCHAR EODdefined : 3;
  UCHAR BufferSize[3];
  UCHAR DCAlgorithm;
  UCHAR Reserved4;
};
#pragma pack(pop)
typedef struct _MODE_DEVICE_CONFIGURATION_PAGE MODE_DEVICE_CONFIGURATION_PAGE;
typedef struct _MODE_DEVICE_CONFIGURATION_PAGE *PMODE_DEVICE_CONFIGURATION_PAGE;
#pragma pack(push, 1)
struct _MODE_DEVICE_CONFIG_PAGE {
  MODE_PARAMETER_HEADER ParameterListHeader;
  MODE_DEVICE_CONFIGURATION_PAGE DeviceConfigPage;
};
#pragma pack(pop)
typedef struct _MODE_DEVICE_CONFIG_PAGE MODE_DEVICE_CONFIG_PAGE;
typedef struct _MODE_DEVICE_CONFIG_PAGE *PMODE_DEVICE_CONFIG_PAGE;
#pragma pack(push, 1)
struct _PORT_OUTPUT {
  UCHAR ChannelSelection;
  UCHAR Volume;
};
#pragma pack(pop)
typedef struct _PORT_OUTPUT PORT_OUTPUT;
typedef struct _PORT_OUTPUT *PPORT_OUTPUT;
#pragma pack(push, 1)
struct _AUDIO_OUTPUT {
  UCHAR CodePage;
  UCHAR ParameterLength;
  UCHAR Immediate;
  UCHAR Reserved[2];
  UCHAR LbaFormat;
  UCHAR LogicalBlocksPerSecond[2];
  PORT_OUTPUT PortOutput[4];
};
#pragma pack(pop)
typedef struct _AUDIO_OUTPUT AUDIO_OUTPUT;
typedef struct _AUDIO_OUTPUT *PAUDIO_OUTPUT;
#if _WIN32_WINDOWS >= 0x0410
#pragma pack(push, 1)
struct _DEVICE_CAPABILITIES {
  UCHAR PageCode;
  UCHAR PageLen;
  UCHAR ReadCaps;
  UCHAR WriteCaps;
  UCHAR CDCaps1;
  UCHAR CDCaps2;
  UCHAR LoadMech;
  UCHAR ChangeCaps;
  UCHAR DCReserved[2];
  UCHAR VolumeLvls[2];
  UCHAR DevBuffer[2];
  UCHAR DCReserved2[6];
  UCHAR CopyMgmtRev[2];
  UCHAR DCReserved3[2];
};
#pragma pack(pop)
typedef struct _DEVICE_CAPABILITIES DEVICE_CAPABILITIES;
typedef struct _DEVICE_CAPABILITIES *PDEVICE_CAPABILITIES;
#pragma pack(push, 1)
struct _DVD_ERROR_RECOVERY_PAGE {
  UCHAR PageCode;
  UCHAR PageLen;
  UCHAR ErrRecoveryParam;
  UCHAR ReadRetry;
  UCHAR WriteRetry;
  UCHAR VerifySpan;
  UCHAR Grp1TO[2];
  UCHAR Grp2TO[2];
};
#pragma pack(pop)
typedef struct _DVD_ERROR_RECOVERY_PAGE DVD_ERROR_RECOVERY_PAGE;
typedef struct _DVD_ERROR_RECOVERY_PAGE *PDVD_ERROR_RECOVERY_PAGE;
#endif
#pragma pack(push, 1)
struct _TOC_HEADER {
  UCHAR DataLength[2];
  UCHAR FirstTrack;
  UCHAR LastTrack;
};
#pragma pack(pop)
typedef struct _TOC_HEADER TOC_HEADER;
#pragma pack(push, 1)
struct _TOC_TD_ADR_CTRL_REC {
  UCHAR Control : 4;
  UCHAR ADR : 4;
};
#pragma pack(pop)
typedef struct _TOC_TD_ADR_CTRL_REC TOC_TD_ADR_CTRL_REC;
#pragma pack(push, 1)
struct _TOC_TRACK_DESCRIPTOR {
  UCHAR Reserved1;
  TOC_TD_ADR_CTRL_REC toc_ctrl_rec;
  UCHAR TrackID;
  UCHAR ReservedForHell2;
  UCHAR StartAddr[4];
};
#pragma pack(pop)
typedef struct _TOC_TRACK_DESCRIPTOR TOC_TRACK_DESCRIPTOR;
#pragma pack(push, 1)
struct _TOC_SESSION_DESCRIPTOR {
  UCHAR Session;
  TOC_TD_ADR_CTRL_REC toc_ses_ctrl_rec;
  UCHAR TrackNo;
  UCHAR Point;
  UCHAR Min;
  UCHAR Sec;
  UCHAR Frame;
  UCHAR Zero;
  UCHAR PMin;
  UCHAR PSec;
  UCHAR PFrame;
};
#pragma pack(pop)
typedef struct _TOC_SESSION_DESCRIPTOR TOC_SESSION_DESCRIPTOR;
#pragma pack(push, 1)
struct _TAPE_POSITION_DATA {
  UCHAR Reserved1 : 2;
  UCHAR BlockPositionUnsupported : 1;
  UCHAR Reserved2 : 3;
  UCHAR EndOfPartition : 1;
  UCHAR BeginningOfPartition : 1;
  UCHAR PartitionNumber;
  USHORT Reserved3;
  UCHAR FirstBlock[4];
  UCHAR LastBlock[4];
  UCHAR Reserved4;
  UCHAR NumberOfBlocks[3];
  UCHAR NumberOfBytes[4];
};
#pragma pack(pop)
typedef struct _TAPE_POSITION_DATA TAPE_POSITION_DATA;
typedef struct _TAPE_POSITION_DATA *PTAPE_POSITION_DATA;
#pragma pack(push, 1)
struct _UPC_REC {
  UCHAR Reserved2 : 7;
  UCHAR UPCValid : 1;
};
#pragma pack(pop)
typedef struct _UPC_REC UPC_REC;
#pragma pack(push, 1)
struct _ISRC_REC {
  UCHAR Reserved2 : 7;
  UCHAR ISRCValid : 1;
};
#pragma pack(pop)
typedef struct _ISRC_REC ISRC_REC;
#pragma pack(push, 1)
struct _CTRL_ADR_REC {
  UCHAR Control : 4;
  UCHAR ADR : 4;
};
#pragma pack(pop)
typedef struct _CTRL_ADR_REC CTRL_ADR_REC;
#pragma pack(push, 1)
struct _SubQ_Data {
  UCHAR Reserved1;
  UCHAR Audio_Status;
  UCHAR Data_Len[2];
  UCHAR Data_Format;
  CTRL_ADR_REC Subq_Ctrl_ADR_rec;
  UCHAR Track;
  UCHAR Index;
  UCHAR AbsAddr[4];
  UCHAR TrackRelAddr[4];
  UPC_REC subq_upc_rec;
  UCHAR UPCCode[14];
  ISRC_REC subq_isrc_rec;
  UCHAR ISRC[14];
};
#pragma pack(pop)
typedef struct _SubQ_Data SubQ_Data;
typedef struct _SubQ_Data *pSubQ_Data;
#pragma pack(push, 1)
struct _SubQ_pos {
  UCHAR Reserved1;
  UCHAR pos_Audio_Status;
  UCHAR pos_Data_Len[2];
  UCHAR pos_Data_Format;
  CTRL_ADR_REC pos_Subq_Ctrl_ADR_rec;
  UCHAR pos_Track;
  UCHAR pos_Index;
  UCHAR pos_AbsAddr[4];
  UCHAR pos_TrackRelAddr[4];
};
#pragma pack(pop)
typedef struct _SubQ_pos SubQ_pos;
typedef struct _SubQ_pos *pSubQ_pos;
#pragma pack(push, 1)
struct _SubQ_UPC {
  UCHAR upc_Reserved1;
  UCHAR upc_Audio_Status;
  UCHAR upc_Data_Len[2];
  UCHAR upc_Data_Format;
  CTRL_ADR_REC upc_Subq_Ctrl_ADR_rec;
  UCHAR upc_Track;
  UCHAR upc_Index;
  UPC_REC upc_subq_upc_rec;
  UCHAR upc_UPCCode[14];
};
#pragma pack(pop)
typedef struct _SubQ_UPC SubQ_UPC;
typedef struct _SubQ_UPC *pSubQ_UPC;
#pragma pack(push, 1)
struct _SubQ_ISRC {
  UCHAR upc_Reserved1;
  UCHAR upc_Audio_Status;
  UCHAR upc_Data_Len[2];
  UCHAR upc_Data_Format;
  CTRL_ADR_REC upc_Subq_Ctrl_ADR_rec;
  UCHAR upc_Track;
  UCHAR upc_Index;
  ISRC_REC upc_subq_isrc_rec;
  UCHAR upc_ISRC[14];
};
#pragma pack(pop)
typedef struct _SubQ_ISRC SubQ_ISRC;
typedef struct _SubQ_ISRC *pSubQ_ISRC;
#pragma pack(push, 1)
struct _FOUR_BYTE {
  UCHAR Byte0;
  UCHAR Byte1;
  UCHAR Byte2;
  UCHAR Byte3;
};
#pragma pack(pop)
typedef struct _FOUR_BYTE FOUR_BYTE;
typedef struct _FOUR_BYTE *PFOUR_BYTE;

#endif
