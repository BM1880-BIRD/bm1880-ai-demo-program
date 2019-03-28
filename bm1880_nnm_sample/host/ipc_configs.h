#ifndef IPC_CONFIGS_H
#define IPC_CONFIGS_H

// communication error
#define TIMEOUT_ERR -2

// communication settings
#define MAX_CMD_LEN 32
#define MAX_MAGIC_LEN 4

// device settings
#define MAX_DEVNAME_LEN 16
#define MAX_FILENAME_LEN 128

// ipc settings
#define MAX_USERS 16
#define USER_NAME_LEN 20
#define USER_INFO_LEN 20
#define USER_INDEX_START 1
#define MAX_EVENT_COUNT 50

#define PACKET_HEADER_SIZE 16//U8+U32+U8+U8+U8+U64

#define MAX_SIZE_EACH_SEND 1024*1024

#define TRUE  1
#define FALSE 0
#define SUCCESS 1
#define FAILED 0

#define BMLINK_SERVER_SESSION 1

#define XM_BSP_PACKAGE_ID "001001001"
#define XM_APP_PACKAGE_ID "001001002"
#define XM_MODEL_PACKAGE_ID "001001003"

typedef enum
{
	CMD_REGISTER_USER = 0,
	CMD_RECOGNIZE_IMAGE,
	CMD_DELETE_USER,
	CMD_SEARCH_USER,
	CMD_ENUMERATE_USER,
	CMD_GET_IMAGE,
	CMD_SEND_FILE,
	CMD_GET_FW_VER,
	CMD_UPGRADE_INIT,
	CMD_UPGRADE_SUBFILEID,
	CMD_UPGRADE_SUBEOF,
	CMD_UPGRADE_ABORT,
	CMD_DEVICE_UPGRADE,
} ProtocolCmdType;


typedef enum
{
	FORMAT_MISC = 0,
	FORMAT_JPEG = 1,
	FORMAT_RAW = 2,
	FORMAT_YUV420 = 3,
} FormatType;

typedef enum
{
	OK,
	BAD_IMAGE_QUALITY,
	STORAGE_FULL,
	USER_ID_NOT_EXIST,
	DETECT_FAIL,
	DEVICE_CLOSED,
	RECV_ERROR,
	EMPTY_NAME,
	NO_FACE,
	BAD_INPUT,
	READY_DO_UPGRADE,
	UPGRADE_FAILED,
} IpcRetCode;

typedef enum
{
	SEND_ONETIME,
	SEND_MULTI_START,
	SEND_MULTI_CONTINUE,
	SEND_MULTI_END,
} SendFileStatus;

typedef enum
{
	BitMain_UpgradeNormal,//porcessing 0~99% 
	BitMain_UpgradeError,
	BitMain_UpgradeSuccess,
}BitMainUpgradeStatus;

typedef enum
{
	BitMain_BspVer,
	BitMain_AppVer,
	BitMain_BModelVer,
} BitMainVer;

typedef struct bm_usb_header_tx_packet
{
	uint8_t magic[MAX_MAGIC_LEN];
	uint32_t data_len;
	uint8_t cmd[MAX_CMD_LEN];
	uint8_t type;
	uint8_t ret_code;
} bm_usb_header_tx_pkt;

typedef struct bm_usb_header_tx_packet_2
{
	uint8_t  magic;
	uint32_t data_len;
	uint8_t  cmd;
	uint8_t  data_type;
	uint8_t  ret_code;
	uint64_t timestamp;
}  __attribute__((packed)) bm_usb_header_tx_pkt_2;

typedef struct bm_usb_ack_packet
{
	uint8_t magic[MAX_MAGIC_LEN];
} bm_usb_ack_pkt;

typedef IpcRetCode RetCode;
typedef struct _BitMainUsr
{
	uint32_t user_id;
	char user_name[USER_NAME_LEN];
	char user_info[USER_INFO_LEN];
} __attribute__((packed)) BitMainUsr;

typedef struct _BMVerTime
{
	uint16_t tm_year;
	uint16_t tm_mon;
	uint16_t tm_mday;
	uint16_t reserved[5];
} __attribute__((packed)) BMVerTime;

typedef struct _BMFWVersion
{
	uint8_t   fw_type;
	BMVerTime ptime;
	char version[16];
} __attribute__((packed)) BMFWVersion;

typedef struct _BMUpgradeInit
{
	uint16_t  filenums;
	uint32_t  filesize;
	uint8_t   fwtype;
	uint8_t   ret;
} __attribute__((packed)) BMUpgradeInit;

typedef struct _BMSubFiles
{
	uint16_t subfileid;
	uint16_t percentage;
} __attribute__((packed)) BMSubFiles;

typedef struct _BMSendFiles
{
	uint32_t size;
	uint8_t  checksum_en;
	uint8_t  checksum;
	uint8_t  continue_send;
	uint16_t data_offset; 
} __attribute__((packed)) BMSendFiles;

typedef struct _BMEvent
{
	uint16_t user_id;
	uint64_t start_time;
	uint64_t end_time;
} __attribute__((packed)) BMEvent;

typedef struct _BMFDResult
{
	uint16_t  reg_user_num;
	uint16_t  unknown_num;
	uint16_t  confidence;
	uint16_t  user_ids[MAX_USERS];
} __attribute__((packed)) BMFDResult;

#define SEC_MASK 0x3F
#define MIN_MASK 0xFC0
#define HOUR_MASK 0x1F000
#define DAY_MASK 0x3E0000
#define MONTH_MASK 0x3C00000
#define YEAR_MASK 0xFC000000

//#define DEBUG_MSG
//#define PROFILE_PERFORMANCE

void parse_timestamp(uint64_t timestamp);

#endif // IPC_CONFIGS_H
