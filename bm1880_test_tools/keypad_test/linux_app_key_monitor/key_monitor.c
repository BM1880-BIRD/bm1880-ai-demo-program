#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
	int keys_fd;
	int event_num = 0;
	char event_name[100];
	struct input_event t;
	
	if (argc != 2) {
		printf("usage: key_monitor.bin [0/1/...]");
	}

	event_num = atoi(argv[1]);
	sprintf(event_name, "/dev/input/event%d", event_num);
	keys_fd = open (event_name, O_RDONLY);
	
	printf ("open %s device ", event_name);
	if (keys_fd <= 0) {
		printf("error!\n");
		return 0;
	} else
		printf("success!.\n");
	

	while (1)
	{
		if (read (keys_fd, &t, sizeof (t)) == sizeof (t)) {
			if (t.type == EV_KEY){
				if (t.value == 0 || t.value == 1) {
					printf ("key 0x%03X %s\n", t.code, (t.value) ? "Pressed" : "Released");
				if(t.code==KEY_ESC)
					break;
				}
			}
		}
	}
	close (keys_fd);

	return 0;
}