#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <execinfo.h>
#include <signal.h>
#include <algorithm>
#include <fstream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <sys/syscall.h>
#include <sys/resource.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "bm_usbtty.hpp"
#include "bmiva.hpp"
#include "bm_storage.hpp"
#include "usbttycommon.h"
#include "bmiva_face.hpp"
#include "base64.hpp"
#include "bmiva_util.hpp"
#include "log_common.h"
#include "Config.hpp"
#include "ipc_configs.h"
#include "bm_link.h"

#include "bm_cquene.hpp"

#define gettid() syscall(__NR_gettid)

using namespace std;
int local_id, remote_id;

static UsbTTyDevice *pDevice = NULL;
static BMCircularBuf *pcb_FDInfo = NULL;
static BMCircularBuf *pcb_Frame = NULL;

vector<string> models;
vector<string> extractor_models;

// handle
bmiva_face_handle_t g_handle;
bmiva_face_handle_t g_Reghandle;

bmiva_dev_t g_dev;
string g_feature_file;
BmivaFeature g_name_features;

typedef struct
{
	FILE *pfile;
	unsigned int fwtype=0;
	unsigned int filenums=0;
	unsigned int filesize=0;
	unsigned int subfileid=0;
	unsigned int received_size=0;
}Upgrade_Info;

Upgrade_Info UpgradeInfo={0};

#if 0
void SendBuf(unsigned char *pBuf, int size, IpcRetCode code)
{
	unsigned char cmd[] = "xxxx";
	DEBUG_LOG("SendBuf %d\n", code);
	pDevice->SendBufPktHeader(cmd, size, FORMAT_MISC, code);
	pDevice->SendBufPkt(pBuf, size);
}
#endif

#ifdef BMLINK
uint16_t SendUSBData(uint32_t size, uint16_t ret, uint8_t *pdata)
{
	uint16_t ret_size;
	bm_usb_header_tx_pkt_2 sendpkt;

	sendpkt.data_len = size;
	sendpkt.ret_code = ret;

	ret_size = BMLinkSessionWrite(local_id, remote_id, &sendpkt, PACKET_HEADER_SIZE, 0, 0, BMLINK_PAYLOAD_BINARY);

	if(ret_size!=PACKET_HEADER_SIZE) {
		printf("usb write error!!!!!!\n");
		return FAILED;
	}
	else if((size>0) && (pdata!=NULL)) {
		ret_size = BMLinkSessionWrite(local_id, remote_id, pdata, size, 0, 0, BMLINK_PAYLOAD_BINARY);

		if(ret_size!=size) {//usb session bug
			printf("Send USB data error!!!(%d!=%d)\n", ret_size, size);
			return FAILED;
		}
	}

	return SUCCESS;
}
#else
uint16_t SendUSBData(uint32_t size, uint16_t ret, uint8_t *pdata)
{
	uint16_t ret_size;
	bm_usb_header_tx_pkt_2 sendpkt;

	sendpkt.data_len = size;
	sendpkt.ret_code = ret;

	//ret_size = BMLinkSessionWrite(local_id, remote_id, &sendpkt, PACKET_HEADER_SIZE, 0, 0, BMLINK_PAYLOAD_BINARY);
	ret_size = pDevice->SendBufPkt((unsigned char*)&sendpkt, PACKET_HEADER_SIZE);

	if(ret_size!=PACKET_HEADER_SIZE) {
		printf("usb write error!!!!!!(%d!=%d)\n", ret_size, PACKET_HEADER_SIZE);
		return FAILED;
	}
	else if((size>0) && (pdata!=NULL)) {
		//ret_size = BMLinkSessionWrite(local_id, remote_id, pdata, size, 0, 0, BMLINK_PAYLOAD_BINARY);
		ret_size = pDevice->SendBufPkt(pdata, size);

		if(ret_size!=size) {//usb session bug
			printf("Send USB data error!!!(%d!=%d)\n", ret_size, size);
			return FAILED;
		}
	}

	return SUCCESS;
}
#endif

#if 0//test
void SendFaceInfo(vector<bmiva_face_info_t> &faceInfo, IpcRetCode code)
{
	int size = faceInfo.size();
	int unitsize = sizeof(bmiva_face_info_t);

	unsigned char *pOutBuf = (unsigned char *)malloc(size * unitsize);
	unsigned char *pBuf = pOutBuf;

	for (int i = 0; i < size; i++)
	{

		memcpy(pBuf, &faceInfo[i], unitsize);
		pBuf += unitsize;
	}

	SendBuf(pOutBuf, size * unitsize, code);
	free(pOutBuf);
}
#endif

void draw_frame(cv::Mat *image, vector<vector<bmiva_face_info_t> > &results)
{
	int size = results[0].size();
	for (int i = 0; i < size; i++)
	{
		bmiva_face_info_t face = results[0][i];
		float x1 = face.bbox.x1;
		float y1 = face.bbox.y1;
		float x2 = face.bbox.x2;
		float y2 = face.bbox.y2;

		cv::rectangle(*image, cv::Point(x1, y1),
				cv::Point(x2, y2), cv::Scalar(255, 255, 0), 3, 8, 0);
		int pos_x = std::max(int(x1 - 5), 0);
		int pos_y = std::max(int(y1 - 10), 0);
		cv::putText(*image, face.bbox.name, cv::Point(pos_x, pos_y),
				cv::FONT_HERSHEY_PLAIN, 4.0, CV_RGB(255, 255, 0), 2.0);

		bmiva_face_pts_t facePts = face.face_pts;
		for (int j = 0; j < 5; j++)
		{
			cv::circle(*image, cv::Point(facePts.x[j], facePts.y[j]),
					1, cv::Scalar(255, 255, 0), 2);
		}
	}
}

static void _do_detect(bmiva_face_handle_t detector,
		vector<cv::Mat> &images,
		vector<vector<bmiva_face_info_t> > &results)
{
	bmiva_face_detector_detect(detector, images, results);
	int sum = 0;
	stringstream output;
	for (size_t i = 0; i < results.size(); i++)
	{
		for (size_t j = 0; j < results[i].size(); j++)
		{
			sum++;
			output << "     face " << j + 1 << ": ("
				<< results[i][j].bbox.x1 << ", "
				<< results[i][j].bbox.y1 << "), ("
				<< results[i][j].bbox.x2 << ", "
				<< results[i][j].bbox.y2 << ")"
				<< endl;
		}
	}
	output << "     Total " << sum << " faces detected.";
	cout << output.str() << endl;
}

void enable_core_dump()
{
	system("echo core-%e-%p-%t > /proc/sys/kernel/core_pattern");
	struct rlimit rl;
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &rl) == -1)
	{
		cout << "setrlimit failed." << endl;
	}
}

void signal_handler(int signum, siginfo_t *info, void *context)
{
	int nptrs;
	void *buffer[100];
	char **strings;

	// dump signal information
	const char *signal_name = "???";
	bool has_address = false;
	switch (signum)
	{
		case SIGABRT:
			signal_name = "SIGABRT";
			break;
		case SIGBUS:
			signal_name = "SIGBUS";
			has_address = true;
			break;
		case SIGFPE:
			signal_name = "SIGFPE";
			has_address = true;
			break;
		case SIGILL:
			signal_name = "SIGILL";
			has_address = true;
			break;
		case SIGSEGV:
			signal_name = "SIGSEGV";
			has_address = true;
			break;
		default:
			cout << "Unknown signal(" << signum << ")" << endl;
			break;
	}

	char code_desc[32];
	char addr_desc[32];
	addr_desc[0] = code_desc[0] = 0;
	if (info != nullptr)
	{
		snprintf(code_desc, sizeof(code_desc), ", code %d", info->si_code);
		if (has_address)
		{
			snprintf(addr_desc, sizeof(addr_desc), ", fault addr %p", info->si_addr);
		}
	}

	cout << "Fatal signal " << signum << " (" << signal_name << ")"
		<< code_desc << addr_desc << " in tid " << gettid() << endl;

	// dump stack trace
	nptrs = backtrace(buffer, 100);
	cout << "backtrace: returned " << nptrs << " addresses" << endl;

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL)
	{
		perror("backtrace_symbols");
		exit(0);
	}

	char prefix[32];
	for (int i = 0; i < nptrs; i++)
	{
		snprintf(prefix, sizeof(prefix), "#%02d ", i);
		cout << prefix << strings[i] << endl;
	}

	// Reset the signal handler as default
	signal(signum, SIG_DFL);

	// flush IO before exiting.
	cout << flush;

	_exit(signum);
}

void register_signal_handler()
{

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	sigfillset(&action.sa_mask);
	action.sa_sigaction = signal_handler;
	action.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGABRT, &action, nullptr);
	sigaction(SIGBUS, &action, nullptr);
	sigaction(SIGFPE, &action, nullptr);
	sigaction(SIGILL, &action, nullptr);
	sigaction(SIGSEGV, &action, nullptr);
}

int init_face_detector(const char *model_dir, int batch, bool flag)
{
	if (!DirExist(model_dir))
	{
		LOGE << "Cannot find valid model dir";
		return -1;
	}

	register_signal_handler();

	string ssh(model_dir), onet(model_dir);
	ssh.append(SSH_MODEL_CFG_FILE);
	onet.append(ONET_MODEL_CFG_FILE);
	models.push_back(ssh);
	models.push_back(onet);

	string ext(model_dir);
	ext.append(EXTRACTOR_MODEL_CFG_FILE);
	extractor_models.push_back(ext);

	bmiva_init(&g_dev);
	bmiva_face_handle_t *detector = &g_handle;
	bmiva_face_detector_create(detector, g_dev, models, BMIVA_FD_TINYSSH);
	bmiva_face_extractor_create(&g_Reghandle, g_dev, extractor_models, BMIVA_FR_BMFACE);

	return 0;
}

void ttyserverhandler(int); /* Ctrl-C handler           */

typedef struct usbttyCmd
{
	char cmdName[128];
	int cmdLen;
} usbttyCmd_t;

#define TTYCMD_FACE_DETECT "face-detect"
#define TTYCMD_FACE_RECONG "face-recong"

usbttyCmd_t cmds_table[] = {
	{TTYCMD_FACE_DETECT, strlen(TTYCMD_FACE_DETECT)},
	{TTYCMD_FACE_RECONG, strlen(TTYCMD_FACE_RECONG)},
};

/* ---------------------------------------------------------------- */
/* FUNCTION  INThandler:                                            */
/*    This function handles the SIGINT (Ctrl-C) signal.             */
/* ---------------------------------------------------------------- */

void ttyserverhandler(int sig)
{
	char c;

	signal(sig, SIG_IGN); /* disable Ctrl-C           */

	DEBUG_LOG("Do you really want to quit? [y/n] ");
	c = getchar(); /* read an input character  */

	if (c == 'y' || c == 'Y') /* if it is y or Y, then    */
	{
		if (pDevice)
		{
			delete pDevice;
			pDevice = NULL;
		}
		exit(0); /* exit.  Otherwise,        */
	}
	else
	{
		signal(SIGINT, ttyserverhandler); /* reinstall the handler    */
	}
}

void decode_image(vector<cv::Mat> &images, unsigned char *image, unsigned int length)
{
	cv::Mat image0;
	std::vector<char> pic(image, image + length);
	cv::imdecode(pic, cv::IMREAD_COLOR, &image0);

	DEBUG_LOG("decode_image jpg size %d %d\n", image0.rows, image0.cols);

	if (image0.rows < 720 && image0.cols < 1280)
	{
		cv::Mat dst;
		int bottom = (720 - image0.rows);
		int right = (1280 - image0.cols);
		cv::copyMakeBorder(image0, dst, 0, bottom, 0, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
		images.push_back(dst);
	}
	else
	{
		images.push_back(image0);
	}
}

int do_register(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	DEBUG_LOG("do_register!!!!\n");

	int ret;
	uint16_t ret_size;
	vector<cv::Mat> images;
	vector<vector<bmiva_face_info_t> > results;

	BitMainUsr *puser = (BitMainUsr*) image.data;
	uint8_t *pdata = image.data + sizeof(BitMainUsr);
	uint32_t pdata_size = rcvpkt.data_len - sizeof(BitMainUsr);
	printf("user_id=%d, user_name=%s\n", puser->user_id, puser->user_name, puser->user_info);

	if (rcvpkt.data_type == FORMAT_JPEG)
	{
		printf("rcvpkt.data_len=%d, image size=%d\n", rcvpkt.data_len, pdata_size);
		decode_image(images, image.data+sizeof(BitMainUsr), pdata_size);
	}
	else
	{
		images.push_back(image);
		DEBUG_LOG("do_register raw size %d %d\n", image.rows, image.cols);
	}

	_do_detect(g_handle, images, results);

	DEBUG_LOG("do_register detect done got %d faces\n", results[0].size());

	if (results.empty() || results[0].empty())
	{
		SendUSBData(0, NO_FACE, NULL);
		return NO_FACE;
	}

	cv::Mat aligned;
	if (face_align(*images.begin(), aligned, results[0].at(0), FACE_RECOG_IMG_W, FACE_RECOG_IMG_H) != 0)
	{
		fprintf(stderr, "do_register face_align failed\n");
		SendUSBData(0, DETECT_FAIL, NULL);
		return DETECT_FAIL;
	}

	vector<cv::Mat> align_images;
	vector<vector<float> > features;
	align_images.push_back(aligned);

	bmiva_face_extractor_predict(g_Reghandle, align_images, features);

	printf("user id=%d, name=[%s], info=[%s]\n", puser->user_id, puser->user_name, puser->user_info);

	ret = BmStorage::get_instance().add_user(puser->user_id, puser->user_name,  puser->user_info, *(images.begin()), features[0]);

	if(ret==RET_ERROR)
		printf("add_user failed\n");
	else if(ret==RET_OK)
		printf("add_user OK\n");
	else
		printf("add_user unknown issue\n");

	SendUSBData(0, OK, NULL);

	return OK;
}


void parse_timestamp(uint64_t timestamp)
{
#if defined(DEBUG_MSG)
	//unsigned int half = *((unsigned char *)(&timestamp));
	unsigned int sec = (timestamp & SEC_MASK);
	unsigned int min = (timestamp & MIN_MASK) >> 6;
	unsigned int hour = (timestamp & HOUR_MASK) >> 12;
	unsigned int day = (timestamp & DAY_MASK) >> 17;
	unsigned int month = (timestamp & MONTH_MASK) >> 22;
	unsigned int year = ((timestamp & YEAR_MASK) >> 26) + 2000;
	DEBUG_LOG("time %d-%d-%d %d:%d:%d\n", year, month, day, hour, min, sec);
#endif
}

int check_event_interval(uint64_t latest, uint64_t now)
{
	unsigned int sec_latest = (latest & SEC_MASK);
	unsigned int sec_now = (now & SEC_MASK);
	//printf("time interval = %d\n", sec_now - sec_latest);

	return sec_now - sec_latest;
}

int do_recognize_with_queue(void)
{

	vector<cv::Mat> images;
	images.clear();

	uint64_t event_time;
	unsigned char* image_data = pcb_Frame->GetFromQuene(&event_time);

	if(image_data==NULL)
		return NO_FACE;

	DEBUG_LOG("\n\ndo_recognize_2, time=%ld\n", event_time);
	parse_timestamp(event_time);

	cv::Mat yuvImage = cv::Mat(720 * 3 / 2, 1280, CV_8UC1, image_data);
	cv::Mat rgbImage(720, 1280, CV_8UC3);
	cvtColor(yuvImage, rgbImage, CV_YUV2BGR_NV21, 3);
	images.push_back(rgbImage);


	vector<vector<bmiva_face_info_t> > results;
	_do_detect(g_handle, images, results);

	float sim_threshold = 0.55;
	if (results[0].empty())
	{
		//SendUSBData(0, NO_FACE, NULL);
		return NO_FACE;
	}

	vector<bmiva_face_info_t> faceInfo = results[0];

	vector<vector<float> > features;
	vector<cv::Mat> align_images;
	faceInfo = results[0];

	int found_user_count = 0;
	unsigned int buffer[3 + MAX_USERS]={0};
	unsigned int *user_ids = &buffer[3];

	BMFDResult *pFDResult = (BMFDResult *)malloc(sizeof(BMFDResult));

	for (size_t i = 0; i < faceInfo.size(); i++)
	{
		cv::Mat aligned(FACE_RECOG_IMG_W, FACE_RECOG_IMG_H, images.begin()->type());
		if (face_align(*images.begin(), aligned, faceInfo[i], FACE_RECOG_IMG_W, FACE_RECOG_IMG_H) != 0)
		{
			continue;
		}

		features.clear();
		align_images.clear();
		align_images.push_back(aligned);

		bmiva_face_extractor_predict(g_Reghandle, align_images, features);

		if (features.empty() || features[0].empty())
		{
			fprintf(stderr, "bmiva_face_extractor_predict got empty results\n");
			break;
		}

		unsigned int get_id;
		float similarity;
		BmStorage::get_instance().match_feature(features[0], get_id, similarity);
		printf("FR: id=%d, sim=%f\n", get_id, similarity);

		if(similarity<=sim_threshold)
			continue;

		long long latest_time;
		if(RET_ERROR == BmStorage::get_instance().get_latest_event_time(get_id, &latest_time)) {//not good method
			BmStorage::get_instance().add_event(get_id, event_time);//first time
			printf("FR: add event first time id=%d(%lld)\n", get_id, event_time);
		}

		if(event_time - latest_time >= 10) {//10 sec
			BmStorage::get_instance().add_event(get_id, event_time);
			printf("FR: add event id=%d(%lld)\n", get_id, event_time);
		}
		else
			printf("FR: not add event becuase time interval less than 10sec(%lld)\n", event_time);

		if(found_user_count<MAX_USERS) {
			BitMainUsr info;
			BmStorage::get_instance().get_info(get_id, info);
			printf("%d: get_id=%d\n", found_user_count, get_id);
			pFDResult->user_ids[found_user_count++] = get_id -1;
		}
	}

	pFDResult->reg_user_num = found_user_count;
	pFDResult->unknown_num = faceInfo.size() - found_user_count;
	pFDResult->confidence = 100;

	pcb_FDInfo->AddtoQuene((unsigned char*)pFDResult, 0);

	free(pFDResult);

	return OK;
}


int do_recognize(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	vector<cv::Mat> images;
	images.clear();

#if 1//defined(PROFILE_PERFORMANCE)
	struct timeval timestamp1;
	gettimeofday(&timestamp1, NULL);
#endif

	uint64_t event_time = rcvpkt.timestamp;

	DEBUG_LOG("\n\ndo_recognize size %d type=%d, time=%lld\n", rcvpkt.data_len, rcvpkt.data_type, rcvpkt.timestamp);
	parse_timestamp(event_time);

	if (rcvpkt.data_type == FORMAT_JPEG)
	{
		//printf("jpg mode\n");
		cv::Mat image0;
		std::vector<char> pic(image.data, image.data + rcvpkt.data_len);
		cv::imdecode(pic, CV_LOAD_IMAGE_COLOR, &image0);
		DEBUG_LOG("jpg %d x %d\n", image0.rows, image0.cols);
		images.push_back(image0);
	}
	else if (rcvpkt.data_type == FORMAT_YUV420)
	{
		//printf("yuv mode\n");
		cv::Mat yuvImage = cv::Mat(720 * 3 / 2, 1280, CV_8UC1, image.data);
		cv::Mat rgbImage(720, 1280, CV_8UC3);
		cvtColor(yuvImage, rgbImage, CV_YUV2BGR_NV21, 3);
		images.push_back(rgbImage);
	}
	else
	{
		//printf("raw mode\n");
		images.push_back(image);
		DEBUG_LOG("raw %d x %d\n", image.rows, image.cols);
	}

#if 1//defined(PROFILE_PERFORMANCE)
	struct timeval timestamp11;
	gettimeofday(&timestamp11, NULL);
	float costime11 = (timestamp11.tv_sec - timestamp1.tv_sec) * 1000000.0 + timestamp11.tv_usec - timestamp1.tv_usec;
	DEBUG_LOG("do_recognize 1.1 cost %f\n", costime11);
#endif

	vector<vector<bmiva_face_info_t> > results;
	_do_detect(g_handle, images, results);

#if 1//defined(PROFILE_PERFORMANCE)
	struct timeval timestamp12;
	gettimeofday(&timestamp12, NULL);
	float costime12 = (timestamp12.tv_sec - timestamp11.tv_sec) * 1000000.0 + timestamp12.tv_usec - timestamp11.tv_usec;
	DEBUG_LOG("do_recognize 1.2 cost %f\n", costime12);
#endif

	float sim_threshold = 0.55;
	if (results[0].empty())
	{
		SendUSBData(0, NO_FACE, NULL);
		return NO_FACE;
	}

	vector<bmiva_face_info_t> faceInfo = results[0];

	vector<vector<float> > features;
	vector<cv::Mat> align_images;
	faceInfo = results[0];

	int found_user_count = 0;
	// corresponding to the parsing of btmain.c recognize_image
	unsigned int buffer[3 + MAX_USERS]={0};
	unsigned int *user_ids = &buffer[3];

	BMFDResult *pFDResult = (BMFDResult *) image.data;

	for (size_t i = 0; i < faceInfo.size(); i++)
	{
		cv::Mat aligned(FACE_RECOG_IMG_W, FACE_RECOG_IMG_H, images.begin()->type());
		if (face_align(*images.begin(), aligned, faceInfo[i], FACE_RECOG_IMG_W, FACE_RECOG_IMG_H) != 0)
		{
			continue;
		}

		features.clear();
		align_images.clear();
		align_images.push_back(aligned);

		bmiva_face_extractor_predict(g_Reghandle, align_images, features);

		if (features.empty() || features[0].empty())
		{
			fprintf(stderr, "bmiva_face_extractor_predict got empty results\n");
			break;
		}

		unsigned int get_id;
		float similarity;
		BmStorage::get_instance().match_feature(features[0], get_id, similarity);
		printf("FR: id=%d, sim=%f\n", get_id, similarity);

		if(similarity<=sim_threshold)
			continue;

#if defined(DEBUG_MSG)
		for (int i = 0; i < 512; ++i)
		{
			DEBUG_LOG("%f ", features[0][i]);
		}
		DEBUG_LOG("\n");
#endif

		long long latest_time;
		if(RET_ERROR == BmStorage::get_instance().get_latest_event_time(get_id, &latest_time)) {//not good method
			BmStorage::get_instance().add_event(get_id, event_time);//first time
			printf("FR: add event first time id=%d(%lld)\n", get_id, event_time);
		}

		if(event_time - latest_time >= 10) {//10 sec
			BmStorage::get_instance().add_event(get_id, event_time);
			printf("FR: add event id=%d(%lld)\n", get_id, event_time);
		}
		else
			printf("FR: not add event becuase time interval less than 10sec(%lld)\n", event_time);

		if(found_user_count<MAX_USERS) {
			BitMainUsr info;
			BmStorage::get_instance().get_info(get_id, info);
			printf("%d: get_id=%d\n", found_user_count, get_id);
			pFDResult->user_ids[found_user_count++] = get_id -1;
		}
	}

	pFDResult->reg_user_num = found_user_count;
	pFDResult->unknown_num = faceInfo.size() - found_user_count;
	pFDResult->confidence = 100;

	uint32_t send_size = sizeof(BMFDResult);
	SendUSBData(send_size, OK, image.data);

#if 1//defined(PROFILE_PERFORMANCE)
	struct timeval timestamp2;
	gettimeofday(&timestamp2, NULL);
	float costime = (timestamp2.tv_sec - timestamp1.tv_sec) * 1000000.0 + timestamp2.tv_usec - timestamp1.tv_usec;
	DEBUG_LOG("do_recognize cost %f\n", costime);
#endif
	return OK;
}


int do_delete(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	int ret;

	BitMainUsr *puser = (BitMainUsr*) image.data;
	printf("remove user_id=%d\n", puser->user_id);

	StorageRetCode ret_code = BmStorage::get_instance().delete_user(puser->user_id);

	if (ret_code == RET_ID_NOT_EXIST)
		SendUSBData(0, USER_ID_NOT_EXIST, NULL);
	else
		SendUSBData(0, OK, NULL);

	return ret;
}

int do_search(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	BMEvent *pevent = (BMEvent *) image.data;
	DEBUG_LOG("do_search %d(%lld ~ %lld)\n", pevent->user_id, pevent->start_time, pevent->end_time);

	unsigned int event_count = 0;

	uint64_t *events = (uint64_t *)image.data;
	StorageRetCode ret = BmStorage::get_instance().search_event(pevent->user_id, MAX_EVENT_COUNT, pevent->start_time, pevent->end_time, event_count, (long long*)events);

	DEBUG_LOG("do_search got %d events, sizeof(uint64_t)=%d\n", event_count, sizeof(uint64_t));
	//printf("event=%lld, %lld\n", events[0], events[1]);

	uint32_t send_size= event_count * sizeof(uint64_t);
	SendUSBData(send_size, OK, image.data);
	return OK;
}


int do_enumerate(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	unsigned int user[128];
	unsigned int count;
	BmStorage::get_instance().list_users(count, user);
	unsigned int *user_ids = (unsigned int *)malloc(count * sizeof(unsigned int));
	StorageRetCode ret = RET_OK;

	BitMainUsr *users = (BitMainUsr *)image.data;

	for (unsigned int i = 0; i < count; ++i)
	{
		*user_ids = user[i];//need to fix
		ret = BmStorage::get_instance().get_info(*(user_ids), *(users + i));
		*(user_ids) -= 1;
		printf("%d: [%s]/[%s]\n", *(user_ids), (users+i)->user_name, (users+i)->user_info);
	}

	uint32_t send_size = count*sizeof(BitMainUsr);
	SendUSBData(send_size, OK, image.data);
	free(user_ids);
	return OK;
}

int do_get_image(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	unsigned int user_id;
	unsigned int image_size;
	unsigned char *buffer=NULL;
	char image_path[32]={0};
	FILE *pfile;


	BitMainUsr *puser = (BitMainUsr*) image.data;
	user_id = puser->user_id;

	sprintf(image_path, "/home/%d.jpg", user_id);
	pfile = fopen(image_path, "rb");

	if(NULL == pfile) {
		printf("fopen failed(%s)\n", image_path);
		SendUSBData(0, STORAGE_FULL, NULL);
		//image.data[OFFSET__RETCODE] = BAD_INPUT;
		return BAD_INPUT;
	}

	fseek(pfile, 0L, SEEK_END);
	image_size = ftell(pfile);
	fseek(pfile, 0L, SEEK_SET);
	buffer = (unsigned char *)malloc(image_size);

	fread(buffer, image_size, 1, pfile);
	memcpy(image.data, (unsigned char *)buffer, image_size);

	free(buffer);

	fclose(pfile);

	uint32_t send_size = image_size;
	SendUSBData(send_size, OK, image.data);

	return OK;
}

int do_receive_file(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{

	int ret, i;
	int write_size;
	int file_size=0;
	unsigned char checksum=0;
	BMSendFiles *pdata = (BMSendFiles *)image.data;

	uint8_t checksum_get = pdata->checksum;
	uint32_t getsize = pdata->size;
	uint32_t continue_send = pdata->continue_send;

	if(NULL == UpgradeInfo.pfile) {
		printf("open file failed\n");
		SendUSBData(0, RECV_ERROR, NULL);
		return BAD_INPUT;//need to fix
	} else {
		write_size = fwrite(image.data + pdata->data_offset, 1, getsize, UpgradeInfo.pfile);
		printf("write %d(%d)bytes\n", write_size, getsize);
		if(write_size!=getsize) {
			printf("write file error!!!!!!\n");
			SendUSBData(0, RECV_ERROR, NULL);
			fclose(UpgradeInfo.pfile);
			return BAD_INPUT;//need to fix
		}
		UpgradeInfo.received_size += getsize;
	}

#if 0//for checksum
	//small file or big file end
	if(continue_send==SEND_ONETIME || continue_send==SEND_MULTI_END) {
		UpgradeInfo.pfile = fopen("download.bin", "rb");

		//get checksum
		fseek(UpgradeInfo.pfile, 0L, SEEK_END);
		file_size = ftell(UpgradeInfo.pfile);
		fseek(UpgradeInfo.pfile, 0L, SEEK_SET);

		for(i=0; i<file_size; i++) {
			checksum ^= fgetc(UpgradeInfo.pfile);
		}

		fclose(UpgradeInfo.pfile);

		if(checksum != checksum_get) {
			printf("checksum error(%x!=%x)!!!\n", checksum, checksum_get);
			image.data[OFFSET__RETCODE] = RECV_ERROR;
			return BAD_INPUT;//need to fix
		}
		//printf("checksum = %x\n", checksum);
	}
#endif

	SendUSBData(0, OK, NULL);
	return OK;
}

int do_get_version(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	BMFWVersion *pdata = (BMFWVersion *)image.data;
	printf("do_get_version : fw_type = %d\n", pdata->fw_type);

	pdata->ptime.tm_year = 118;
	pdata->ptime.tm_mon = 12;
	pdata->ptime.tm_mday = 13;

	if(pdata->fw_type == BitMain_BspVer)
		strcpy(pdata->version, XM_BSP_PACKAGE_ID);
	else if(pdata->fw_type == BitMain_AppVer)
		strcpy(pdata->version, XM_APP_PACKAGE_ID);
	else if(pdata->fw_type == BitMain_BModelVer)
		strcpy(pdata->version, XM_MODEL_PACKAGE_ID);


	uint32_t send_size = sizeof(BMFWVersion);
	SendUSBData(send_size, OK, image.data);

	return 0;
}

int do_upgrade_init(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{

	BMUpgradeInit *pdata = (BMUpgradeInit *)image.data;

	UpgradeInfo.filenums = pdata->filenums;
	UpgradeInfo.filesize = pdata->filesize;
	UpgradeInfo.fwtype = pdata->fwtype;
	UpgradeInfo.received_size = 0;

	printf("fwtype=%d, filenums=%d, filesize=%d\n", UpgradeInfo.fwtype, UpgradeInfo.filenums, UpgradeInfo.filesize);

	uint32_t send_size = sizeof(BMUpgradeInit);
	SendUSBData(send_size, OK, image.data);
	return 0;

}

int do_upgrade_subfileid(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	BMSubFiles *pdata = (BMSubFiles *)image.data;

	UpgradeInfo.subfileid = pdata->subfileid;
	printf("new subfileid=%d\n", UpgradeInfo.subfileid);

	char fname[32]={0};

	sprintf(fname, "download.bin%d", UpgradeInfo.subfileid);
	printf("open file %s\n", fname);
	UpgradeInfo.pfile = fopen(fname, "wb");

	SendUSBData(0, OK, NULL);
	return 0;

}

int do_upgrade_subeof(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{
	fclose(UpgradeInfo.pfile);

	BMSubFiles *psubfiles = (BMSubFiles*) image.data;

	psubfiles->subfileid = UpgradeInfo.subfileid;
	psubfiles->percentage = 100*UpgradeInfo.received_size/UpgradeInfo.filesize;


	printf("subfileid=%d, filenums=%d\n", UpgradeInfo.subfileid, UpgradeInfo.filenums);

	uint32_t send_size = sizeof(BMSubFiles);

	if((UpgradeInfo.subfileid+1) == UpgradeInfo.filenums)
		SendUSBData(send_size, READY_DO_UPGRADE, image.data);
	else
		SendUSBData(send_size, OK, image.data);

	return 0;

}

int do_upgrade_abort(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{

	printf("###do upgrade force about: do nothing now!!!!!\n");

	SendUSBData(0, OK, NULL);
	return 0;
}

int do_device_upgrade(cv::Mat &image, bm_usb_header_tx_pkt_2 &rcvpkt)
{

	printf("###do device upgrade now!!!!!\n");
	printf("###disable for test!!!!!\n");

	SendUSBData(0, OK, NULL);
#if 0//not ready
	if(UpgradeInfo.fwtype == BitMain_AppVer) {
		printf("Device upgrade APP part!!!!\n");
		system("/system/data/ota/ota_app.sh");
	}
	else if(UpgradeInfo.fwtype == BitMain_BspVer) {
		printf("Device upgrade BSP part!!!!\n");
		system("/system/data/ota/ota_bsp.sh");
	}
	else {
		printf("Not support yet!\n");
		length = PAYLOAD_LENGTH;
		image.data[OFFSET__RETCODE] = OK;
		return 0;
	}

	FILE *fstatus;
	char status;

	fstatus = fopen("/tmp/upgrade_status", "rb");

	if(NULL != fstatus) {
		status = fgetc(fstatus);
		printf("upgrade status = %c\n", status);
		fclose(fstatus);
	}

	length = PAYLOAD_LENGTH;

	if(status=='0')
		image.data[OFFSET__RETCODE] = UPGRADE_FAILED;
	else if(status=='1')
		image.data[OFFSET__RETCODE] = OK;
#endif
	return 0;
}

void *bm_cmd_server_thread(void *arg)
{
	printf("Start %s\n", __FUNCTION__);

	bmlink_t *link = (bmlink_t *)arg;
	int ret;
	int send_size;
	int img_size = 1280 * 720 * 3;
	cv::Mat image(720, 1280, CV_8UC3);

	if (arg == NULL)
		return NULL;

#ifdef BMLINK
	local_id = BMLINK_SERVER_SESSION;
	ret = BMLinkSessionOpen(link, 1, &local_id);
	if (ret != 0)
		cout << endl << "BMLinkSessionOpen failed" << endl;

	while (link->valid)
#else
		pDevice->Flush();

	while (pDevice->IsOpened())
#endif
	{
		bm_usb_header_tx_pkt_2 rcvpkt;

#ifdef BMLINK
		ret = BMLinkSessionRead(local_id, &remote_id, &rcvpkt, sizeof(bm_usb_header_tx_pkt_2), -1);

		printf("ret=%d, data_len=%d, cmd=%d, data_type=%d\n", ret, rcvpkt.data_len, rcvpkt.cmd, rcvpkt.data_type);

		if(ret>0 && rcvpkt.data_len>0) {
			ret = BMLinkSessionRead(local_id, &remote_id, image.data, rcvpkt.data_len, -1);
			if(ret<rcvpkt.data_len) {
				printf("read packet failed, ret=%d(%d)\n", ret, rcvpkt.data_len);
				continue;
			}
		}
#else
		pDevice->ValidateRecvBufPkt((unsigned char *)&rcvpkt, sizeof(bm_usb_header_tx_pkt_2), 3);

		printf("data_len=%d, cmd=%d, data_type=%d\n", rcvpkt.data_len, rcvpkt.cmd, rcvpkt.data_type);

		if(rcvpkt.data_len>0) {
			pDevice->ValidateRecvBufPkt(image.data, rcvpkt.data_len, 3);
		}

#endif

		if (rcvpkt.cmd == CMD_REGISTER_USER)
		{
			ret = do_register(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_RECOGNIZE_IMAGE)
		{
#ifdef FRAME_QUEUE
			pcb_Frame->AddtoQuene(image.data, rcvpkt.timestamp);

			if(!pcb_FDInfo->QueneisEmpty()) {//asynchronous for saving usb transfer loading

				SendUSBData(sizeof(BMFDResult), OK, pcb_FDInfo->GetFromQuene(NULL));
				pcb_FDInfo->DelFromQuene();
			}
			else
				SendUSBData(0, NO_FACE, NULL);

			ret = OK;
#else
			ret = do_recognize(image, rcvpkt);
#endif
		}
		else if (rcvpkt.cmd == CMD_DELETE_USER)
		{
			ret = do_delete(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_SEARCH_USER)
		{
			ret = do_search(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_ENUMERATE_USER)
		{
			ret = do_enumerate(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_GET_IMAGE)
		{
			ret = do_get_image(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_SEND_FILE)
		{
			ret = do_receive_file(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_GET_FW_VER)
		{
			ret = do_get_version(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_UPGRADE_INIT)
		{
			ret = do_upgrade_init(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_UPGRADE_SUBFILEID)
		{
			ret = do_upgrade_subfileid(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_UPGRADE_SUBEOF)
		{
			ret = do_upgrade_subeof(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_UPGRADE_ABORT)
		{
			ret = do_upgrade_abort(image, rcvpkt);
		}
		else if (rcvpkt.cmd == CMD_DEVICE_UPGRADE)
		{
			ret = do_device_upgrade(image, rcvpkt);
		}
		else
		{
			fprintf(stderr, "unsupported cmd\n");
			continue;
		}

	}

	BMLinkSessionClose(local_id);

	return NULL;
}

void *video_frame_thread(void *arg)
{

	printf("Start %s\n", __FUNCTION__);

	while(1) {

		if(!pcb_Frame->QueneisEmpty()) {
			do_recognize_with_queue();
			pcb_Frame->DelFromQuene();
		}

		usleep(1);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	int ret;
	bmlink_t link;

	if (argc < 2)
	{
		cout << "Usage: " << argv[0] << " devicename " << endl;
		exit(1);
	}

	signal(SIGINT, ttyserverhandler); /* install Ctrl-C handler   */

#ifdef FRAME_QUEUE
	pcb_FDInfo = new BMCircularBuf(sizeof(BMFDResult));
	pcb_Frame = new BMCircularBuf(1280*720*3);
#endif

	printf("v1.0\n");

	printf("do handshake with usb host\n");
	connect_to_host();

#ifdef BMLINK
	ret = BMLinkSingleInit(2, 2, 2, 2);
	if (ret != 0)
		cout << endl << "BMLinkSingleInit failed" << endl;

	ret = BMLinkOpen(argv[1], BMLINK_TTYUSB, NULL, &link);
	if (ret != 0)
		cout << endl << "BMLinkOpen failed" << endl;
#else
	pDevice = new UsbTTyDevice(argv[1], 4000000);
#endif

	init_face_detector("/usr/data", 4, 0);

#ifdef FRAME_QUEUE
	//thread for recognized frame
	pthread_t tid_frame;
	pthread_create(&tid_frame, NULL, video_frame_thread, NULL);
#endif


	//thread for cmd server
	pthread_t tid_cmd;
	pthread_create(&tid_cmd, NULL, bm_cmd_server_thread, (void *)&link);
	pthread_join(tid_cmd, NULL);


	bmiva_face_extractor_destroy(g_Reghandle);
	bmiva_face_detector_destroy(g_handle);
	bmiva_deinit(g_dev);

#ifdef FRAME_QUEUE
	delete pcb_FDInfo;
	delete pcb_Frame;
#endif

	return 0;
}
