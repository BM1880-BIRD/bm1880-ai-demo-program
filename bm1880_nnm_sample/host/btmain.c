#include "btmain.h"
#include "bm_usbtty.h"
#include "bm_link.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

static bool used[MAX_USERS];
static unsigned int user_count = 0;
static bmlink_t kBMLink;
static bool kIsInitialized = 0;

static upgradeStatusCB status_cbFun=NULL;


#define null 0

#define TRY_LOCK                                     \
    int _ret = 0;                                    \
    if ((_ret = pthread_mutex_lock(&lib_lock)) != 0) \
    {                                                \
        fprintf(stderr, "can't lock mutex\n");                \
        return;                                      \
    }

#define UNLOCK_AND_RET               \
    pthread_mutex_unlock(&lib_lock); \
    return;

#define CHECK_RECV_HEADER(ret)          \
    if (ret)                            \
    {                                   \
        fprintf(stderr, "recv header failed\n"); \
        UNLOCK_AND_RET                  \
    }

#define CHECK_RECV_BUFFER(ret)          \
    if (ret)                            \
    {                                   \
        fprintf(stderr, "recv buffer failed\n"); \
        UNLOCK_AND_RET                  \
    }

pthread_mutex_t lib_lock;

// PAYLOAD_LENGTH + 40 + 1280 * 720 * 3 / 2
unsigned char g_buffer[1382568];

#ifdef BMLINK
uint16_t SendUSBData(uint32_t local_id, uint8_t cmd, uint8_t type, uint64_t id, uint32_t size)
{
    uint32_t ret;

    bm_usb_header_tx_pkt_2 sendpkt;
    sendpkt.cmd = cmd;
    sendpkt.data_type = type;
    sendpkt.data_len = size;
    sendpkt.timestamp = id;

    ret = BMLinkSessionWrite(local_id, BMLINK_SERVER_SESSION, &sendpkt, PACKET_HEADER_SIZE, 0, 0, 0);

    if(ret!=PACKET_HEADER_SIZE) {
	printf("usb write error!\n");
	return FAILED;
    }
    else if(size>0){
	ret = BMLinkSessionWrite(local_id, BMLINK_SERVER_SESSION, g_buffer, size, 0, 0, 0);
	if(ret==size)
	    return SUCCESS;
	else
	    return FAILED;
    }
    else
	return SUCCESS;
}

uint16_t RecvUSBData(uint32_t local_id, uint32_t *recv_size)
{
    uint32_t ret;
    bm_usb_header_tx_pkt_2 rcvpkt;
    int remote_id = BMLINK_SERVER_SESSION;

    ret = BMLinkSessionRead(local_id, &remote_id, &rcvpkt, PACKET_HEADER_SIZE, -1);

    if(ret == PACKET_HEADER_SIZE) {
	if(rcvpkt.data_len > 0) {
	    ret = BMLinkSessionRead(local_id, &remote_id, g_buffer, rcvpkt.data_len, -1);
	    if(ret == rcvpkt.data_len) {
	        *recv_size = rcvpkt.data_len;
	        printf("ret=%d, rcvpkt.data_len=%d\n", ret, rcvpkt.data_len);
    	        return rcvpkt.ret_code;
	    }
	    else {
	        printf("usb read error!!!\n");
		return RECV_ERROR;
	    }

	}
    	return rcvpkt.ret_code;
    }
    else
	return RECV_ERROR;
}

#else
uint16_t SendUSBData(uint32_t local_id, uint8_t cmd, uint8_t type, uint64_t id, uint32_t size)
{
    uint32_t ret;

    bm_usb_header_tx_pkt_2 sendpkt;
    sendpkt.cmd = cmd;
    sendpkt.data_type = type;
    sendpkt.data_len = size;
    sendpkt.timestamp = id;

    //ret = BMLinkSessionWrite(local_id, BMLINK_SERVER_SESSION, &sendpkt, PACKET_HEADER_SIZE, 0, 0, 0);
    ret = tty_write_buffer(&sendpkt, PACKET_HEADER_SIZE);

    if(ret!=PACKET_HEADER_SIZE) {
	printf("usb write error!\n");
	return FAILED;
    }
    else if(size>0){
	//ret = BMLinkSessionWrite(local_id, BMLINK_SERVER_SESSION, g_buffer, size, 0, 0, 0);
	ret = tty_write_buffer(g_buffer, size);
	if(ret==size)
	    return SUCCESS;
	else
	    return FAILED;
    }
    else
	return SUCCESS;
}

uint16_t RecvUSBData(uint32_t local_id, uint32_t *recv_size)
{
    uint32_t ret;
    bm_usb_header_tx_pkt_2 rcvpkt;
    //int remote_id = BMLINK_SERVER_SESSION;

    //ret = BMLinkSessionRead(local_id, &remote_id, &rcvpkt, PACKET_HEADER_SIZE, -1);
    ret = tty_recv_buffer_pkt(&rcvpkt, PACKET_HEADER_SIZE, 3);

    if(ret == PACKET_HEADER_SIZE) {
	if(rcvpkt.data_len > 0) {
	    //ret = BMLinkSessionRead(local_id, &remote_id, g_buffer, rcvpkt.data_len, -1);
	    ret = tty_recv_buffer_pkt(g_buffer, rcvpkt.data_len, 3);
	    if(ret == rcvpkt.data_len) {
	        *recv_size = rcvpkt.data_len;
	        printf("ret=%d, rcvpkt.data_len=%d\n", ret, rcvpkt.data_len);
    	        return rcvpkt.ret_code;
	    }
	    else {
	        printf("usb read error!!!\n");
		return RECV_ERROR;
	    }

	}
    	return rcvpkt.ret_code;
    }
    else
	return RECV_ERROR;
}

#endif

int btmain_probe(void)
{
    return 0;
}

#ifdef BMLINK
void try_open_device(void)
{
	int ret;
	
	if (kIsInitialized)
		return;

	ret = BMLinkSingleInit(2, 2, 2, 2);
	if (ret != 0)
		fprintf(stderr, "\n BMLinkSingleInit failed \n");

	ret = BMLinkOpen(TTYACM_PATH, BMLINK_TTYUSB, NULL, &kBMLink);
	if (ret != 0)
		fprintf(stderr, "\n BMLinkOpen failed \n");

	kIsInitialized = 1;
}
#else
void try_open_device(void)
{
    if (tty_is_opened()) {
        return;
    }

    TRY_LOCK

    if (!tty_is_opened()) {
        tty_open_device((char *)"/dev/ttyACM0", 4000000);
        if (!tty_is_opened()) {
            tty_open_device((char *)"/dev/ttyACM1", 4000000);
        }

        if (!tty_is_opened()) {
            fprintf(stderr, "can't open usb device\n");
            UNLOCK_AND_RET
        }

        tty_flush();
    }

    UNLOCK_AND_RET
}
#endif

void btmain_init(void)
{
    printf("XXXXXXXXXXXXXXX  btmain_init XXXXXXXXXXXXXXXX\n");
    wait_module_connected();
    try_open_device();
    memset(used, 0, sizeof(used));
    pthread_mutex_init(&lib_lock, NULL);
}

void btmain_uninit(void)
{
        printf("XXXXXXXXXXXXXXX  btmain_uninit XXXXXXXXXXXXXXXX\n");
	BMLinkClose(&kBMLink);
	kIsInitialized = 0;
}

int generate_user_id(void)
{
    for (int i = USER_INDEX_START; i < MAX_USERS; ++i)
    {
        if (!used[i])
        {
            return i;
        }
    }
    return -1;
}

/*
    input: id + name + additional_info + image
    output: n/a
*/
void register_user(uint64_t session_id, uint32_t image_size, unsigned char *image, char *name, char *additional_info, register_cb callback)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size;
	
    try_open_device();

    if (user_count > MAX_USERS)
    {
        if (callback)
        {
            callback(session_id, STORAGE_FULL, 0);
        }
        return;
    }

    if (strlen(name) == 0)
    {
        if (callback)
        {
            callback(session_id, EMPTY_NAME, 0);
        }
        return;
    }

    TRY_LOCK

    // generate id
    int user_id = generate_user_id();
    assert(user_id >= 0 && user_id < MAX_USERS);

    unsigned int buffer_offset = 0;
    BitMainUsr *puser = (BitMainUsr *)g_buffer;
    memset(puser, 0, sizeof(BitMainUsr));
    puser->user_id = user_id;
    memcpy(puser->user_name, name, strlen(name));
    memcpy(puser->user_info, additional_info, strlen(additional_info));
    buffer_offset = sizeof(BitMainUsr);
     
    memcpy(g_buffer + buffer_offset, image, image_size);
    buffer_offset += image_size;

#ifdef BMLINK
	// Initiate communication
	ret_code = RECV_ERROR;
	ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
	if (ret == 0)
#endif
	{
    		ret = SendUSBData(local_id, CMD_REGISTER_USER, FORMAT_JPEG, 0, buffer_offset);
		if (ret == SUCCESS)
		    ret_code = RecvUSBData(local_id, &recv_size);
#ifdef BMLINK
		BMLinkSessionClose(local_id);
#endif
	}

    if (ret_code == OK)
    {
	printf("ret_code is OK\n");
        assert(user_id >= 0 && user_id < MAX_USERS);
        used[user_id] = true;
        ++user_count;
    }

    if (callback)
    {
        callback(session_id, ret_code, user_id);
    }
    UNLOCK_AND_RET
}

void parse_timestamp(uint64_t timestamp)
{
#if defined(DEBUG_MSG)
    unsigned int sec = (timestamp & SEC_MASK);
    unsigned int min = (timestamp & MIN_MASK) >> 6;
    unsigned int hour = (timestamp & HOUR_MASK) >> 12;
    unsigned int day = (timestamp & DAY_MASK) >> 17;
    unsigned int month = (timestamp & MONTH_MASK) >> 22;
    unsigned int year = ((timestamp & YEAR_MASK) >> 26) + 2000;
    printf("time %d-%d-%d %d:%d:%d\n", year, month, day, hour, min, sec);
#endif
}

/*
    input: long long (4 bytes) + image
    output: family count (4 bytes) + stranger count (4 bytes) + confidence (4 bytes) + families (count x sizeof(unsigned int))
*/
void recognize_image(uint64_t session_id, uint32_t image_size, unsigned char *image, detect_cb callback)
{
	int ret, local_id=0;
	unsigned int ret_code;
    uint32_t recv_size;

    printf("recognize_image timestamp %lld\n", session_id);
    parse_timestamp(session_id);

#if defined(PROFILE_PERFORMANCE)
    struct timeval timestamp1;
    gettimeofday(&timestamp1, NULL);
#endif

    try_open_device();

    memcpy(g_buffer, image, image_size);

    TRY_LOCK

#ifdef BMLINK
	// Initiate communication
	ret_code = RECV_ERROR;
	ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
	if (ret == 0)
#endif
	{
    		ret = SendUSBData(local_id, CMD_RECOGNIZE_IMAGE, FORMAT_YUV420, session_id, image_size);
		if (ret == SUCCESS)
		    ret_code = RecvUSBData(local_id, &recv_size);
#ifdef BMLINK
		BMLinkSessionClose(local_id);
#endif
	}


    unsigned int family_count = 0;
    unsigned int stranger_count = 0;
    unsigned int confidence = 0;
    unsigned int *families = null;

#if defined(PROFILE_PERFORMANCE)
    struct timeval timestamp2;
    gettimeofday(&timestamp2, NULL);
    float costime = (timestamp2.tv_sec - timestamp1.tv_sec) * 1000000.0 + timestamp2.tv_usec - timestamp1.tv_usec;
    printf("recognize_image cost %f\n", costime);
#endif

    if (ret_code == OK)
    {
	BMFDResult *pFDResult = (BMFDResult *)g_buffer;	
	family_count = pFDResult->reg_user_num;
	stranger_count = pFDResult->unknown_num;
	confidence = pFDResult->confidence;
        families = (uint16_t *)pFDResult->user_ids;

	printf("fc/sc/conf/familes[%d,%d,%d]\n", family_count, stranger_count, confidence);
    }

    if (callback)
    {
        callback(session_id, ret_code, family_count, stranger_count, confidence, families);
    }
    UNLOCK_AND_RET
}

/*
    input: user id (4 bytes)
    output: n/a
*/
void delete_user(uint64_t session_id, uint32_t user_id, delete_cb callback)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size;
	
    printf("delete_user %d\n", user_id);

    try_open_device();

    if (user_count == 0 || user_id >= MAX_USERS || !used[user_id])
    {
        fprintf(stderr, "no user existed\n");
        if (callback)
        {
            callback(session_id, USER_ID_NOT_EXIST);
        }
        return;
    }

    TRY_LOCK

    BitMainUsr *puser = (BitMainUsr *)g_buffer;
    puser->user_id = user_id;

#ifdef BMLINK
	// Initiate communication
	ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
	if (ret == 0)
#endif
	{
    		ret = SendUSBData(local_id, CMD_DELETE_USER, 0, 0, sizeof(BitMainUsr));
		if (ret == SUCCESS) {
		    ret_code = RecvUSBData(local_id, &recv_size);
		}
#ifdef BMLINK
		BMLinkSessionClose(local_id);
#endif
	}

    printf("delete user ret %d\n", ret_code);
    if (ret_code == OK)
    {
        used[user_id] = false;
        --user_count;
	printf("user_id=%d, user_count=%d\n", user_id, user_count);
    }

    if (callback)
    {
        callback(session_id, ret_code);
    }
    UNLOCK_AND_RET
}

/*
    input: user id (4 bytes) + begin time (8 bytes) + end time (8 bytes)
    output: event number (4 bytes) + events (count x sizeof(long long))
*/
void search_user(uint64_t session_id, uint32_t user_id, uint64_t begin, uint64_t end, search_cb callback)
{
	int ret, local_id=0;
	unsigned int ret_code;
	
    printf("search_user %d\n", user_id);

    try_open_device();

    if (user_count == 0 || user_id >= MAX_USERS || !used[user_id])
    {
        printf("no user existed\n");
        if (callback)
        {
            callback(session_id, USER_ID_NOT_EXIST, 0, null);
        }
        return;
    }

    TRY_LOCK

    uint32_t recv_size=0;
    BMEvent *pevent = (BMEvent *)g_buffer;
    pevent->user_id = user_id;
    pevent->start_time = begin;
    pevent->end_time = end;

#ifdef BMLINK
	// Initiate communication
	ret_code = RECV_ERROR;
	ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
	if (ret == 0)
#endif
	{
    		ret = SendUSBData(local_id, CMD_SEARCH_USER, 0, 0, sizeof(BMEvent));
		if (ret == SUCCESS) {
		    ret_code = RecvUSBData(local_id, &recv_size);
		}
#ifdef BMLINK
		BMLinkSessionClose(local_id);
#endif
	}

    uint64_t *events = null;
    int event_num = 0;

    if (ret_code == OK)
    {
	printf("ret_code==OK, recv_size=%d, sizeof(long long)=%d\n", recv_size, sizeof(uint64_t));
        event_num = recv_size/sizeof(uint64_t);
        events = (uint64_t *)g_buffer;
    }

    printf("search_user ret %d got %d events\n", ret_code, event_num);

#if defined(DEBUG_MSG)
    for (int i = 0; i < event_num; ++i)
        parse_timestamp(*(events + i));
#endif

    if (callback) {
        unsigned int count = event_num > MAX_EVENT_COUNT ? MAX_EVENT_COUNT : event_num;
        callback(session_id, ret_code, count, events);
    }
    UNLOCK_AND_RET
}

/*
    input: n/a
    output: count (4 bytes) + info (count * sizeof(BitMainUsr))
*/
void enumerate_users(uint64_t session_id, enumerate_cb callback)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size=0;
	
    printf("-------- enumerate_users --------\n");

    try_open_device();

    TRY_LOCK

#ifdef BMLINK
	// Initiate communication
	ret_code = RECV_ERROR;
	ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
	if (ret == 0)
#endif
	{
    		ret = SendUSBData(local_id, CMD_ENUMERATE_USER, 0, 0, 0);
		printf("SendUSBData ret=%d\n", ret);
		if (ret == SUCCESS) {
		    ret_code = RecvUSBData(local_id, &recv_size);
			printf("RecvUSBData ret_code=%d, recv_size=%d\n", ret_code, recv_size);
		}
#ifdef BMLINK
		BMLinkSessionClose(local_id);
#endif
	}

    BitMainUsr *info = null;

    if (ret_code == OK) {
        user_count = recv_size/sizeof(BitMainUsr);
        info = (BitMainUsr *)g_buffer;
        printf("!!!enumerate_users. recv_size=%d, got code %d, %d users\n", recv_size, ret_code, user_count);
    }

    for (int i = 0; i < user_count; ++i) {
        printf("load user id %d name %s info %s\n", (info + i)->user_id, (info + i)->user_name, (info + i)->user_info);
        used[(info + i)->user_id] = true;
    }

    if (callback)
        callback(session_id, ret_code, user_count, info);

    UNLOCK_AND_RET
}

/*
    input: user id (4 bytes)
    output: image size (4 bytes)
            image (jpg)
*/
void get_user_image(uint64_t session_id, uint32_t user_id, get_user_image_cb callback)
{
    int ret, local_id=0;
    unsigned int ret_code;
   uint32_t recv_size;
	
    printf("get_user_image %d\n", user_id);

    try_open_device();

    if (user_count == 0 || user_id >= MAX_USERS || !used[user_id]) {
        fprintf(stderr, "user doesn't existed\n");
        if (callback)
            callback(session_id, USER_ID_NOT_EXIST, 0, null);
        return;
    }

    TRY_LOCK

    BitMainUsr *puser = (BitMainUsr *)g_buffer;
    puser->user_id = user_id;

#ifdef BMLINK
	// Initiate communication
	ret_code = RECV_ERROR;
	ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
	if(ret == 0)
#endif
	{
    		ret = SendUSBData(local_id, CMD_GET_IMAGE, 0, 0, sizeof(BitMainUsr));
		if (ret == SUCCESS) {
		    ret_code = RecvUSBData(local_id, &recv_size);
		}
#ifdef BMLINK
		BMLinkSessionClose(local_id);
#endif
	}

    // handle response
    unsigned int image_size = 0;
    void *images = null;

    printf("get_user_image got %d\n", ret_code);

    if (ret_code == OK) {
        image_size = recv_size;
        images = g_buffer;
    }

    if (callback)
        callback(session_id, ret_code, image_size, images);

    UNLOCK_AND_RET
}

void getVersionInfo(uint64_t session_id, BitMainVer ver, BMVerTime *ptime, char* strversion)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size;
    printf("----------- getVersionInfo  BitMainVer=%d------------------\n", ver);

    try_open_device();

    TRY_LOCK

    BMFWVersion *pdata = (BMFWVersion*) g_buffer;
    pdata->fw_type = ver;

#ifdef BMLINK
    // Initiate communication
    ret_code = RECV_ERROR;
    ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
    if (ret == 0)
#endif
    {
    	ret = SendUSBData(local_id, CMD_GET_FW_VER, 0, 0, sizeof(BMFWVersion));
	if (ret == SUCCESS) {
	   ret_code = RecvUSBData(local_id, &recv_size);
	}
#ifdef BMLINK
	BMLinkSessionClose(local_id);
#endif
    }

    if (ret_code == OK) {

	BMFWVersion *pdata_new = (BMFWVersion *)(g_buffer);

	memcpy(ptime, &pdata_new->ptime, sizeof(BMVerTime));
	memcpy(strversion, &pdata_new->version, sizeof(pdata_new->version));

	printf("fw_type = %d, date=[%d/%d/%d], ver=%s\n", pdata_new->fw_type,\
						pdata_new->ptime.tm_year,\
						pdata_new->ptime.tm_mon,\
						pdata_new->ptime.tm_mday,\
						pdata_new->version);
    }
    UNLOCK_AND_RET
}

void putUpgradeData(uint64_t session_id, unsigned char *image, uint32_t image_size)
{
    int ret, local_id=0;
    uint32_t recv_size;

    printf("-------------- getVersionInfo  sendfile size=%d------------------\n", image_size);

    parse_timestamp(session_id);

#if defined(PROFILE_PERFORMANCE)
    struct timeval timestamp1;
    gettimeofday(&timestamp1, NULL);
#endif

    try_open_device();

    BMSendFiles *pdata = (BMSendFiles*) g_buffer;

#if 0//for checksum
    unsigned char checksum=0;
    for(i=0;i<image_size;i++) {
	checksum ^= image[i];
    }
    //printf("checksum=%x\n", checksum);
#else
    pdata->checksum = 0xff;
#endif


    TRY_LOCK
 
	int ret_code = RECV_ERROR;
#ifdef BMLINK
	// Initiate communication
	ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
	if (ret == 0)
#endif
	{
	    // copy image
	    unsigned long buffer_offset_tmp;
	    unsigned char *image_tmp;
	    unsigned long leave_size = image_size;
	    image_tmp = image;
	    unsigned long tx_size=0;
	    while(leave_size>0) {

		if(leave_size>MAX_SIZE_EACH_SEND) {
		    tx_size = MAX_SIZE_EACH_SEND;
		}
		else {
		    tx_size = leave_size;
		}

		leave_size -= tx_size;
		buffer_offset_tmp = sizeof(BMSendFiles);

		// copy image
		memcpy(g_buffer + buffer_offset_tmp, image_tmp, tx_size);
		pdata->data_offset = buffer_offset_tmp;

		// copy file size
		pdata->size = tx_size;
		buffer_offset_tmp+=tx_size;

    		ret = SendUSBData(local_id, CMD_SEND_FILE, 0, session_id, buffer_offset_tmp);
		if (ret == SUCCESS) {
		    ret_code = RecvUSBData(local_id, &recv_size);
    		}

		image_tmp += tx_size;
    	    }
#ifdef BMLINK
	    BMLinkSessionClose(local_id);
#endif
	}

	if(ret_code != OK) {
	    if(status_cbFun) {
		status_cbFun(session_id, 0, BitMain_UpgradeError);
	    }
	    printf("send file failed(%d)!!!!\n", ret_code);
	}

    UNLOCK_AND_RET
}

void UpgradeInit(uint64_t session_id, uint32_t filenums, uint32_t totalFileLen, upgradeStatusCB cbFun, BitMainVer ver)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size;
	
    printf("----------- UpgradeInit ------------------\n");
    printf("fw_type=%d, filenums = %d, totalFileLen=%u\n", ver, filenums, totalFileLen);

    try_open_device();

    TRY_LOCK

    BMUpgradeInit *upgrade_init = (BMUpgradeInit*) g_buffer;

    upgrade_init->filenums = filenums;
    upgrade_init->filesize = totalFileLen;
    upgrade_init->fwtype = ver;

#ifdef BMLINK
    // Initiate communication
    ret_code = RECV_ERROR;
    ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
    if (ret == 0)
#endif
    {
   	ret = SendUSBData(local_id, CMD_UPGRADE_INIT, 0, 0, sizeof(BMUpgradeInit));
	if (ret == SUCCESS) {
		    ret_code = RecvUSBData(local_id, &recv_size);
	}
#ifdef BMLINK
	BMLinkSessionClose(local_id);
#endif
    }

    status_cbFun = cbFun;

    if(ret_code == OK) {
	if(status_cbFun) {
	    status_cbFun(session_id, 0, BitMain_UpgradeNormal);
	}
    }

    UNLOCK_AND_RET
}

void ReadyAcceptSubfile(uint64_t session_id, uint32_t fileId)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size;
	
    printf("----------- ReadyAcceptSubfile subfileId=%d------------------\n", fileId);

    try_open_device();

    TRY_LOCK

    BMSubFiles *psubfile = (BMSubFiles*)g_buffer;
    psubfile->subfileid = fileId;
 
#ifdef BMLINK
    // Initiate communication
    ret_code = RECV_ERROR;
    ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
    if (ret == 0)
#endif
    {
    	ret = SendUSBData(local_id, CMD_UPGRADE_SUBFILEID, 0, 0, sizeof(BMSubFiles));
	if (ret == SUCCESS) {
	   ret_code = RecvUSBData(local_id, &recv_size);
	}
#ifdef BMLINK
	BMLinkSessionClose(local_id);
#endif
    }

    if(ret_code!=OK) {
	if(status_cbFun) {
	    status_cbFun(session_id, 0, BitMain_UpgradeError);
	}
    }

    UNLOCK_AND_RET
}

void subFileEOF(uint64_t session_id)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size;
	
    printf("----------- SubFileEOF ------------------\n");

    try_open_device();
    TRY_LOCK

#ifdef BMLINK
    // Initiate communication
    ret_code = RECV_ERROR;
    ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
    if (ret == 0)
#endif
    {
   	ret = SendUSBData(local_id, CMD_UPGRADE_SUBEOF, 0, 0, 0);
	if (ret == SUCCESS) {
		    ret_code = RecvUSBData(local_id, &recv_size);
	}
#ifdef BMLINK
	BMLinkSessionClose(local_id);
#endif
    }

    if(ret_code == OK) {
	BMSubFiles *psubfiles = (BMSubFiles *)g_buffer;	
	printf("up_percent=%d\n", psubfiles->percentage);
	if(status_cbFun) {
	    status_cbFun(session_id, psubfiles->percentage, BitMain_UpgradeNormal);
	}
    }
    else if(ret_code == READY_DO_UPGRADE) {

	printf("start do upgrade!!!!\n");
	status_cbFun(session_id, 100, BitMain_UpgradeNormal);
	
	//do bm1880 upgrade
	doDeviceUpgrade(session_id);
    }
    else
	printf("unkonwn cmd(%d)\n", ret_code);

    UNLOCK_AND_RET
}

void doDeviceUpgrade(uint64_t session_id)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size;
	
    printf("----------- doDeviceUpgrade ------------------\n");

#ifdef BMLINK
    // Initiate communication
    ret_code = RECV_ERROR;
    ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
    if (ret == 0)
#endif
    {
    	ret = SendUSBData(local_id, CMD_DEVICE_UPGRADE, 0, 0, 0);
	if (ret == SUCCESS) {
	    ret_code = RecvUSBData(local_id, &recv_size);
	}
#ifdef BMLINK
	BMLinkSessionClose(local_id);
#endif
    }

    if(ret_code == OK) {
	printf("do Device Upgrade finished, reboot device!!!!\n");
	if(status_cbFun) {
	    status_cbFun(session_id, 0, BitMain_UpgradeSuccess);
	}
    }
    else {
	printf("do Device Upgrade failed, try again!!!!\n");
	if(status_cbFun) {
	    status_cbFun(session_id, 0, BitMain_UpgradeError);
	}
    }

}

void abortUpgrade(uint64_t session_id)
{
    int ret, local_id=0;
    unsigned int ret_code;
    uint32_t recv_size;
	
    printf("----------- aboutUpgrade ------------------\n");

    try_open_device();

    TRY_LOCK

#ifdef BMLINK
    // Initiate communication
    ret_code = RECV_ERROR;
    ret = BMLinkSessionOpen(&kBMLink, 0, &local_id);
    if (ret == 0)
#endif
    {
    	ret = SendUSBData(local_id, CMD_UPGRADE_ABORT, 0, 0, 0);
	if (ret == SUCCESS) {
	    ret_code = RecvUSBData(local_id, &recv_size);
	}
#ifdef BMLINK
	BMLinkSessionClose(local_id);
#endif
    }

    if(ret_code == OK) {
	if(status_cbFun) {
	    status_cbFun(session_id, 0, BitMain_UpgradeError);
	}
    }

    UNLOCK_AND_RET
}

