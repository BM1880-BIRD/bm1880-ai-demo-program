#ifndef BTMAIN_H
#define BTMAIN_H

#include "bm_usbtty.h"

typedef void (*register_cb)(uint64_t session_id, uint32_t ret_code, uint32_t user_id);
typedef void (*detect_cb)(uint64_t session_id, uint32_t ret_code, uint32_t family_count, uint32_t stranger_count, uint32_t confidence, uint32_t *family_ids);
typedef void (*delete_cb)(uint64_t session_id, uint32_t ret_code);
typedef void (*search_cb)(uint64_t session_id, uint32_t ret_code, uint32_t event_count, uint64_t *events);
typedef void (*enumerate_cb)(uint64_t session_id, uint32_t ret_code, uint32_t user_count, BitMainUsr *users_info);
typedef void (*get_user_image_cb)(uint64_t session_id, uint32_t ret_code, uint32_t image_size, void *images);
typedef void (*upgradeStatusCB)(uint64_t session_id, uint32_t process, uint32_t status);

void btmain_init(void);
void btmain_uninit(void);
void register_user(uint64_t session_id, uint32_t image_size, unsigned char *image, char *name, char *additional_info, register_cb callback);
void recognize_image(uint64_t session_id,  uint32_t image_size, unsigned char *image, detect_cb callback);
void delete_user(uint64_t session_id, uint32_t user_id, delete_cb callback);
void search_user(uint64_t session_id, uint32_t user_id, uint64_t begin, uint64_t end, search_cb callback);
void enumerate_users(uint64_t session_id, enumerate_cb callback);
void get_user_image(uint64_t session_id, uint32_t user_id, get_user_image_cb callback);
void getVersionInfo(uint64_t session_id, BitMainVer ver, BMVerTime *ptime, char* strversion);
void putUpgradeData(uint64_t session_id, unsigned char *image, uint32_t image_size);
void UpgradeInit(uint64_t session_id, uint32_t filenums, uint32_t totalFileLen, upgradeStatusCB cbFun, BitMainVer ver);
void ReadyAcceptSubfile(uint64_t session_id, uint32_t fileId);
void subFileEOF(uint64_t session_id);
void abortUpgrade(uint64_t session_id);
void doDeviceUpgrade(uint64_t session_id);

#endif // BTMAIN_H
