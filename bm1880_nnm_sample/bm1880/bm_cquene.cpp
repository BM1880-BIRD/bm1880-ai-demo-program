#include "bm_cquene.hpp"

BMCircularBuf::BMCircularBuf(uint32_t size) {

	buf_size = size;
	buf_front = 0;
	buf_rear = 0;
	buf_cnt = 0;

	if(pthread_mutex_init(&buf_lock, NULL) != 0)
		fprintf(stderr, "mutex buf_lock init failed\n");

	for(int i=0; i<MAX_BUFFER_NUM; i++)
		quene[i] = (unsigned char*)malloc(size);

}


BMCircularBuf::~BMCircularBuf(void) {

	for(int i=0; i<MAX_BUFFER_NUM; i++)
		free(quene[i]);

	pthread_mutex_destroy(&buf_lock);
}

bool BMCircularBuf::QueneisFull(void) {
	return (buf_cnt == MAX_BUFFER_NUM);
}

bool BMCircularBuf::QueneisEmpty(void) {
	return (buf_cnt == 0);
}

void BMCircularBuf::AddtoQuene(unsigned char *pdata, uint64_t id) {

	DEBUG_LOG("%s: cnt=%d, front=%d, rear=%d\n", __FUNCTION__, buf_cnt, buf_front, buf_rear);

	pthread_mutex_lock(&buf_lock);

	if(QueneisFull()) {
		DEBUG_LOG("!!!!!!!!!!!quene is full(%d/%d/%d)\n", buf_cnt, buf_front, buf_rear);
		pthread_mutex_unlock(&buf_lock);
		return;
	}

	memcpy(quene[buf_rear], pdata, buf_size);
	identify[buf_rear] = id;

	buf_rear = (buf_rear+1) % MAX_BUFFER_NUM;
	buf_cnt++;

	pthread_mutex_unlock(&buf_lock);

	return;
}

void BMCircularBuf::DelFromQuene(void) {

	DEBUG_LOG("%s, front=%d, rear=%d\n", __FUNCTION__, buf_front, buf_rear);

	pthread_mutex_lock(&buf_lock);

	if(QueneisEmpty()) {
		DEBUG_LOG("quene is empty\n");
		pthread_mutex_unlock(&buf_lock);
		return;
	}

	buf_front = (buf_front+1) % MAX_BUFFER_NUM;
	buf_cnt--;

	pthread_mutex_unlock(&buf_lock);

	return;
}

unsigned char* BMCircularBuf::GetFromQuene(uint64_t* id) {

	DEBUG_LOG("%s, cnt=%d,  front=%d, rear=%d\n", __FUNCTION__, buf_cnt, buf_front, buf_rear);

	if(QueneisEmpty()) {
		DEBUG_LOG("quene is empty\n");
		return NULL;
	}

	if(id!=NULL)
		*id = identify[buf_front];

	return quene[buf_front];
}
