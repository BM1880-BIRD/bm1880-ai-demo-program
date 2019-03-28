#include <stdio.h>
#include "btmain.h"
#include "bm_usbtty.h"
#include "bm_link.h"
#include "ipc_configs.h"

void callback_test(uint64_t session_id, uint32_t ret_code, uint32_t image_size, void *images);

void usage(void)
{
	printf("usage: test_getimage user_id\n");
}

int main(int argc, char *argv[]) {

	if(argc<2) {
		usage();
		return 0;
	}

	printf("start get_user_image user_id=%d\n", atoi(argv[1]));
	uint64_t sid = 100;
	unsigned char *buffer;
	uint32_t uid;
	FILE *pfile;
	uint32_t size;

	btmain_init();

	uid = atoi(argv[1]);
	enumerate_users(sid, NULL);
	get_user_image(sid, uid, callback_test);

	btmain_uninit();

	return 0;
}

void callback_test(uint64_t session_id, uint32_t ret_code, uint32_t image_size, void *images)
{
	printf("### do_get_image got callback, ret=%d, image_size= %d\n", ret_code, image_size);
	FILE *pfile;

	pfile = fopen("download.jpg", "wb");

	fwrite(images, 1, image_size, pfile);

	fclose(pfile);
}

