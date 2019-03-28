#include <stdio.h>
#include "btmain.h"
#include "bm_usbtty.h"
#include "bm_link.h"
#include "ipc_configs.h"
#include <sys/time.h>

#define MAX_BUFFER_SIZE 1382400 //(1280*720*3*2)

void callback_test(uint64_t session_id, uint32_t ret_code, uint32_t family_count, uint32_t stranger_count, uint32_t confidence, uint32_t *family_ids);

void usage(void)
{
	printf("usage: test_dorecognize image_path\n");
}

int main(int argc, char *argv[]) {

	if(argc<2) {
		usage();
		return 0;
	}

	printf("start do_recognize\n");
	uint64_t sid = 100;
	unsigned char *buffer;
	FILE *pfile;
	uint32_t size;

	buffer = malloc(MAX_BUFFER_SIZE);

	btmain_init();

	pfile = fopen(argv[1], "rb");

	fseek(pfile, 0L, SEEK_END);
	size = ftell(pfile);
	fseek(pfile, 0L, SEEK_SET);

	fread(buffer, size, 1, pfile);

	unsigned long diff;
	struct timeval start, end;

	gettimeofday(&start, NULL);

	for(int i=0;i<100;i++) {
		recognize_image(sid, size, buffer, callback_test);
		printf("time=%d\n", i);
	}

	gettimeofday(&end, NULL);
	diff =  1000000 * (end.tv_sec-start.tv_sec) + end.tv_usec - start.tv_usec;
	printf("spend %ld usec\n", diff);

	btmain_uninit();

	return 0;
}

void callback_test(uint64_t session_id, uint32_t ret_code, uint32_t family_count, uint32_t stranger_count, uint32_t confidence, uint32_t *family_ids)
{
	printf("### do_register got callback, ret=%d\n", ret_code);
}

