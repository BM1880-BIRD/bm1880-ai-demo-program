#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__



#define DEMO_FW_VERSION "2018/11 : 1.0.1"

#define MAX_INPUT_CHARS 100
#define NUM_CMD_PARAM 20
#define LEN_CMD_PARAM 20
//#define DEBU
//#define PERFORMACE_TEST
#define  LOG(x,fmt...) \
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
#define LOG_DEBUG_USER_5 (5)

#ifdef DEBUG
#define DEFAULT_LEVEL LOG_DEBUG_USER_H
#else
#define DEFAULT_LEVEL LOG_DEBUG_NORMAL
#endif

typedef struct diag_cli_cmd {
  const char * cmd_name;
  const char * cmd_info;
  int (*func)(int argc, char *argv[]);
}DIAG_CLI_CMD_T;

extern bool performace_test;
extern string host_ip_addr;
extern int log_level;

int CliCmdHelp(int argc, char **argv);
int CliCmdIpconfig(int argc, char **argv);
int CliCmdLogLevel(int argc, char *argv[]);


#endif
