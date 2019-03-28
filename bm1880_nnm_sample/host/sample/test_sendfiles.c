#include <stdio.h>
#include "btmain.h"
#include "bm_usbtty.h"
#include "bm_link.h"
#include "ipc_configs.h"
#include <sys/time.h>

//#define MAX_BUFFER_SIZE 1*1024*1024  //32*1024
#define MAX_BUFFER_SIZE 512*1024  //32*1024

//#define TEST_LARGE_FILE

void callback_test(uint64_t session_id, int process, int status);

void usage(void)
{
	printf("usage: test_sendfiles fwtype filepath\n");
	printf("fwtype: 0:app, 1:bsp, 2:bmodel, 3:test\n");
}

int main(int argc, char *argv[]) {

	if(argc<3) {
		usage();
		return 0;
	}

	uint64_t sid = 101;
	unsigned char *buffer;
	FILE *pfile;
	unsigned long size;
	unsigned int totalsizes=1200000;
	int i=0;
	int fwtype = atoi(argv[1]);

	printf("start sendfile [%s], fwtype=%d\n", argv[2], atoi(argv[1]));

#ifndef TEST_LARGE_FILE
	buffer = malloc(MAX_BUFFER_SIZE);
#else
	char *v_data = malloc(MAX_BUFFER_SIZE);
#endif

	btmain_init();

	BMVerTime ptime;
	char version[16]={0};

	//getVersionInfo(sid, BitMain_AppVer, &ptime, &version);
	getVersionInfo(sid, BitMain_AppVer, &ptime, &version);
	printf("##sendfile => time[%d/%d/%d], ver=%s\n", ptime.tm_year, ptime.tm_mon, ptime.tm_mday, version);

	char filelist[4][64];
	memset(filelist, 0, sizeof(filelist));

	int filenum=1;
	strcpy(filelist[0], argv[2]);
	//int filenum=3;//test
	//strcpy(filelist[0], "x00");
	//strcpy(filelist[1], "x01");
	//strcpy(filelist[2], "x02");
	//strcpy(filelist[3], "bmface.bmodel.tar.gzad");


	//UpgradeInit(sid, filenum, totalsizes/*test*/, callback_test, BitMain_AppVer);
	//UpgradeInit(sid, filenum, totalsizes/*test*/, callback_test, fwtype);
	UpgradeInit(sid, filenum, totalsizes/*test*/, callback_test, fwtype);

	unsigned long diff;
	struct timeval start, end;

	gettimeofday(&start, NULL);


#ifndef TEST_LARGE_FILE
	for(i=0;i<filenum;i++) {

		ReadyAcceptSubfile(sid, i);

		printf("open %s\n", filelist[i]);
		pfile = fopen(filelist[i], "rb");
		fseek(pfile, 0L, SEEK_END);
		size = ftell(pfile);
		fseek(pfile, 0L, SEEK_SET);

		//get checksum
		//unsigned char checksum=0;
		//for(int j=0;j<size;j++) {
		//  checksum ^= fgetc(pfile);
		//}
		//printf("#1 checksum=%x\n", checksum);
		//fseek(pfile, 0L, SEEK_SET);

		int leave_size = size;
		uint32_t read_size=MAX_BUFFER_SIZE;//simulation tcp packet

		while(leave_size>0) {

			if(leave_size<=read_size)
				read_size=leave_size;

			fread(buffer, read_size, 1, pfile);
			//putUpgradeData(sid, buffer, read_size);
			putUpgradeData(sid, buffer, read_size);

			leave_size -= read_size;
			//printf("send %d byte\n\n", read_size);
		}
		printf("send file %s finish\n", filelist[i]);

		fclose(pfile);
		subFileEOF(sid);
	}
#else

	ReadyAcceptSubfile(sid, i);

	size = 100*1024*1024;//test tx size

	int leave_size = size;
	uint32_t read_size=MAX_BUFFER_SIZE;//simulation tcp packet

	while(leave_size>0) {

		if(leave_size<=read_size)
			read_size=leave_size;

		putUpgradeData(sid, v_data, read_size);

		leave_size -= read_size;
		//printf("send %d byte\n\n", read_size);
	}
	printf("send file %s finish\n", filelist[i]);


#endif

	gettimeofday(&end, NULL);
	diff =  1000000 * (end.tv_sec-start.tv_sec) + end.tv_usec - start.tv_usec;
	printf("spend %ld usec\n", diff);

	btmain_uninit();

#ifndef TEST_LARGE_FILE
	free(buffer);
#else
	free(v_data);
#endif
	return 0;
}

void callback_test(uint64_t session_id, int process, int status)
{
	printf("### sendfile got callback, in %d persent, status= %d\n", process, status);
}

