/*
 ********************************************************************
 * Demo program on Bitmain BM1880
 *
 * Copyright © 2018 Bitmain Technologies. All Rights Reserved.
 *
 * Author:  liang.wang02@bitmain.com
 *			xiaojun.zhu@bitmain.com
 ********************************************************************
*/
#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__


#define DEMO_FW_VERSION "2019/01 : 1.0.2"

#define MAX_INPUT_CHARS 200
#define NUM_CMD_PARAM 20
#define LEN_CMD_PARAM 20
//#define DEBU
//#define PERFORMACE_TEST
#define  BM_LOG(x,fmt...) \
        do{\
                if(x <= log_level)\
                {\
                        fmt;\
                }\
        }while(0)

#define LOG_DISABLE (0)
#define LOG_DEBUG_ERROR (1)
#define LOG_DEBUG_NORMAL (2)
#define LOG_DEBUG_USER_3 (3)
#define LOG_DEBUG_USER_4 (4)
#define LOG_DEBUG_USER_(x) (x)

#ifdef DEBUG
#define DEFAULT_LEVEL LOG_DEBUG_USER_(3)
#else
#define DEFAULT_LEVEL LOG_DEBUG_NORMAL
#endif

#define PERFORMACE_TIME_MESSURE_START(start_time) \
		do {\
			if(performace_test)\
			{\
				gettimeofday(&start_time,NULL);\
			}\
		}while(0)

#define PERFORMACE_TIME_MESSURE_END(end_time) \
		do {\
			if(performace_test)\
			{\
				gettimeofday(&end_time,NULL);\
				float recog_time = (end_time.tv_sec - start_time.tv_sec)*1000000.0 + end_time.tv_usec-start_time.tv_usec;\
				BM_LOG(LOG_DEBUG_NORMAL, cout<<"recognize cost time:%f us\n"<<recog_time<<endl);\
			}\
		}while(0)

typedef struct diag_cli_cmd {
  const char * cmd_name;
  const char * cmd_info;
  int (*func)(int argc, char *argv[]);
}DIAG_CLI_CMD_T;

#define BM1880_EDB_ENV_FILE "fdfr_demo_env.conf"

typedef struct __bm1880_env
{
	//network
	string host_ip_addr = "192.168.1.163";
	int host_port = 9555;
	//320x240 lcd
	bool lcd_display = false;
	int lcd_resize_type = 0;
	//fd fr
	int fd_algo = 0;
	bool record_image = false;
	bool fd_only_maximum = false;
	int do_fr = 0;
	bool do_facepose = false;
	string record_path;
	//frame source
	string frame_source;
	int camera_width = 1280;
	int camera_height = 720;
	int camera_id = 0;
	string load_pic_path;
	//camera_capture_mode
	bool camera_capture_mode = false;
	bool step = false;
	bool pause = false;
	bool stop = false;
	//face spoofing
	bool afs_enable = 0;
	//debug
	bool afs_debug_show_visual = 0;
	string afs_frame_record_path;
	string afs_frame_record_real_prefix;
	string afs_frame_record_fake_prefix;
	bool afs_debug_frame_record_real = 0;
	bool afs_debug_frame_record_fake = 0;
} BM1880_ENV_CONFIG_T;

extern BM1880_ENV_CONFIG_T bm1880_config;
extern bool performace_test;
extern int log_level;


string GetSystemTime(void);
void BmListAllFiles(string &dir_name , vector<string> &file_list);
void BmFdFrDemoPrintEnvConfig(void);


#endif
