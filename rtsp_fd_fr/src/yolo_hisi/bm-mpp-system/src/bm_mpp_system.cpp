#include <iostream>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <curl/curl.h>
#include "bm_mpp.h"
#include "sample_comm.h"

using namespace bm_mpp;
using std::cout;
using std::endl;
using std::vector;
using std::shared_ptr;
using std::mutex;
using std::lock_guard;
using uchar = unsigned char;

void VENC_HandleSig(HI_S32 signo){
    if (SIGINT == signo || SIGTERM == signo){
        SAMPLE_COMM_ISP_Stop();
        SAMPLE_COMM_SYS_Exit();
        cout << "\033[0;31mprogram termination abnormally!\033[0;39m\n" << endl;
    }
    exit(HI_FAILURE);
}
void VENC_StreamHandleSig(HI_S32 signo){
    if (SIGINT == signo || SIGTERM == signo){
        SAMPLE_COMM_SYS_Exit();
        cout << "\033[0;31mprogram exit abnormally!\033[0;39m\n" << endl;
    }
    exit(HI_FAILURE);
}

long int get_jpeg_size(VENC_STREAM_S* pstStream){
    VENC_PACK_S*  pstData;
    long int filesize = 0;
    for (int i = 0; i < pstStream->u32PackCount; i++){
        pstData = &pstStream->pstPack[i];
        filesize += pstData->u32Len - pstData->u32Offset;
    }
    return filesize;
}

void get_jpeg(VENC_STREAM_S* pstStream, vector<uchar> &des){
    VENC_PACK_S* pstData;
    long int currentsize = 0;
    for (int i = 0; i < pstStream->u32PackCount; i++){
        pstData = &pstStream->pstPack[i];
        std::copy(pstData->pu8Addr + pstData->u32Offset, pstData->pu8Addr + pstData->u32Len, des.data() + currentsize);
        currentsize += pstData->u32Len - pstData->u32Offset;
    }
}

std::unique_ptr<BM_MPP_SYSTEM> BM_MPP_SYSTEM::get_instance(int queue_size){
    return std::unique_ptr<BM_MPP_SYSTEM>(new BM_MPP_SYSTEM(queue_size));
}

BM_MPP_SYSTEM::BM_MPP_SYSTEM(int queue_size){
    q = std::unique_ptr<FrameQueue>(new FrameQueue);
    q->set_queue_size(queue_size);
    init_mpp();
    runner = std::thread(&BM_MPP_SYSTEM::runner_start, this);
}

BM_MPP_SYSTEM::~BM_MPP_SYSTEM(){
    stop_recv_pic();
    SAMPLE_COMM_VENC_StopGetStream();
    runner.join();
}

void BM_MPP_SYSTEM::runner_start(){
    while(true){recv_pic();}
}

int BM_MPP_SYSTEM::init_mpp(){

    gs_encode_Norm = VIDEO_ENCODING_MODE_NTSC;
    stVbConf.u32MaxPoolCnt = 128;
    SAMPLE_COMM_VI_GetSizeBySensor(&encode_Size);

    /* video buffer */
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_encode_Norm,
                        encode_Size, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 12;

    /* step 2: mpp system init.*/
    if (HI_SUCCESS != SAMPLE_COMM_SYS_Init(&stVbConf)) 
        throw std::runtime_error("system init failed with");

    /* step 3: start vi dev & chn to capture */
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    if (HI_SUCCESS != SAMPLE_COMM_VI_StartVi(&stViConfig))
        throw std::runtime_error("start vi failed!");

    /* step 4: start vpss and vi bind vpss */
    if (HI_SUCCESS != SAMPLE_COMM_SYS_GetPicSize(gs_encode_Norm, encode_Size, &stSize)) 
        throw std::runtime_error("SAMPLE_COMM_SYS_GetPicSize failed!");
    
    // Set Vpss group
    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    if (HI_SUCCESS != SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr)) 
        throw std::runtime_error("Start Vpss failed!");

    if (HI_SUCCESS != SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode)) 
        throw std::runtime_error("Vi bind Vpss failed!");
    // Set Vpps channel
    VpssChn = 1;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = stSize.u32Width;
    stVpssChnMode.u32Height     = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

    stVpssChnAttr.s32SrcFrameRate = HI_FAILURE;
    stVpssChnAttr.s32DstFrameRate = HI_FAILURE;
    if (HI_SUCCESS != SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL)) 
        throw std::runtime_error("Enable vpss chn failed!");

    /* step 5: start stream venc */
    VpssGrp = 0;
    VpssChn = 1;
    VencChn = 1;
    if (HI_SUCCESS != SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize, HI_FALSE)) 
        throw std::runtime_error("Start snap failed!");
    if (HI_SUCCESS != SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn))     
        throw std::runtime_error("Start Venc failed!");

    cout << "BM_MPP_SYSTEM Init finish!" << endl;
}


std::vector<unsigned char> BM_MPP_SYSTEM::get(){
    return q->get();
}

int BM_MPP_SYSTEM::start_recv_venc_pic(){
    stRecvParam.s32RecvPicNum = 1;
    if (HI_SUCCESS != HI_MPI_VENC_StartRecvPicEx(VencChn, &stRecvParam)) 
        throw std::runtime_error("HI_MPI_VENC_StartRecvPic faild with");
};

int BM_MPP_SYSTEM::process_recv_pic(){
    s32VencFd = HI_MPI_VENC_GetFd(VencChn);
    if (s32VencFd < 0){
        cout << "HI_MPI_VENC_GetFd faild with " << s32VencFd << endl;
        return HI_FAILURE;
    }
    FD_ZERO(&read_fds);
    FD_SET(s32VencFd, &read_fds);
    TimeoutVal.tv_sec  = 2;
    TimeoutVal.tv_usec = 0;
    ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (ret < 0){
        cout << "snap select failed!" << endl;
        return HI_FAILURE;
    }else if (0 == ret){
        cout << "snap time out!" << endl;
        return HI_FAILURE;
    }else {
        if (FD_ISSET(s32VencFd, &read_fds)){
            if (HI_MPI_VENC_Query(VencChn, &stStat) != HI_SUCCESS){
                throw std::runtime_error("HI_MPI_VENC_StartRecvPic faild with");
            }
			if(0 == stStat.u32CurPacks){
				  cout << "NOTE: Current  frame is NULL!" << endl;
				  return HI_FAILURE;
			}
            stStream.pstPack = new VENC_PACK_S[stStat.u32CurPacks];
            stStream.u32PackCount = stStat.u32CurPacks;
            if (HI_MPI_VENC_GetStream(VencChn, &stStream, HI_FAILURE) != HI_SUCCESS){
                cout << "HI_MPI_VENC_GetStream failed with " << ret << endl;
                delete (stStream.pstPack);
                stStream.pstPack = nullptr;
                return HI_FAILURE;
            }
            /* store to queue */
            long int filesize = get_jpeg_size(&stStream);
            vector<uchar> buf(filesize);
            get_jpeg(&stStream, buf);
            q->put(std::move(buf));
            
            if (HI_MPI_VENC_ReleaseStream(VencChn, &stStream) != HI_SUCCESS){
                cout << "HI_MPI_VENC_ReleaseStream failed with " << ret << endl;
                delete (stStream.pstPack);
                stStream.pstPack = nullptr;
                return HI_FAILURE;
            }
            delete (stStream.pstPack);
            stStream.pstPack = nullptr;
        }
    }
};

int BM_MPP_SYSTEM::stop_recv_pic(){
    if (HI_MPI_VENC_StopRecvPic(VencChn) != HI_SUCCESS){
        cout << "HI_MPI_VENC_StopRecvPic failed with " << ret << endl;
        return HI_FAILURE;
    }
}

FrameQueue::FrameQueue(){
};

FrameQueue::~FrameQueue(){
};
void FrameQueue::set_queue_size(int size){
    queue_max_size = size;
}
vector<uchar> FrameQueue::get(){
    lock_guard<mutex> lock (mtx);
    if(is_empty())
        return {};
    vector<uchar> frame = std::move(frame_queue.front());
    frame_queue.pop();
    return std::move(frame);
};
bool FrameQueue::put(vector<uchar> &&frame){
    lock_guard<mutex> lock (mtx);
    if(is_filled()) return false;

    frame_queue.push(frame);
    return true;
}
bool FrameQueue::is_empty(){
    return frame_queue.size() == 0;
}
bool FrameQueue::is_filled(){
    return frame_queue.size() == queue_max_size;
}