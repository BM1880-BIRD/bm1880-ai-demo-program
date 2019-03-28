/// \file bm_link.c
/// \brief asynchronous usb interface of based on ttyusb or libusb
///
/// This is a temporary solution, we will add more features in future
/// To simplify the user interface, we assume you have only one usb device
/// So once initialized the environment, you can open only one link with the default scheduler that is BMLinkOpen(path, type, NULL, link)
/// 
/// If you have many usb devices, you have to do more as follows
/// First initialize the global session table once, then create a scheduler for each link you want to open, finally open sessions with the link of target device
///
/// \author jie.huang
/// \version V0.1.0-alpha
/// \date 2018-11-05


#include "bm_link.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>


#define BMLINK_SESSION_MAX 1024
#define BMLINK_MEMPOOL_MAX 256
#define BMLINK_WRITE_DATA_SMALL 1024
#define BMLINK_WRITE_DATA_BIG 1536000
#define BMLINK_READ_DATA_SMALL 1024
#define BMLINK_READ_DATA_BIG 1536000 // 128000
#define BMLINK_QUEUE_PRIORITY_MAX 255
#define BMLINK_QUEUE_PRIORITY_MIN 0
#define BMLINK_SESSION_RESTRICTED 128


typedef struct {
	uint32_t magic;
	uint32_t local_id;
	uint32_t remote_id;
	uint32_t sequence;
	uint32_t payload_size;
	uint32_t payload_type; // 
	uint32_t reserve1;
	uint32_t reserve2;
} bmlink_packet_head_t;

typedef struct {
	int32_t valid;
	int32_t session_id;
	// uint32_t remote_id;
	uint32_t sequence;  
	uint32_t start_time;
	uint32_t end_time;
	sem_t read_sem;
	bmlink_t *link;
} bmlink_session_t;

typedef struct {
	sem_t sem;
	int32_t initialized;
	uint32_t start;
	uint32_t restricted;
	uint32_t end;
	uint32_t top;
	uint32_t size;
	bmlink_session_t element[BMLINK_SESSION_MAX];
} bmlink_session_table_t;


static const uint32_t kTtyFrameLength = 64 * 1024;
static const char *kBMLinkMagic = "BMTX";
static bmlink_session_table_t kBMLinkSessionTable;
static bmlink_scheduler_t kBMLinkScheduler;


static int32_t BMLinkSessionTableInit(void);
static void* BMLinkSchedulerWriteThread(void* para);
static void* BMLinkSchedulerReadThread(void* para);
static bmlink_scheduler_t *BMLinkSchedulerGet(void);

int32_t wait_module_connected(void)
{
	int32_t fd, ret;
	struct termios old_settings, new_settings;
	speed_t speed = B115200;
	char tmp[128]={0};
	int num_read=0;

	fd = open(TTYACM_PATH, O_RDWR, O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("unable to open comport ");
		return -1;
	}

	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		close(fd);
		perror("Another process has locked the comport.");
		return -1;
	}

	ret = tcgetattr(fd, &old_settings);
	if (ret == -1) {
		close(fd);
		flock(fd, LOCK_UN);
		perror("unable to read port settings ");
		return -1;
	}

	memcpy(&new_settings, &old_settings, sizeof(new_settings));
	cfsetispeed(&new_settings, speed);
	cfsetospeed(&new_settings, speed);
	cfmakeraw(&new_settings);

	new_settings.c_cflag |= (CLOCAL | CREAD);
	new_settings.c_lflag |= ~(ICANON | ECHO | ECHOE | ISIG);
	new_settings.c_oflag |= ~OPOST;
	new_settings.c_cc[VMIN] = 0;
	new_settings.c_cc[VTIME] = 0;

	//new_settings.c_iflag |= (IXON | IXOFF);
	//new_settings.c_lflag |= (IXON | IXOFF);

	ret = tcsetattr(fd, TCSANOW, &new_settings);
	if (ret == -1) {
		tcsetattr(fd, TCSANOW, &old_settings);
		close(fd);
		flock(fd, LOCK_UN);
		perror("unable to adjust port settings ");
		return -1;
	}

	while(num_read<=0) {

		printf("#Wait USB module connection, ret=%d\n", num_read);

		num_read = read(fd, tmp, 128);

		if(num_read >0) {
			printf("get [%s]\n", tmp);
			write(fd, "ICLINK", 6);
		}
		sleep(1);
	}

	printf("#USB module connected\n");

	close(fd);
	flock(fd, LOCK_UN);

	printf("wait module reopen\n");
	sleep(1);//wait usb module reopen usbtty ready
}

int32_t connect_to_host(void)
{
	int32_t fd, ret;
	struct termios old_settings, new_settings;
	speed_t speed = B115200;

	fd = open(TTYUSB_PATH, O_RDWR, O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("unable to open comport ");
		return -1;
	}

	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		close(fd);
		perror("Another process has locked the comport.");
		return -1;
	}

	ret = tcgetattr(fd, &old_settings);
	if (ret == -1) {
		close(fd);
		flock(fd, LOCK_UN);
		perror("unable to read port settings ");
		return -1;
	}

	memcpy(&new_settings, &old_settings, sizeof(new_settings));
	cfsetispeed(&new_settings, speed);
	cfsetospeed(&new_settings, speed);
	cfmakeraw(&new_settings);

	new_settings.c_cflag |= (CLOCAL | CREAD);
	new_settings.c_lflag |= ~(ICANON | ECHO | ECHOE | ISIG);
	new_settings.c_oflag |= ~OPOST;
	new_settings.c_cc[VMIN] = 0;
	new_settings.c_cc[VTIME] = 0;

	ret = tcsetattr(fd, TCSANOW, &new_settings);
	if (ret == -1) {
		tcsetattr(fd, TCSANOW, &old_settings);
		close(fd);
		flock(fd, LOCK_UN);
		perror("unable to adjust port settings ");
		return -1;
	}

	int num_ret=0;
	char tmp[6] = "ICLINK";
	char tmp_[128] = {0};

	do {
		write(fd, tmp, 6);
		usleep(1);
		do {
			usleep(1);
			num_ret = read(fd, tmp_, 6);
		}while(num_ret<=0);
	}while(num_ret<=0);

	close(fd);
	flock(fd, LOCK_UN);
}

static int32_t TtyUsbOpen(const char *path)
{
	int32_t fd, ret;
	struct termios old_settings, new_settings;
	speed_t speed = B115200;

	if (path == NULL)
		return -1;

	fd = open(path, O_RDWR, O_NOCTTY);
	if (fd < 0) {
		perror("unable to open comport ");
		return -1;
	}

	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		close(fd);
		perror("Another process has locked the comport.");
		return -1;
	}

	ret = tcgetattr(fd, &old_settings);
	if (ret == -1) {
		close(fd);
		flock(fd, LOCK_UN);
		perror("unable to read port settings ");
		return -1;
	}

	memcpy(&new_settings, &old_settings, sizeof(new_settings));
	cfsetispeed(&new_settings, speed);
	cfsetospeed(&new_settings, speed);
	cfmakeraw(&new_settings);

	new_settings.c_cflag |= (CLOCAL | CREAD);
	new_settings.c_lflag |= ~(ICANON | ECHO | ECHOE | ISIG);
	new_settings.c_oflag |= ~OPOST;
	new_settings.c_cc[VMIN] = 0;
	new_settings.c_cc[VTIME] = 0;

	ret = tcsetattr(fd, TCSANOW, &new_settings);
	if (ret == -1) {
		tcsetattr(fd, TCSANOW, &old_settings);
		close(fd);
		flock(fd, LOCK_UN);
		perror("unable to adjust port settings ");
		return -1;
	}

	return fd;
}


static int32_t TtyUsbRead(int32_t fd, void *buf, size_t size) {
	size_t count = 0;
	size_t length;
	int32_t ret;

	if (fd < 0)
		return -1;

	while (count < size) {
		length = size - count;
		if (kTtyFrameLength != 0 && kTtyFrameLength < length)
			length = kTtyFrameLength;

		while (length > 0) {
			ret = read(fd, ((char *)buf) + count, length);
			if (ret < 0)
				return count;
			length -= ret;
			count += ret;
		}
	}

	return count;
}


static int32_t TtyUsbWrite(int32_t fd, const void *buf, size_t size) {
	size_t count = 0;
	size_t length, written;

	if (fd < 0)
		return -1;

	while (count < size) {
		length = size - count;
		if (kTtyFrameLength != 0 && kTtyFrameLength < length)
			length = kTtyFrameLength;

		written = write(fd, ((const char *)buf) + count, length);
		if (written != length) {
			if (written > 0)
				count += written;
			return count;
		}
		count += length;
	}

	return count;
}


static int32_t TtyUsbClose(int32_t fd)
{
	close(fd);

	return 0;
}


int32_t BMLinkOpen(const char *path, bmlink_protocol_t type, bmlink_scheduler_t *scheduler, bmlink_t *link) {
	int32_t ret;

	if (path == NULL || link == NULL)
		return -1;

	if (scheduler == NULL)
		scheduler = BMLinkSchedulerGet();
	if (scheduler == NULL)
		return -1;

	link->type = type;
	link->scheduler = scheduler;

	switch (link->type) {
		case BMLINK_TTYUSB:
			link->fd = TtyUsbOpen(path);
			if (link->fd < 0)
				return -1;
			break;

		case BMLINK_LIBUSB:
		default:
			return -1;
	}

	link->valid = 1;
	ret = pthread_create(&scheduler->write_thread_id, NULL, BMLinkSchedulerWriteThread, link);
	if (ret == 0)
		ret = pthread_create(&scheduler->read_thread_id, NULL, BMLinkSchedulerReadThread, link);

	if (ret != 0) {
		link->valid = 0;
		switch (link->type) {
			case BMLINK_TTYUSB:
				TtyUsbClose(link->fd);
				break;

			case BMLINK_LIBUSB:
			default:
				return -1;
		}
		return -1;
	}

	return 0;
}


static int32_t BMLinkRead(bmlink_t *link, void *buf, size_t size, int32_t timeout) {
	int32_t ret;

	if (link == NULL || buf == NULL || size == 0)
		return -1;

	switch (link->type) {
		case BMLINK_TTYUSB:
			ret = TtyUsbRead(link->fd, buf, size);
			if (ret < 0)
				return -1;
			break;

		case BMLINK_LIBUSB:
		default:
			return -1;
	}

	return ret;
}


static int32_t BMLinkWrite(bmlink_t *link, const void *buf, size_t size, int32_t timeout) {
	int32_t ret;

	if (link == NULL || buf == NULL || size == 0)
		return -1;

	switch (link->type) {
		case BMLINK_TTYUSB:
			ret = TtyUsbWrite(link->fd, buf, size);
			if (ret < 0)
				return -1;
			break;

		case BMLINK_LIBUSB:
		default:
			return -1;
	}

	return ret;
}


int32_t BMLinkClose(bmlink_t *link) {
	if (link == NULL)
		return -1;

	link->valid = 0;
	switch (link->type) {
		case BMLINK_TTYUSB:
			TtyUsbClose(link->fd);
			break;

		case BMLINK_LIBUSB:
		default:
			return -1;
	}

	return 0;
}


static inline void *BMLinkListNodeMalloc(size_t size) {
	// to do: in case of frequent memory operation
	return malloc(size);
}


static inline void BMLinkListNodeFree(void *ptr) {
	// to do: in case of frequent memory operation
	return free(ptr);
}

static bmlink_data_t *BMLinkDataMalloc(bmlink_scheduler_t *scheduler, size_t size, uint32_t write) {
	uint32_t i;
	bmlink_mempool_t *mempool;

	if (scheduler == NULL || size > BMLINK_READ_DATA_BIG)
		return NULL;

	if (write == 1) {
		if (size < BMLINK_WRITE_DATA_SMALL) {
			mempool = &scheduler->write_small_mempool;
		} else {
			mempool = &scheduler->write_big_mempool;
		}
	} else {
		if (size < BMLINK_READ_DATA_SMALL) {
			mempool = &scheduler->read_small_mempool;
		} else {
			mempool = &scheduler->read_big_mempool;
		}
	}

	if (mempool->size == 0)
		return NULL;

	for (i = 0; i < mempool->size; i++) {
		if ((mempool->element + i)->used == 0) {
			if (sem_trywait(&(mempool->element + i)->sem) != 0)
				continue;
			(mempool->element + i)->used = 1;
			sem_post(&(mempool->element + i)->sem);
			return (mempool->element + i);
		}
	}

	return NULL;  
}


static void BMLinkDataFree(bmlink_data_t *ptr) {
	if (ptr == NULL)
		return;

	sem_wait(&ptr->sem);
	ptr->used = 0;
	sem_post(&ptr->sem);

	return;
}


static int32_t BMLinkListInit(bmlink_list_t *list) {
	sem_init(&list->sem, 0, 1);
	list->head = NULL;
	list->tail = NULL;

	return 0;
}


static int32_t BMLinkListInsert(bmlink_list_t *list, bmlink_list_node_t *node, bmlink_data_t *data, int32_t after) {
	bmlink_list_node_t *tmp_node;

	if (list == NULL || data == NULL)
		return -1;

	if (node != list->head && node != list->tail) {
		tmp_node = list->head;
		while (tmp_node) {
			tmp_node = tmp_node->next;
			if (tmp_node == node)
				break;
		}
		if (tmp_node == NULL)
			return -1;
	}

	tmp_node = BMLinkListNodeMalloc(sizeof(bmlink_list_node_t));
	if (tmp_node == NULL)
		return -1;
	tmp_node->data = data;
	tmp_node->prev = NULL;
	tmp_node->next = NULL;

	if (list->head == NULL)
	{
		list->head = tmp_node;
		list->tail = tmp_node;
		return 0;
	}

	if (after == 1) {
		if (node == list->tail) {
			list->tail->next = tmp_node;
			tmp_node->prev = list->tail;
			tmp_node->next = NULL;
			list->tail = tmp_node;
		} else {
			node->next->prev = tmp_node;
			tmp_node->next = node->next;
			tmp_node->prev = node;
			node->next = tmp_node;
		}
	} else {
		if (node == list->head) {
			list->head->prev = tmp_node;
			tmp_node->next = list->head;
			tmp_node->prev = NULL;
			list->head = tmp_node;
		} else {
			node->prev->next = tmp_node;
			tmp_node->prev = node->prev;
			tmp_node->next = node;
			node->prev = tmp_node;
		}
	}
	return 0;
}


static int32_t BMLinkListDelete(bmlink_list_t *list, bmlink_list_node_t *node, bmlink_data_t **data) {
	if (list == NULL || node == NULL)
		return -1;

	if (list->head == NULL)
		return -1;

	if (node == list->head) {
		list->head = list->head->next;
		if (list->head == NULL) {
			list->tail = NULL;
		} else {
			list->head->prev = NULL;
		}
	} else if (node == list->tail) {
		list->tail = list->tail->prev;
		if (list->tail == NULL) {
			list->head = NULL;
		} else {
			list->tail->next = NULL;
		}
	} else {
		if (node->next == NULL || node->prev == NULL)
			return -1;
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}

	if (data != NULL) {
		*data = node->data;
	} else {
		BMLinkDataFree(node->data);
	}

	BMLinkListNodeFree(node);
	return 0;
}


static int32_t BMLinkListFindByPriority(bmlink_list_t *list, uint32_t priority, bmlink_list_node_t **node) {
	bmlink_list_node_t *tmp_node;

	if (list == NULL || node == NULL)
		return -1;

	if (priority == BMLINK_QUEUE_PRIORITY_MIN || list->tail == NULL || priority <= list->tail->data->priority) {
		*node = NULL;
		return 0;
	}

	tmp_node = list->head;
	while (tmp_node) {
		if (tmp_node->data->priority < priority)
			break;
		tmp_node = tmp_node->next;
	}

	*node = tmp_node;
	return 0;
}


static int32_t BMLinkListFindBySession(bmlink_list_t *list, int32_t session_id, bool local, bmlink_list_node_t **node) {
	bmlink_list_node_t *tmp_node;

	if (list == NULL || node == NULL)
		return -1;

	tmp_node = list->head;
	while (tmp_node) {
		if (local) {
			if (tmp_node->data->local_id == session_id)
				break;
		} else {
			if (tmp_node->data->remote_id == session_id)
				break;
		}

		tmp_node = tmp_node->next;
	}

	if (tmp_node == NULL)
		return -1;

	*node = tmp_node;
	return 0;
}


static inline bmlink_session_table_t *BMLinkSessionTableGet(void) {
	bmlink_session_table_t *table = &kBMLinkSessionTable;

	if (table->initialized != 1)
		return NULL;

	return table;
}


static int32_t BMLinkSessionTableInit(void) {
	uint32_t i;
	bmlink_session_table_t *table = &kBMLinkSessionTable;

	sem_init(&table->sem, 0, 1);
	table->start = 0;
	table->restricted = BMLINK_SESSION_RESTRICTED;
	table->end = BMLINK_SESSION_MAX;
	table->top = table->restricted;
	table->size = 0;

	for (i = table->start; i < table->end; i++) {
		table->element[i].valid = 0;
		table->element[i].session_id = i;
		sem_init(&table->element[i].read_sem, 0, 0);
	}

	table->initialized = 1;
	return 0;
}


static inline int32_t BMLinkSessionHash(int32_t session_id, uint32_t *index) {
	if (session_id < 0)
		return -1;

	*index = session_id;
	return 0;
}


int32_t BMLinkSessionOpen(bmlink_t *link, bool restricted, int32_t *session_id) {
	uint32_t i, start, end;
	bmlink_session_table_t *table;

	if (link == NULL || session_id == NULL)
		return -1;

	table = BMLinkSessionTableGet();
	if (table == NULL)
		return -1;
	sem_wait(&table->sem);

	if (restricted) {
		if (*session_id >= table->start && *session_id < table->restricted) {
			if (table->element[*session_id].valid == 0) {
				table->element[*session_id].valid = 1;
				table->element[*session_id].sequence = 0;
				table->element[*session_id].link = link;
				sem_post(&table->sem);
				return 0;
			}
		}
	} else {
		if (table->size < table->end - table->restricted) {
			if (table->size < table->top - table->restricted) {
				start = table->restricted;
				end = table->top;
			} else {
				start = table->top;
				end = table->end;
			}

			for (i = start; i < end; i++) {
				if (table->element[i].valid == 0) {
					table->element[i].valid = 1;
					table->element[i].sequence = 0;
					table->element[i].link = link;
					*session_id = table->element[i].session_id;
					table->size++;
					if (table->top < table->size + table->restricted)
						table->top = table->size + table->restricted;
					sem_post(&table->sem);
					return 0;
				}
			}
		}
	}

	sem_post(&table->sem);
	return -1;
}


int32_t BMLinkSessionRead(int32_t local_id, int32_t *remote_id, void *buf, size_t size, int32_t timeout) {
	int32_t ret;
	uint32_t index;
	sem_t *read_sem;
	bmlink_session_t* session;
	bmlink_t *link;
	bmlink_session_table_t *table;
	bmlink_list_node_t *node;
	bmlink_data_t *data;
	bmlink_list_t *list;

	if (remote_id == NULL || buf == NULL || size == 0)
		return -1;

	// find the session
	ret = BMLinkSessionHash(local_id, &index);
	if (ret != 0)
		return -1;

	table = BMLinkSessionTableGet();
	if (table == NULL)
		return -1;
	sem_wait(&table->sem);

	if (index >= table->top || table->element[index].valid == 0 || table->element[index].link == NULL) {
		sem_post(&table->sem);
		return -1;
	}
	session = &table->element[index];
	link = session->link;
	read_sem = &session->read_sem;
	sem_post(&table->sem);

	// copy data from the read list
	sem_wait(read_sem);
	list = &link->scheduler->read_list;
	sem_wait(&list->sem);  
	ret = BMLinkListFindBySession(list, local_id, 0, &node);
	if (ret != 0) {
		sem_post(&list->sem);
		return -1;
	}

	ret = BMLinkListDelete(list, node, &data);
	if (ret != 0) {
		sem_post(&list->sem);
		return -1;
	}
	sem_post(&list->sem);

	if (size > data->payload_size)
		size = data->payload_size;
	memcpy(buf, data->payload, size);
	ret = size;
	*remote_id = data->local_id;
	BMLinkDataFree(data);

	return ret;
}


int32_t BMLinkSessionWrite(int32_t local_id, int32_t remote_id, const void *buf, size_t size, int32_t timeout, uint32_t priority, bmlink_payload_type_t payload_type) {
	int32_t ret;
	uint32_t index;
	bmlink_session_t* session;
	bmlink_t *link;
	bmlink_session_table_t *table;

	bmlink_list_node_t *node;
	bmlink_data_t *data;
	bmlink_list_t *list;

	if (buf == NULL || size == 0)
		return -1;

	// find the session
	ret = BMLinkSessionHash(local_id, &index);
	if (ret != 0)
		return -1;

	table = BMLinkSessionTableGet();
	if (table == NULL)
		return -1;
	sem_wait(&table->sem);

	if (index >= table->top || table->element[index].valid == 0 || table->element[index].link == NULL) {
		sem_post(&table->sem);
		return -1;
	}

	session = &table->element[index];
	link = session->link;
	data = BMLinkDataMalloc(link->scheduler, size, 1);
	if (data == NULL) {
		sem_post(&table->sem);
		return -1;
	}
	data->local_id = session->session_id;
	data->remote_id = remote_id;
	data->sequence = session->sequence++;
	sem_post(&table->sem);
	data->timeout = timeout;
	data->priority = priority;
	data->payload_type = payload_type;
	data->payload_size = size;
	memcpy(data->payload, buf, size);

	// add to the write list according to priority
	list = &link->scheduler->write_list;
	sem_wait(&list->sem);
	ret = BMLinkListFindByPriority(list, priority, &node);
	if (ret != 0) {
		BMLinkDataFree(data);
		sem_post(&list->sem);
		return -1;
	}

	if (node == NULL) {
		ret = BMLinkListInsert(list, list->tail, data, 1);
	}
	else {
		ret = BMLinkListInsert(list, node, data, 0);
	}
	sem_post(&list->sem);
	if (ret != 0) {
		BMLinkDataFree(data);
		return -1;
	}

	// notify the scheduler write thread
	sem_post(&link->scheduler->write_sem);
	return size;
}


int32_t BMLinkSessionClose(int32_t session_id) {
	int32_t ret;
	uint32_t index;
	bmlink_session_table_t *table;

	ret = BMLinkSessionHash(session_id, &index);
	if (ret != 0)
		return -1;

	table = BMLinkSessionTableGet();
	if (table == NULL)
		return -1;
	sem_wait(&table->sem);

	if (index >= table->top) {
		sem_post(&table->sem);
		return -1;
	}

	if (table->element[index].valid == 1) {
		table->element[index].valid = 0;
		table->element[index].link = NULL;
		table->size--;
		if (index == table->top - 1)
			table->top--;
	}

	sem_post(&table->sem);
	return 0;
}


static int32_t BMLinkMempoolInit(bmlink_mempool_t *mempool, size_t elem_num, size_t elem_size) {
	uint32_t i;

	if (mempool == NULL)
		return -1;

	mempool->size = elem_num;
	if (elem_num == 0) {
		mempool->element = NULL;
	} else {
		mempool->element = malloc(elem_num * sizeof(bmlink_data_t));
		if (mempool->element == NULL) {
			mempool->size = 0;
			return -1;
		}
	}

	for (i = 0; i < mempool->size; i++) {
		sem_init(&(mempool->element + i)->sem, 0, 1);
		(mempool->element + i)->used = 0;
		(mempool->element + i)->payload_size = elem_size;
		if (elem_size == 0) {
			(mempool->element + i)->payload = NULL;
		} else {
			(mempool->element + i)->payload = malloc(elem_size);
			if ((mempool->element + i)->payload == NULL)
				return -1;
		}
	}

	return 0;
}


static bmlink_scheduler_t *BMLinkSchedulerGet(void) {
	return &kBMLinkScheduler;
}



int32_t BMLinkSchedulerInit(bmlink_scheduler_t *scheduler, size_t big_write_mempool, size_t small_write_mempool, size_t big_read_mempool, size_t small_read_mempool) {
	int32_t ret;

	sem_init(&scheduler->write_sem, 0, 0);

	if (big_write_mempool + small_write_mempool + big_read_mempool + small_read_mempool > BMLINK_MEMPOOL_MAX)
		return -1;

	ret = BMLinkMempoolInit(&scheduler->write_big_mempool, big_write_mempool, BMLINK_WRITE_DATA_BIG);
	if (ret != 0)
		return -1;

	ret = BMLinkMempoolInit(&scheduler->write_small_mempool, small_write_mempool, BMLINK_WRITE_DATA_SMALL);
	if (ret != 0)
		return -1;

	ret = BMLinkMempoolInit(&scheduler->read_big_mempool, big_read_mempool, BMLINK_WRITE_DATA_BIG);
	if (ret != 0)
		return -1;

	ret = BMLinkMempoolInit(&scheduler->read_small_mempool, small_read_mempool, BMLINK_WRITE_DATA_SMALL);
	if (ret != 0)
		return -1;

	ret = BMLinkListInit(&scheduler->write_list);
	if (ret != 0)
		return -1;

	ret = BMLinkListInit(&scheduler->read_list);
	if (ret != 0)
		return -1;

	return 0;
}


static void* BMLinkSchedulerWriteThread(void* para) {
	int32_t ret;
	uint32_t head_size;
	bmlink_t *link = (bmlink_t *)para;
	bmlink_scheduler_t *scheduler = link->scheduler;
	bmlink_list_t *list = &scheduler->write_list;
	bmlink_data_t *data;
	bmlink_packet_head_t head;

	head_size = sizeof(head);
	strncpy((char *)&head.magic, kBMLinkMagic, sizeof(head.magic));


	while (link->valid) {
		sem_wait(&scheduler->write_sem);
		sem_wait(&list->sem);
		ret = BMLinkListDelete(list, list->head, &data);
		sem_post(&list->sem);
		if (ret != 0)
			continue;

		head.local_id = htonl(data->local_id);
		head.remote_id = htonl(data->remote_id);
		head.sequence = htonl(data->sequence);
		head.payload_size = htonl(data->payload_size);
		head.payload_type = htonl(data->payload_type);
		ret = BMLinkWrite(link, &head, head_size, data->timeout);

		if (ret != head_size) {
			BMLinkDataFree(data);
			continue;
		}

		ret = BMLinkWrite(link, data->payload, data->payload_size, data->timeout);
		BMLinkDataFree(data);
	}

	return NULL;
}


static void* BMLinkSchedulerReadThread(void* para) { 
	int32_t ret;
	uint32_t head_size, payload_size, magic, index;
	bmlink_t *link = (bmlink_t *)para;
	bmlink_scheduler_t *scheduler = link->scheduler;
	bmlink_list_t *list = &scheduler->read_list;
	bmlink_data_t *data;
	bmlink_packet_head_t head;
	bmlink_session_table_t *table;
	sem_t *read_sem;

	head_size = sizeof(head);
	strncpy((char *)&magic, kBMLinkMagic, sizeof(magic));

	while (1) {
		table = BMLinkSessionTableGet();
		if (table != NULL)
			break;
		sleep(1);
	}

	while (link->valid) {
		// read from the link
		ret = BMLinkRead(link, &head, head_size, -1);
		if (ret != head_size || head.magic != magic)
			continue;

		payload_size = ntohl(head.payload_size);
		data = BMLinkDataMalloc(scheduler, payload_size, 0);
		if (data == NULL)
			continue;

		ret = BMLinkRead(link, data->payload, payload_size, 2);
		if (ret != payload_size) {
			BMLinkDataFree(data);
			continue;
		}

		data->local_id = ntohl(head.local_id);
		data->remote_id = ntohl(head.remote_id);
		data->sequence = ntohl(head.sequence);
		data->payload_type = ntohl(head.payload_type);
		data->payload_size = ntohl(head.payload_size);

		// find the session
		ret = BMLinkSessionHash(data->remote_id, &index);
		if (ret != 0) {
			BMLinkDataFree(data);
			continue;
		}   

		sem_wait(&table->sem);
		if (index >= table->top || table->element[index].valid == 0 || table->element[index].link == NULL) {
			sem_post(&table->sem);
			BMLinkDataFree(data);
			continue;
		}

		read_sem = &table->element[index].read_sem;
		sem_post(&table->sem);

		// add to the read list
		sem_wait(&list->sem);
		ret = BMLinkListInsert(list, list->tail, data, 1);
		sem_post(&list->sem);
		if (ret != 0) {
			BMLinkDataFree(data);
			continue;
		}

		// notify the session reader
		sem_post(read_sem);
	}

	return NULL;
}


int32_t BMLinkSingleInit(size_t big_write_mempool, size_t small_write_mempool, size_t big_read_mempool, size_t small_read_mempool) {
	int32_t ret;

	ret = BMLinkSessionTableInit();
	if (ret != 0)
		return -1;

	ret = BMLinkSchedulerInit(BMLinkSchedulerGet(), big_write_mempool, small_write_mempool, big_read_mempool, small_read_mempool);
	if (ret != 0)
		return -1;

	return 0;
}

