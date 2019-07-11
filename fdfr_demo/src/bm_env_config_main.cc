/*
 ********************************************************************
 * Demo program on Bitmain BM1880
 *
 * Copyright © 2018 Bitmain Technologies. All Rights Reserved.
 *
 * Author:  liang.wang02@bitmain.com
 *
 ********************************************************************
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include "qnn.hpp"

using namespace std;
using namespace cv;
using namespace qnn::vision;
using namespace qnn::utils;

#include "bm_common_config.h"
#include "bm_cli.h"
#include "bm_face.h"
#include "bm_spi_lcd.h"
#include "bm_camera.h"

#ifdef PERFORMACE_TEST
bool performace_test = true;
#else
bool performace_test = false;
#endif

EDB_ENV_CONFIG_T edb_config;

//Show Welcome message.
#define SHOW_WELLCOM()\
do\
{\
	cout<<"Welcome to BM1880 Demo program"<<endl;\
	cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;\
	cout<<"## www.sophon.cn "<<endl;\
	cout<<"## Version: " << __DATE__<<endl;\
	cout<<"## support: liang.wang02@bitmain.com"<<endl;\
	cout<<"## Bitmain Tecknologies.All Rights Reserved."<<endl;\
	cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;\
}while(0)

void BmfaceDoFaceSpoofing(int mode,string &source_path)
{
	;
}
string GetSystemTime(void)
{
	time_t timep;
	time (&timep);
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y_%m_%d_%H_%M_%S",localtime(&timep) );
	return tmp;
}

void BmListAllFiles(string &dir_name , vector<string> &file_list)
{
	string name;
	struct dirent * filename;	// return value for readdir()
	DIR * dir;					// return value for opendir()
	struct stat s_buf;
	string dir_or_file;

	dir = opendir(dir_name.c_str());

	if (NULL == dir) {
		cout<<"Can not open dir "<<dir_name<<endl;
		return;
	}
	//cout<<"Successfully opened the dir !"<<endl;

	/* read all the files in the dir ~ */
	while (( filename = readdir(dir) ) != NULL) {
		// get rid of "." and ".."
		if( strcmp( filename->d_name , "." ) == 0 ||
			strcmp( filename->d_name , "..") == 0 )
			continue;

		dir_or_file = dir_name +'/'+(filename->d_name);
		stat(dir_or_file.c_str() ,&s_buf);
		if (S_ISDIR(s_buf.st_mode)) {
			BmListAllFiles(dir_or_file, file_list);
		} else {
			file_list.emplace_back(dir_or_file);
	 	}
	}
	closedir(dir);
}


typedef struct config_item {
	char *key = NULL;
	char *value = NULL;
}CONFIG_ITEM_T;

/*
 *
 */
char *strtrimr(char *pstr)
{
	int i;
	i = strlen(pstr) - 1;
	while (isspace(pstr[i]) && (i >= 0))
		pstr[i--] = '\0';
	return pstr;
}
/*
 *
 */
char *strtriml(char *pstr)
{
	int i = 0,j;
	j = strlen(pstr) - 1;
	while (isspace(pstr[i]) && (i <= j))
		i++;
	if (0<i)
		strcpy(pstr, &pstr[i]);
	return pstr;
}
/*
 *
 */
char *strtrim(char *pstr)
{
	char *p;
	p = strtrimr(pstr);
	return strtriml(p);
}

/*
 *
 *
 */
int  get_item_from_line(char *line, CONFIG_ITEM_T *item)
{
	char *p = strtrim(line);
	int len = strlen(p);
	if (len <= 0) {
		return 1;
	}
	else if (p[0]=='#') {
		return 2;
	} else {
		char *p2 = strchr(p, '=');
		*p2++ = '\0';
		item->key = (char *)malloc(strlen(p) + 1);
		item->value = (char *)malloc(strlen(p2) + 1);
		strcpy(item->key,p);
		strcpy(item->value,p2);
	}
	return 0;
}

int file_to_items(const char *file, CONFIG_ITEM_T *items, int *num)
{
	char line[1024];
	FILE *fp;
	fp = fopen(file,"r");
	if(fp == NULL)
		return 1;
	int i = 0;
	while (fgets(line, 1023, fp)) {
		char *p = strtrim(line);
		int len = strlen(p);
		if (len <= 0) {
			continue;
		}
		else if(p[0]=='#') {
			continue;
		} else {
			char *p2 = strchr(p, '=');
			if(p2 == NULL)
				continue;
			*p2++ = '\0';
			items[i].key = (char *)malloc(strlen(p) + 1);
			items[i].value = (char *)malloc(strlen(p2) + 1);
			strcpy(items[i].key,p);
			strcpy(items[i].value,p2);

			i++;
		}
	}
	(*num) = i;
	fclose(fp);

	return 0;
}

int read_conf_value(const char *key, char *value,const char *file)
{
	char line[1024];
	memset(value, 0, 256);
	FILE *fp;
	fp = fopen(file,"r");
	if(fp == NULL)
		return 1;

	while (fgets(line, 1023, fp)) {
		CONFIG_ITEM_T item;

		if (get_item_from_line(line,&item) == 0) {
			if (!strcmp(item.key,key)) {
				strcpy(value,item.value);
				fclose(fp);
				if(item.key)
					free(item.key);
				if(item.value)
					free(item.value);
				break;
			}
		}
	}

	return 0;
}
int write_conf_value(const char *key,char *value,const char *file)
{
	CONFIG_ITEM_T items[20];
	int num;
	file_to_items(file, items, &num);

	int i=0;
	for (i=0;i<num;i++) {
		if(!strcmp(items[i].key, key)) {
			items[i].value = value;
			break;
		}
	}

	FILE *fp;
	fp = fopen(file, "w");

	if(fp == NULL)
		return 1;

	i=0;
	for (i=0;i<num;i++) {
		fprintf(fp,"%s=%s\n",items[i].key, items[i].value);
	}
	fclose(fp);
	return 0;
}


#define PRINT_CONFIG(A, B)  << #A<<"."<<#B << " = " << A.B << endl
void BmPrintEdbEnvConfig(void)
{
	BM_LOG(LOG_DEBUG_NORMAL, cout
	<< "-------------------------EdbEnvConfig-------------------------" << endl
	PRINT_CONFIG(edb_config, host_ip_addr)
	PRINT_CONFIG(edb_config, host_port)
	PRINT_CONFIG(edb_config, lcd_display)
	PRINT_CONFIG(edb_config, record_path)
	PRINT_CONFIG(edb_config, record_image)
	PRINT_CONFIG(edb_config, fd_only_maximum)
	PRINT_CONFIG(edb_config, do_fr)
	PRINT_CONFIG(edb_config, do_facepose)
	PRINT_CONFIG(edb_config, camera_type)
	PRINT_CONFIG(edb_config, camera_capture_mode)
	PRINT_CONFIG(edb_config, load_pic_path)
	PRINT_CONFIG(edb_config, step)
	//face spoofing
	PRINT_CONFIG(edb_config, fs_enable)
	//PRINT_CONFIG(edb_config, )
	<< "-----------------------------End------------------------------" << endl
	);
}

#define READ_CONF(A,B,C) \
			read_conf_value(#B, s, BM1880_EDB_ENV_FILE);\
			A.B = C
void Bm1880DemoEnvSetup(void)
{
	char s[256];

	if (access(BM1880_EDB_ENV_FILE,0) == -1) {
		BM_LOG(LOG_DEBUG_ERROR, cout << "Can't find config file." << endl);
	} else {
		//
		READ_CONF(edb_config, host_ip_addr, s);
		READ_CONF(edb_config, host_port, atoi(s));
		READ_CONF(edb_config, lcd_display, atoi(s));
		READ_CONF(edb_config, lcd_resize_type, atoi(s));
		READ_CONF(edb_config, record_path, s);
		READ_CONF(edb_config, record_image, atoi(s));
		READ_CONF(edb_config, fd_only_maximum, atoi(s));
		READ_CONF(edb_config, do_fr, atoi(s));
		READ_CONF(edb_config, do_facepose, atoi(s));
		READ_CONF(edb_config, camera_type, s);
		READ_CONF(edb_config, camera_capture_mode , atoi(s));
		READ_CONF(edb_config, load_pic_path, s);
		//face spoofing
		READ_CONF(edb_config, fs_enable, atoi(s));
	}
}

void BmPrintHelp(char **argv)
{
	cout << endl << "Usage: " << endl;
	cout << "help         : ./demo_edb --help" << endl;
	cout << "capture      : ./demo_edb --capture" << endl;
	cout << "run          : ./demo_edb --run --source [dir/pic/cam] --path [path/camera id] [--setp]" << endl;
	cout << "facereg      : ./demo_edb --facereg --add --source [dir/pic] --path [a valid path]" <<endl;
	cout << "facereg      : ./demo_edb --facereg --delete [id]" <<endl;
	cout << "facereg      : ./demo_edb --facereg --update [id] --source [pic] --path [a valid path]" <<endl;
	cout << "facereg      : ./demo_edb --facereg --list" <<endl;

	cout << "If no options will enter cli mode." << endl;
	//exit(-1);
}

enum pgm_opts
{
	opt_null, //0
	opt_help, //1
	opt_capture,
	opt_run,
	opt_source,
	opt_path,
	opt_step,
	opt_facereg,
	opt_add,
	opt_delete,
	opt_update,
	opt_list,
	opt_antispoof,
	opt_config,
	//opt_,
	//opt_,
	//opt_,
};

enum repo_opts
{
	REPO_NULL,
	REPO_ADD,
	REPO_DELETE,
	REPO_UPDATE,
	REPO_LIST,
	REPO_DELETE_ALL,
};


static struct option long_options[] = \
{
	{"help",		no_argument,		NULL,	opt_help},
	{"capture",		no_argument,		NULL,	opt_capture},
	{"run",			no_argument,		NULL,	opt_run},
	{"source",		required_argument,	NULL,	opt_source},
	{"path",		required_argument,	NULL,	opt_path},
	{"step",		no_argument,		NULL,	opt_step},
	{"facereg",		no_argument,		NULL,	opt_facereg},
	{"add",			no_argument,		NULL,	opt_add},
	{"delete",		required_argument,	NULL,	opt_delete},
	{"update",		required_argument,	NULL,	opt_update},
	{"list",		no_argument,		NULL,	opt_list},
	{"antispoof",	no_argument,		NULL,	opt_antispoof},
	{"config",		required_argument,	NULL,	opt_config},
	{NULL,			 0,					NULL,	opt_null}
};

int main(int argc, char *argv[])
{
	//
	int input_opt;
    int digit_optind = 0;
    int option_index = 0;

	repo_opts reg_mode = REPO_NULL;
	size_t face_id = 0;
	string source,sorce_path;
	bool pre_do_face_reg = false;
	bool pre_do_antispoof = false;
	bool capture_mode = false;
	bool run_mode = false;
	bool step_mode = false;
	SHOW_WELLCOM();
	
    while ((input_opt = getopt_long(argc, argv, "c:s:p", long_options, &option_index))!= -1) {
        BM_LOG(LOG_DEBUG_USER_4, cout<< "opt = 0x" << hex << input_opt 
			<< " optarg = " << ((optarg==NULL)?"Null":optarg)
			<< " optind = " << dec << optind
			<< " argv[optind] = " << ((argv[optind] ==NULL)?"Null":argv[optind]) 
			<< " option_index = " << dec << option_index
			<<endl);

		switch (input_opt) {
			case opt_help:
				BmPrintHelp(argv);
				return 0;
			case opt_capture:
				capture_mode = true;
				break;
			case opt_run:
				run_mode = true;
				break;
			case opt_source:
				source = optarg;
				break;
			case opt_path:
				sorce_path = optarg;
				break;
			case opt_step:
				edb_config.step = true;
				break;
			case opt_facereg:
				pre_do_face_reg = true;
				break;
			case opt_add:
				reg_mode = REPO_ADD;//repo add
				break;
			case opt_delete:
				face_id = atoi(optarg);
				cout << "face_id = " << face_id << endl;
				if(face_id != 0)
					reg_mode = REPO_DELETE;//repo delete
				else {
					string __arg(optarg);
					if(__arg.compare("all") == 0)
						reg_mode = REPO_DELETE_ALL;//repo delete all
				}
				break;
			case opt_update:
				face_id = atoi(optarg);
				reg_mode = REPO_UPDATE;//repo update
				break;
			case opt_list:
				reg_mode = REPO_LIST;//repo list
				break;
			case opt_antispoof:
				pre_do_antispoof = true;
				break;
			case '?':
				break;
			default:
				break;
		}
	}

	Bm1880DemoEnvSetup();

	if (capture_mode == true) {
		edb_config.camera_capture_mode = true;
		edb_config.record_image = true;
	}

	if (run_mode == true) {
		if (source.empty() || sorce_path.empty()) {
			BM_LOG(LOG_DEBUG_ERROR, cout << "Invalid input !!" << endl);
		} else {
			edb_config.source_type = source;
			edb_config.load_pic_path = sorce_path;
		}
		edb_config.record_image = false;
	}

	if (pre_do_antispoof == true) {
		if (source.compare("pic") == 0) {
			edb_config.fs_debug_frame_record_real = 0;
			edb_config.fs_debug_frame_record_fake = 0;
			BmfaceDoFaceSpoofing(1, sorce_path);
			char c = getchar();
			return 0;
		} else if(source.compare("dir") == 0) {
			edb_config.fs_debug_frame_record_real = 1;
			edb_config.fs_debug_frame_record_fake = 0;
			BmfaceDoFaceSpoofing(2, sorce_path);
			char c = getchar();
			return 0;
		} else {
			cout << "Invalid input!!" << endl;
			return -1;
		}
	}

	if (pre_do_face_reg == true) {
		cout << "reg_mode = " << reg_mode << endl;

		switch(reg_mode) {
			case REPO_ADD://repo add
				if (source.compare("pic") == 0) {
					BmFaceAddIdentity(1, sorce_path, 0);
				} else if (source.compare("dir") == 0) {
					BmFaceAddIdentity(2, sorce_path, 0);
				}
				break;
			case REPO_DELETE://repo delete
				BmFaceRemoveIdentity(face_id);
				break;
			case REPO_UPDATE://repo update
				BmFaceUpdateIdentity(sorce_path, face_id);
			case REPO_LIST://repo list
				BmFaceShowRepoList();
				break;
			case REPO_DELETE_ALL://repo delete all
				BmFaceRemoveAllIdentities();
				break;
			default:
				cout << "Invalid input!!" << endl;
				return -1;
		}
		char c = getchar();
		return 0;
	}

	//-------------------------------
	if ((capture_mode==true) || (run_mode == true)) {
		char __cmd0[] = "camera";
		char __cmd1[] = "open";
		static char *__argv[2] = {
			__cmd0,
			__cmd1,
		};
		CliCmdCamera(2, __argv);

		while (1) {
			sleep(1);
		}
	} else {
		BmCliInit();
		BmCliListCliHelp();
		BmCliListCliBase();
		BmCliMain();
	}
	return 0;
}




