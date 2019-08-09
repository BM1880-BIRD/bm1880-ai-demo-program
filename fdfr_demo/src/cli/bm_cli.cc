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
#include <string.h>
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#include "bm_common_config.h"
#include "bm_cli.h"
#include "bm_spi_lcd.h"
#include "bm_camera.h"

static char s[MAX_INPUT_CHARS] = "";
static int _argc=0;
static char _argv_a[NUM_CMD_PARAM][LEN_CMD_PARAM];
static char *_argv[NUM_CMD_PARAM];
int log_level = DEFAULT_LEVEL;



int BmCliCmdHelp(int argc, char **argv);
int BmCliCmdLogLevel(int argc, char *argv[]);
void BmCliListCliBase(void);

int BmCliInputCmdParse(void)
{
	char __cmd[LEN_CMD_PARAM];
	char pos=0;
	int i;

	_argc = 0;

	for (i=0; i<MAX_INPUT_CHARS; i++) {
		if ((*(s+i) != ' ') && (*(s+i) != '\n')) {
			__cmd[pos] = *(s+i);
			pos ++;

			if ( pos >= LEN_CMD_PARAM) {
				BM_LOG(LOG_DEBUG_ERROR,cout<<"error: input command is too long !!"<<endl);
				return -1;
			}
		} else {
			if (pos != 0 ) {
				__cmd[pos] = '\0';
				strcpy(_argv_a[_argc],__cmd);
				BM_LOG(LOG_DEBUG_USER_(3),cout<<"get param: "<<__cmd<<" + argv "<<_argv_a[_argc]<<endl);
				_argc++;
				pos = 0;
				if (_argc >= NUM_CMD_PARAM) {
					break;
				}
			}
			if (*(s+i) == '\n') {
				break;
			}
		}
	}
	BM_LOG(LOG_DEBUG_USER_(3),cout<<"Total param: "<<_argc<<endl);
	return 0;
}

static const DIAG_CLI_CMD_T BmCmdList[]={
	{"help","Show how to use this debug system.",BmCliCmdHelp},
	{"loglevel","Set debug log level. ",BmCliCmdLogLevel},
	{"printcfg","Show bm1880 env config.",BmCliCmdPrintCfg},
	{"setcfg","Set bm1880 env config.",BmCliCmdSetCfg},
	{"lcd","Lcd display enable.",CliCmdEnableLcd},
	{"fdfr","Config/Open/Stop camera",CliCmdfdfr},
	{"testfr","Start face detect program",CliCmdTestFr},
	{"facereg","Start face register program",CliCmdDoFaceRegister},
	{"repo","Repository manage,",CliCmdRepoManage},
	{"getframe","Get Frame form camera", BmCliCmdGetFrame},
	//{"clr","clear all setting", clr_cmd},
	{"exit","Exit demo program!",NULL},
};

void BmCliListCliBase(void)
{
	cout << "bm1880:>";
}

void BmCliListCliHelp(void)
{
	int i;

	BM_LOG(LOG_DEBUG_NORMAL,cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl);
	for (i=0; i<sizeof(BmCmdList)/sizeof(DIAG_CLI_CMD_T);i++) {
		cout << BmCmdList[i].cmd_name << "     \t" << "\"" << BmCmdList[i].cmd_info << "\"" <<endl;
	}
	BM_LOG(LOG_DEBUG_NORMAL,cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl);
}

int BmCliCmdHelp(int argc, char *argv[])
{
	BmCliListCliHelp();
	return 0;
}




int BmCliCmdLogLevel(int argc, char *argv[])
{
	int level;
	if(argc == 1) {
		cout << "Usage: loglevel [0-9]" << endl;
		cout << "Current log_level = " << log_level << " ." <<endl;
		return 0;
	} else if(argc == 2) {
		if ((!strcmp(argv[1],"perf"))) {
			performace_test = !performace_test;
			cout << "performace test : " << (performace_test?"true":"false") <<endl;
		} else {
			level = atoi(argv[1]);
			if ((level >= 0) && (level <=9)) {
				log_level = level;
				cout<< "set log_level = " << log_level << " ."<<endl;
				return 0;
			}
		}
	}
	cout << "Input Error !!\n" <<endl;
	return -1;
}

void BmCliInit(void)
{
	int i;
	for(i=0; i<NUM_CMD_PARAM; i++) {
		//printf("i=%d, addr 0x%lx \n",i,(long)_argv_a[i]);
		_argv[i] = _argv_a[i];
	}
}

void BmCliMain(int argc, char *argv[])
{
	int i;
	system("stty erase ^H");

	if((argc !=0) && !strcmp(argv[0], "exit")){
		cout<<endl;
		return;
	}
	if (argc != 0) {
		for (i=0; i< sizeof(BmCmdList)/sizeof(DIAG_CLI_CMD_T); i++) {
			if (!strcmp(argv[0], BmCmdList[i].cmd_name)) {
				//cout<<"i = "<<i<<" "<<CmdList[i].cmd_name<<endl;
				if(NULL != BmCmdList[i].func) {
					BmCmdList[i].func(argc, (char **)argv);
				}
			}
		}
	}
	while (1) {
		//flockfile(stdin);
		fgets(s,MAX_INPUT_CHARS,stdin);
		//cout<<"your input is :"<<s<<endl;

		if( 0 != BmCliInputCmdParse()){
			return;
		}

		if((_argc !=0) && !strcmp(_argv[0], "exit")){
			cout<<endl;
			break;
		}
		if (_argc != 0) {
			for (i=0; i< sizeof(BmCmdList)/sizeof(DIAG_CLI_CMD_T);i++) {
				if (!strcmp(_argv[0], BmCmdList[i].cmd_name)) {
					//cout<<"i = "<<i<<" "<<CmdList[i].cmd_name<<endl;
					if(NULL != BmCmdList[i].func) {
						BmCmdList[i].func(_argc, (char **)_argv);
					}
				}
			}
		}
		BmCliListCliBase();
		sleep(1);
	}
}

