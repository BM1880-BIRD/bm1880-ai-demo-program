#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
using namespace std;

#include "common_config.h"
#include "face_det.h"

#define DEMO_FW_VERSION "2018/11 : 1.0.1"
string host_ip_addr="192.168.1.127";
int log_level = DEFAULT_LEVEL;
#ifdef PERFORMACE_TEST
bool performace_test = true;
#else
bool performace_test = false;
#endif
static char s[MAX_INPUT_CHARS] = "";
static int _argc=0;
static char _argv_a[NUM_CMD_PARAM][LEN_CMD_PARAM];
static char *_argv[NUM_CMD_PARAM];
int input_cmd_parse(void)
{
	char __cmd[LEN_CMD_PARAM];
	char pos=0;
	int i;

	_argc = 0;
	for(i=0; i<MAX_INPUT_CHARS; i++)
	{
	if((*(s+i) != ' ') && (*(s+i) != '\n'))
	{
		__cmd[pos] = *(s+i);
		pos ++;
		if( pos >= LEN_CMD_PARAM)
		{
			LOG(LOG_DEBUG_ERROR,cout<<"error: input command is too long !!"<<endl);
			return -1;
		}
	}
	else
	{
		if(pos != 0 )
		{
			__cmd[pos] = '\0';
			strcpy(_argv_a[_argc],__cmd);
			LOG(LOG_DEBUG_USER_5,cout<<"get param: "<<__cmd<<" + argv "<<_argv_a[_argc]<<endl);
			_argc++;
			pos = 0;
			if( _argc >= NUM_CMD_PARAM)
			{
			break;
			}
		}
		//cout<<"i="<<i<<" *(s+i) = "<<(int)*(s+i)<<endl;
		if(*(s+i) == '\n')
		{
			break;
		}
	}
	}
	LOG(LOG_DEBUG_USER_4,cout<<"Total param: "<<_argc<<endl);
	return 0;
}


static const DIAG_CLI_CMD_T cmdList[]={
	{"help","Show how to use this debug system.",cli_cmd_help},
	{"loglevel","Set debug log level. ",cli_cmd_log_level},
	{"ipconfig","IP address setting.",cli_cmd_ipconfig},
	{"threshold","Read the value of sim_threshold",cli_cmd_threshold_value},
	{"rdpath","Set the test result record path",cli_cmd_record_path},
	{"variance","Enable/disable variance filter",cli_cmd_variance},
	{"algorithm","Bmiva face algorithm",cli_cmd_face_algorithm},
	{"uvc","Open/Stop uvc camera",cli_cmd_uvc_camera},
	{"facedet","Start face detect program",cli_cmd_do_facedet},
	
	//{"clr","clear all setting", clr_cmd},
	{"exit","Exit demo program!",NULL},
};

void list_cli_base(void)
{
	cout<<"bm1880:>";
}

void list_cli_help(void)
{
	int i;

	for(i=0; i<sizeof(cmdList)/sizeof(DIAG_CLI_CMD_T);i++)
	{
		cout<<cmdList[i].cmd_name<<"      "<<"\""<<cmdList[i].cmd_info<<"\""<<endl;
	}
	cout<<"+++++++++++++++++++++++++++++++++++++++++"<<endl;
}



int main(int argc, char *argv[])
{
	int i;
	for(i=0; i<NUM_CMD_PARAM; i++)
	{
		//printf("i=%d, addr 0x%lx \n",i,(long)_argv_a[i]);
		_argv[i] = _argv_a[i];
	}
	//flockfile(stdin);
	cout<<"Welcome to BM1880 Demo program"<<endl;
	cout<<"+++++++++++++++++++++++++++++++++++++++++"<<endl;
	cout<<"## www.sophon.com "<<endl;
	cout<<"## Version: "<<DEMO_FW_VERSION<<endl;
	cout<<"## support: liang.wang02@bitmain.com"<<endl;
	cout<<"## Bitmain Tecknologies.All Rights Reserved."<<endl;
	cout<<"+++++++++++++++++++++++++++++++++++++++++"<<endl;
	//fgets_unlocked(s,MAX_INPUT_CHARS,stdin);
	list_cli_help();
	list_cli_base();
	while(1)
	{
		fgets(s,MAX_INPUT_CHARS,stdin);
		//cout<<"your input is :"<<s<<endl;

		if( 0 != input_cmd_parse()){
			return -1;
		}

		if((_argc !=0) && !strcmp(_argv[0], "exit")){
			cout<<endl;
			break;
		}
		if((_argc !=0))
		{
			for(i=0; i< sizeof(cmdList)/sizeof(DIAG_CLI_CMD_T);i++)
			{
				if(!strcmp(_argv[0],cmdList[i].cmd_name))
				{
					//cout<<"i = "<<i<<" "<<cmdList[i].cmd_name<<endl;
					if(NULL != cmdList[i].func)
					{
						cmdList[i].func(_argc, (char **)_argv);
					}
				}
			}
		}
		list_cli_base();
		sleep(1);
	}

	return 0;
}


int cli_cmd_help(int argc, char *argv[])
{
	list_cli_help();
	return 0;
}

int cli_cmd_ipconfig(int argc, char *argv[])
{
	if(argc == 1)
	{
		if( host_ip_addr.empty() )
		{
			cout<<"Please set server IP address first !!!"<<endl;
		}
	}
	else
	{
		host_ip_addr = argv[1];
		//cout<<argv[0]<<"  "<<argv[1]<<endl;
	}
	cout<<"The server IP is :"<<host_ip_addr<<endl;
	return 0;
}


int cli_cmd_log_level(int argc, char *argv[])
{
	int level;
	if(argc == 1)
	{
		cout<<"Usage: loglevel [0-9]"<<endl;
		cout<<"Current log_level = "<<log_level<<" ."<<endl;
		return 0;
	}
	else if(argc == 2)
	{
		if((!strcmp(argv[1],"perf")))
		{
			performace_test = !performace_test;
			cout<<"performace test : "<<(performace_test?"true":"false")<<endl;
		}
		else
		{
			level = atoi(argv[1]);
			if((level > 0) && (level <=9))
			{
				log_level = level;
				cout<<"set log_level = "<<log_level<<" ."<<endl;
				return 0;
			}
		}
	}
	cout<<"Input Error !!\n"<<endl;
	return -1;
}
