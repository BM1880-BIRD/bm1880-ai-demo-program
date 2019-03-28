#ifndef BM_CQUENE_H
#define BM_CQUENE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include "ipc_configs.h"

#define MAX_BUFFER_NUM 3

class BMCircularBuf
{
public:
    BMCircularBuf(uint32_t size);
    ~BMCircularBuf(void);
    void AddtoQuene(unsigned char *pdata, uint64_t frame_time);
    void DelFromQuene(void);
    unsigned char* GetFromQuene(uint64_t* id);
    bool QueneisFull(void);
    bool QueneisEmpty(void);
private:

    uint32_t buf_size;
    uint32_t buf_front;
    uint32_t buf_rear;
    uint32_t buf_cnt;
    pthread_mutex_t buf_lock;
    unsigned char* quene[MAX_BUFFER_NUM];
    uint64_t identify[MAX_BUFFER_NUM];

};

#endif // BM_CQUENE_H
