#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__


#define MAX_INPUT_CHARS 100
#define NUM_CMD_PARAM 20
#define LEN_CMD_PARAM 20
typedef struct diag_cli_cmd {
  const char * cmd_name;
  const char * cmd_info;
  int (*func)(int argc, char *argv[]);
}DIAG_CLI_CMD_T;

//#define DEBU
//#define PERFORMACE_TEST

extern string host_ip_addr;
extern int log_level;
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

extern bool performace_test;

int cli_cmd_help(int argc, char **argv);
int cli_cmd_ipconfig(int argc, char **argv);
int cli_cmd_log_level(int argc, char *argv[]);


#endif
