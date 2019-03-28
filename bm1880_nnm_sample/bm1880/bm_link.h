/// \file bm_link.h
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


#ifndef BM_LINK_H_
#define BM_LINK_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>

#ifdef __cplusplus

extern "C"{

#endif

#define TTYACM_PATH "/dev/ttyACM0"
#define TTYUSB_PATH "/dev/ttyGS0"

	typedef enum {
		BMLINK_TTYUSB = 0,
		BMLINK_LIBUSB,
		BMLINK_PROTOCOL_MAX
	} bmlink_protocol_t;

	typedef enum {
		BMLINK_PAYLOAD_BINARY = 0,
		BMLINK_PAYLOAD_JASON,
		BMLINK_PAYLOAD_MAX
	} bmlink_payload_type_t;

	typedef struct {
		int32_t used;
		int32_t timeout;
		int32_t local_id;
		uint32_t remote_id;
		uint32_t sequence;
		uint32_t priority;
		sem_t sem;
		bmlink_payload_type_t payload_type; 
		uint32_t payload_size;
		uint8_t *payload;
	} bmlink_data_t;

	typedef struct {
		uint32_t size;
		bmlink_data_t *element;
	} bmlink_mempool_t;

	typedef struct bmlink_list_node {
		bmlink_data_t *data;
		struct bmlink_list_node *prev;
		struct bmlink_list_node *next;
	} bmlink_list_node_t;

	typedef struct bmlink_list {
		sem_t sem;
		bmlink_list_node_t *head;
		bmlink_list_node_t *tail;
	} bmlink_list_t;

	typedef struct {
		sem_t write_sem;
		pthread_t write_thread_id;
		pthread_t read_thread_id;
		bmlink_mempool_t write_big_mempool;
		bmlink_mempool_t write_small_mempool;
		bmlink_mempool_t read_big_mempool;
		bmlink_mempool_t read_small_mempool;
		bmlink_list_t write_list;
		bmlink_list_t read_list;
	} bmlink_scheduler_t;

	typedef struct {
		bmlink_protocol_t type;
		int32_t valid;
		int32_t fd;
		// libusb_device_handle *handle;
		bmlink_scheduler_t *scheduler;
	} bmlink_t;

	// initiallization for single usb device
	int32_t BMLinkSingleInit(size_t big_write_mempool, size_t small_write_mempool, size_t big_read_mempool, size_t small_read_mempool);

	// initiallization for multiple usb devices
	// now not open


	int32_t BMLinkOpen(const char *path, bmlink_protocol_t type, bmlink_scheduler_t *scheduler, bmlink_t *link);
	int32_t BMLinkClose(bmlink_t *link);

	// 0 for all, [1, BMLINK_SESSION_RESTRICTED) restricted for different service, [BMLINK_SESSION_RESTRICTED, BMLINK_SESSION_MAX] random for clients
	int32_t BMLinkSessionOpen(bmlink_t *link, bool restricted, int32_t *session_id);
	int32_t BMLinkSessionRead(int32_t local_id, int32_t *remote_id, void *buf, size_t size, int32_t timeout);
	int32_t BMLinkSessionWrite(int32_t local_id, int32_t remote_id, const void *buf, size_t size, int32_t timeout, uint32_t priority, bmlink_payload_type_t payload_type);
	int32_t BMLinkSessionClose(int32_t session_id);
	int32_t connect_to_host(void);
	int32_t wait_module_connected(void);

#ifdef __cplusplus

}

#endif

#endif // BM_LINK_H_
