#pragma once
#include <memory>
#include <curl/curl.h>
#include <signal.h>
#include <mutex>
#include <queue>
#include <atomic>
#include <thread>
#include "sample_comm.h"

#define MAX_PACK_SIZE 100000

namespace bm_mpp{
class FrameQueue;

class BM_MPP_SYSTEM {
    public:
        static std::unique_ptr<BM_MPP_SYSTEM> get_instance(int queue_size);

        BM_MPP_SYSTEM(int queue_size);
        ~BM_MPP_SYSTEM();
       
        std::vector<unsigned char> get();
        
        void runner_start();

    private:
        int init_mpp();
        int start_recv_venc_pic();
        int process_recv_pic();
        int stop_recv_pic();
        int recv_pic(){
            start_recv_venc_pic();
            process_recv_pic();
            stop_recv_pic();
        };

    private:
        std::unique_ptr<FrameQueue> q;
        std::atomic_bool running;
        std::thread runner;
   
    private:
        PAYLOAD_TYPE_E encode_PayLoad;
        PIC_SIZE_E encode_Size;
        int profile = 0;

        VB_CONF_S stVbConf;
        SAMPLE_VI_CONFIG_S stViConfig = {};

        VPSS_GRP VpssGrp;
        VPSS_CHN VpssChn;
        VPSS_GRP_ATTR_S stVpssGrpAttr;
        VPSS_CHN_ATTR_S stVpssChnAttr;
        VPSS_CHN_MODE_S stVpssChnMode;

        VENC_CHN VencChn;
        SAMPLE_RC_E encode_RcMode = SAMPLE_RC_CBR;
        int ret = HI_SUCCESS;
        HI_U32 u32BlkSize;
        SIZE_S stSize;

        VIDEO_NORM_E gs_encode_Norm;

        // Video snap process
        struct timeval TimeoutVal;
        fd_set read_fds;
        HI_S32 s32VencFd;
        VENC_CHN_STAT_S stStat;
        VENC_STREAM_S stStream;
        VENC_RECV_PIC_PARAM_S stRecvParam;
};

class FrameQueue{
    public:
        FrameQueue();
        ~FrameQueue();
        void set_queue_size(int size);
        bool put(std::vector<unsigned char> &&frame);
        std::vector<unsigned char> get();
        bool is_empty();
        bool is_filled();

    private:
        mutable std::mutex mtx;
        std::queue<std::vector<unsigned char>> frame_queue;
        int queue_max_size;
      
};

}