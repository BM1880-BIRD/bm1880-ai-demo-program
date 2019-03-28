#include <stdio.h>
#include "btmain.h"
#include "bm_usbtty.h"
#include "bm_link.h"
#include "ipc_configs.h"

#define MAX_BUFFER_SIZE 1382400 //(1280*720*3*2)

void callback_register(uint64_t session_id, uint32_t ret_code, uint32_t user_id);
void callback_enumeate(uint64_t session_id, uint32_t ret_code, uint32_t user_count, BitMainUsr *users_info);
void callback_del(uint64_t session_id, uint32_t ret_code);
void callback_event(uint64_t session_id, uint32_t ret_code, uint32_t event_count, uint64_t *events);

uint32_t user_num=0;

void usage(void)
{
	printf("usage: test_dofacedb add image_path username otherinfo\n");
	printf("usage: test_dofacedb del user_id\n");
	printf("usage: test_dofacedb list\n");
	printf("usage: test_dofacedb event user_id\n");
}

int main(int argc, char *argv[]) {

	int act_flag=0;//1:add, 2:del, 3:list user, 4:list event

	if(argc<2) {
		usage();
		return 0;
	}
	else if(!strcmp(argv[1], "add")) {
		if(argc!=5) {
			usage();
			return 0;
		}
		act_flag=1;
		printf("start add user\n");
	}
	else if(!strcmp(argv[1], "del")) {
		if(argc!=3) {
			usage();
			return 0;
		}
		act_flag=2;
		printf("start del user\n");
	}
	else if(!strcmp(argv[1], "list")) {
		if(argc!=2) {
			usage();
			return 0;
		}
		act_flag=3;
		printf("start list user\n");
	}
	else if(!strcmp(argv[1], "event")) {
		if(argc!=3) {
			usage();
			return 0;
		}
		act_flag=4;
		printf("start list event by user_id\n");
	}
	else {
		printf("unknow cmd\n");
		return 0;
	}


	uint64_t sid = 100;
	unsigned char *buffer;
	FILE *pfile;
	uint32_t size;
	uint32_t remove_id=0;
	uint32_t user_id=0;

	buffer = malloc(MAX_BUFFER_SIZE);

	btmain_init();

	enumerate_users(sid, callback_enumeate);

	if(act_flag==1) {//add
		pfile = fopen(argv[2], "rb");

		fseek(pfile, 0L, SEEK_END);
		size = ftell(pfile);
		fseek(pfile, 0L, SEEK_SET);

		fread(buffer, size, 1, pfile);

		register_user(sid, size, buffer, argv[3], argv[4], callback_register);
		fclose(pfile);
	}
	else if(act_flag==2) {//del
		remove_id = atoi(argv[2]);
		printf("remove id =%d\n", remove_id);
		//if(remove_id>user_num) {
		//printf("no such user id\n");
		//btmain_uninit();
		//return 0;
		//}
		//else {
		delete_user(sid, remove_id, callback_del);
		//}
	}
	else if(act_flag==3) {//list
		//enumerate_users(sid, callback_enumeate);
	}
	else if(act_flag==4) {//event
		user_id = atoi(argv[2]);
		printf("user_id =%d\n", user_id);
		if(user_id>user_num) {
			printf("no such user id\n");
			btmain_uninit();
			return 0;
		}
		else {
			search_user(sid, user_id, 0, 1500000000, callback_event);
		}
	}

	btmain_uninit();

	return 0;
}

void callback_register(uint64_t session_id, uint32_t ret_code, uint32_t user_id)
{
	printf("### do_register got callback, ret=%d, user_id=%d\n", ret_code, user_id);
}

void callback_enumeate(uint64_t session_id, uint32_t ret_code, uint32_t user_count, BitMainUsr *users_info)
{
	printf("### enumeate got callback, ret=%d, user cnt=%d\n", ret_code, user_count);
	user_num = user_count;
}

void callback_del(uint64_t session_id, uint32_t ret_code)
{
	printf("### del user got callback, ret=%d\n", ret_code);
}

void callback_event(uint64_t session_id, uint32_t ret_code, uint32_t event_count, uint64_t *events)
{
	printf("### list event got callback, ret=%d, event cnt=%d\n", ret_code, event_count);
	for(int i=0;i<event_count;i++) {
		printf("event=%lld\n", events[i]);
	}
}

