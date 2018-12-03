#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "bmiva.hpp"
#include "bmiva_util.hpp"
#include "bmiva_face.hpp"
#include "common_config.h"
#include "face_det.h"

using namespace std;
using namespace cv;


vector<string> models;
vector<string> extractor_models;
bmiva_face_algorithm_t algo = BMIVA_FD_TINYSSH;
//bmiva_face_algorithm_t algo = BMIVA_FD_MTCNN;
bmiva_dev_t g_dev;
bmiva_face_handle_t g_handle;
bmiva_face_handle_t g_Reghandle;
string g_feature_file;
BmivaFeature g_name_features;
//socket
//static int sockfd;
char sendData[UVC_PR_X*UVC_PR_Y*3];
//
pthread_t uvc_tid;
pthread_t sk_tid;
struct sockaddr_in servaddr;
//threshold
//float sim_threshold = 0.55;
float sim_threshold = 0.1;
int do_face_det = 0;
int add_variance = 0;
string person_name ="";
string record_path = "/demo_program/facedet_record";

static std::queue<cv::Mat>imagebuffer;
std::condition_variable available_;
std::mutex lock_;


int socket_connect()
{
	return 0;
	#if 0
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		LOG(LOG_DEBUG_ERROR,cout<<"create socket error: "<<(strerror(errno))<<"errno:"<<(errno)<<endl);
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8000);
	if(inet_pton(AF_INET, host_ip_addr.c_str(), &servaddr.sin_addr) <= 0)
	{
		LOG(LOG_DEBUG_ERROR,cout<<"inet_pton error"<<endl);
		exit(0);
	}

	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		LOG(LOG_DEBUG_ERROR,cout<<"connect error: "<<(strerror(errno))<<"errno: "<<(errno)<<endl);
		exit(0);
	}

	LOG(LOG_DEBUG_NORMAL,cout<<"ready to send buffer to server: "<<endl);
	#endif
}

void send_socket(cv::Mat frame)
{
	int sockfd;
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		LOG(LOG_DEBUG_ERROR,cout<<"create socket error: "<<(strerror(errno))<<"errno:"<<(errno)<<endl);
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = PF_INET;
	servaddr.sin_port = htons(9555);
	if(inet_pton(AF_INET, host_ip_addr.c_str(), &servaddr.sin_addr) <= 0)
	{
		LOG(LOG_DEBUG_ERROR,cout<<"inet_pton error"<<endl);
		exit(0);
	}

	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		LOG(LOG_DEBUG_ERROR,cout<<"connect error: "<<(strerror(errno))<<"errno: "<<(errno)<<endl);
		exit(0);
	}

	LOG(LOG_DEBUG_NORMAL,cout<<"ready to send buffer to server: "<<endl);

	std::vector<uchar> data_encode;
	cv::imencode(".jpg", frame, data_encode);

#if 0
	for(int i = 0 ;i <frame->rows;i++)
	{
		uchar *data = frame->ptr<uchar>(i);
		int num = i*frame->cols*3;
		for(int j = 0; j<frame->cols*3; j++)
		{
			sendData[num+j]=(char)data[j];
		}
	}
	LOG(LOG_DEBUG_USER_4,cout<<"senddata size: "<<(sizeof(sendData))<<endl);
#endif

	FILE *fp = fdopen(sockfd, "w");
    if (fp == nullptr) {
        fprintf(stderr, "failed to fdopen\n");
        ::close(sockfd);
    }
    printf("send data size:%d\n", data_encode.size());
    string mimetype = "image/jpeg";
    string url = "/display";
    fprintf(fp, "POST %s HTTP/1.1\n", url.data());
    fprintf(fp, "Content-Type: %s\n", mimetype.data());
    fprintf(fp, "Content-Length: %lu\n", data_encode.size());
    fprintf(fp, "\n");
    fwrite(data_encode.data(), sizeof(unsigned char), data_encode.size(), fp);
    fclose(fp);
#if 0
	int bytes=send(sockfd,sendData,sizeof(sendData),0);

	if(bytes<0)
	{
		LOG(LOG_DEBUG_ERROR,cout<<"send msg error:"<<(strerror(errno))<<"errno "<<(errno)<<endl);
		exit(0);
	}

	LOG(LOG_DEBUG_USER_4,cout<<"send bytes: "<<(bytes)<<endl);

	close(sockfd);
#endif
}

#if 0
void do_recognize_register(cv::Mat &frame,vector<vector<bmiva_face_info_t>>&results, string name)
{
	dbg_printf("string name:%s\n",name.c_str());
	std::fstream feature_fp(FACE_FEATURE_FILE,std::ios::app);
	bmiva_face_info_t* faceinfo = &results[0].at(0);
	cv::Mat aligned;
	if(face_align(frame,aligned,results[0].at(0),FACE_RECOG_IMG_W,FACE_RECOG_IMG_H)!=0)
	{
		return;
	}
	vector<cv::Mat>align_images;
	vector<vector<float>>features;
	align_images.push_back(aligned);
	bmiva_face_extractor_predict(g_Reghandle,align_images,features);
	std::ostringstream save_name_feature;
	save_name_feature << name.c_str();
	for(auto value:features[0])
	{
		save_name_feature << " " <<value;
	}
	save_name_feature << endl;
	feature_fp << save_name_feature.str();
	feature_fp.close();
}
#endif

void* socket_thread(void *arg)
{
	while(true)
	{

		std::unique_lock<std::mutex> locker(lock_);

		available_.wait(locker);
		
		if(!imagebuffer.empty())
		{
			auto image = std::move(imagebuffer.front());
			imagebuffer.pop();
			locker.unlock();
			LOG(LOG_DEBUG_USER_4,"start to send\n");
			send_socket(image);
			LOG(LOG_DEBUG_USER_4,"send done\n");
			//usleep(50000);
		}
	}
}

void* openuvc_thread(void *arg)
{
	int Ret = 0;
	struct timeval start_time,end_time;
	
	if(host_ip_addr.empty())
	{
		LOG(LOG_DEBUG_ERROR,cout<<" Please set host ip address first."<<endl);
		pthread_exit(NULL);
	}
	socket_connect();
	pthread_create(&sk_tid,NULL,socket_thread,NULL);
	//pthread_join(tid,NULL);
	//std::thread t(socket_thread);

	string model_dir = "";

	if (algo == BMIVA_FD_MTCNN)
	{
		string det1(model_dir), det2(model_dir), det3(model_dir);
		det1.append(DET1_MODEL_CFG_FILE);
		det2.append(DET2_MODEL_CFG_FILE);
		det3.append(DET3_MODEL_CFG_FILE);
		models.push_back(det1);
		models.push_back(det2);
		models.push_back(det3);
	}
	else if (algo == BMIVA_FD_TINYSSH)
	{
		string ssh(model_dir), onet(model_dir);
		ssh.append(SSH_MODEL_CFG_FILE);
		onet.append(ONET_MODEL_CFG_FILE);
		models.push_back(ssh);
		models.push_back(onet);
	}

	string ext(model_dir);
	ext.append(EXTRACTOR_MODEL_CFG_FILE);
	extractor_models.push_back(ext);

	g_name_features.clear();

	g_feature_file = model_dir;
	g_feature_file.append(FACE_FEATURE_FILE);

	ifstream fin(g_feature_file.c_str());
	if(fin){
		NameLoad(&g_name_features,g_feature_file);
	}else{
		LOG(LOG_DEBUG_ERROR,cout<<"feature txt not exist"<<endl);
		//return -1;
		pthread_exit(NULL);
	}

	bmiva_init(&g_dev);
	bmiva_face_handle_t *detector = &g_handle;
	bmiva_face_detector_create(detector,g_dev,models,algo);
	bmiva_face_extractor_create(&g_Reghandle,g_dev,extractor_models,BMIVA_FR_BMFACE);


	cv::VideoCapture capture;
	capture.open(200);
	if(capture.isOpened())
	{
		LOG(LOG_DEBUG_NORMAL,cout<<"open successful ."<<endl);
	}
	else
	{
		LOG(LOG_DEBUG_ERROR,cout<<"open fail !!"<<endl);
	}
	double width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	double fps = capture.get(CV_CAP_PROP_FPS);

	LOG(LOG_DEBUG_USER_4,printf("width:%.2f,height:%.2f fps:%.2f\n",width,height,fps));
	#if 0
	int ex1 = static_cast<int>(capture.get(CV_CAP_PROP_FOURCC));
	char ext1[] = {(char)(ex1 &255),(char)((ex1&0xff00)>>8),(char)((ex1&0xff0000)>>16),(char)((ex1&0xff000000)>>24),0};
	cout << "input codec type:"<< ext1 << endl;
	#endif
	//capture.set(CV_CAP_PROP_FOURCC,CV_FOURCC('D','I','V','X'));
	capture.set(CV_CAP_PROP_FOURCC,CV_FOURCC('N','V','1','2'));
	//capture.set(CV_CAP_PROP_FOURCC,CV_FOURCC('M','J','P','G'));
	//capture.set(CV_CAP_PROP_FRAME_WIDTH,1280);
	//capture.set(CV_CAP_PROP_FRAME_HEIGHT,720);
	capture.set(CV_CAP_PROP_FRAME_WIDTH,UVC_PR_X);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT,UVC_PR_Y);
	//capture.set(CV_CAP_PROP_FPS,30);
	width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	fps = capture.get(CV_CAP_PROP_FPS);

	LOG(LOG_DEBUG_USER_4,printf("after set,width:%.2f,height:%.2f fps:%.2f\n",width,height,fps));
	#if 0
	int ex2 = static_cast<int>(capture.get(CV_CAP_PROP_FOURCC));
	char ext2[] = {(char)(ex2 &255),(char)((ex2&0xff00)>>8),(char)((ex2&0xff0000)>>16),(char)((ex2&0xff000000)>>24),0};
	cout << "after set input codec type:"<<ext2<<endl;
	#endif

	
	int frameNum = 200;
	cv::VideoWriter outputVideo;
	cv::Size swh = cv::Size(width,height);
	outputVideo.open("/dump.avi",CV_FOURCC('M','J','P','G'),25.0,swh);

	while(1)
	{
		#ifdef PERF_TEST
		gettimeofday(&start_time,NULL);
		#endif
		Mat frame;
		capture >> frame;
		#ifdef DEBUG
		LOG(LOG_DEBUG_USER_4,cout<<"get frame"<<endl);
		#endif
		if(frame.empty())
		{
			LOG(LOG_DEBUG_ERROR,cout<<"frame empty"<<endl);
			break;
		}
		if(frameNum >= 0)
		{
			outputVideo << frame;
		}
		if(frameNum >= 0)
		{
			frameNum -- ;
		}

		LOG(LOG_DEBUG_USER_4,printf("frame width:%d height:%d\n",frame.cols,frame.rows));
		

		if(performace_test)
		{
			gettimeofday(&end_time,NULL);
			float cap_cost = (end_time.tv_sec - start_time.tv_sec)*1000000.0 + end_time.tv_usec-start_time.tv_usec;
			LOG(LOG_DEBUG_NORMAL,cout<<"cap costtime: "<<(cap_cost)<<" us ."<<endl);
		}

		// do face detect =============================
		//cv::imshow("demo",frame)
		//frame = cv::imread("/test.jpg");
		//dbg_printf("start capturing\n");
		//cv::imwrite("cap.jpg",frame);
		//dbg_printf("cap screen done\n");
		LOG(LOG_DEBUG_USER_4,cout<<"start to detect ."<<endl);
		vector<cv::Mat>images;
		vector<vector<bmiva_face_info_t>> results;
		images.push_back(frame);
		bmiva_face_detector_detect(g_handle,images,results);

		LOG(LOG_DEBUG_USER_4,cout<<"detect done ."<<endl);
		
		if(performace_test)
		{
			gettimeofday(&start_time,NULL);
			float det_cost = (start_time.tv_sec - end_time.tv_sec)*1000000.0 + start_time.tv_usec-end_time.tv_usec;
			LOG(LOG_DEBUG_NORMAL,cout<<"det costtime: "<<(det_cost)<<" us ."<<endl);
		}

		// print postion=========================
		#if 1//def DEBUG
		int sum = 0;
		float results_size =0, max_size =0;
		int max_size_id = 0;
		stringstream output;
		for (size_t i = 0; i < results.size(); i++ )
		{
			for (size_t j = 0; j < results[i].size(); j++ )
			{
				float X1 = results[i][j].bbox.x1;
				float Y1 = results[i][j].bbox.y1;
				float X2 = results[i][j].bbox.x2;
				float Y2 = results[i][j].bbox.y2;
				sum++;
				//cout<<"	i="<<i<<",j="<<j<<endl;
				output << "     face " << j + 1 << ": ("
				<< results[i][j].bbox.x1 << ", "
				<< results[i][j].bbox.y1 << "), ("
				<< results[i][j].bbox.x2 << ", "
				<< results[i][j].bbox.y2 << ")"
				<< endl;

				results_size = (X2-X1)*(Y2-Y1);
				if(j==0)
				{
					max_size = results_size;
					max_size_id = j;
				}
				else
				{
					if(results_size > max_size)
					{
						max_size = results_size;
						max_size_id = j;
					}
				}
			}
			//cout<<" max_size="<<max_size<<",max_size_id="<<max_size_id<<endl;
		}
		LOG(LOG_DEBUG_USER_3,output << "     Total " << sum << " faces detected." << endl);
		//cout << output.str() << endl;

		if((add_variance != 0) && (sum != 0))
		{
			Mat grayImage;
			cv::cvtColor(frame, grayImage, COLOR_BGR2GRAY);
			cv::Laplacian(grayImage, grayImage, CV_8U);

			Mat tmp_mean, tmp_stddev;
			cv::meanStdDev(grayImage, tmp_mean, tmp_stddev);
			double m = tmp_mean.at<double>(0,0);
			double variance = pow(m, 2);
			if (add_variance > variance)
			{
				LOG(LOG_DEBUG_ERROR,cout<<"bad image quality variance="<<variance<<endl);
			    //continue;
			    sum = 0;
			}

		}
		#if 0
		printf("position %d :%f,%f,%f,%f\n",i,x1,y1,x2,y2);
		if( (x2-x1)*(y2-y1) < 200)
		{
				printf("face too small .\n");
				continue;
		}
		#endif

		#endif

		//do recognize register==================
		#if 0
		if((results[0].size()>0)&&(fg_register)){
			do_recognize_register(frame,results,name);
		}
		#endif

		//do id certification===================
		//vector<bmiva_face_info_t>faceinfo =results[0];
		bmiva_face_info_t faceinfo_max;
		if(sum != 0)
		{
			faceinfo_max = results[0][max_size_id];
		}
		//BmivaFeature name_features;
		//NameLoad(&name_features,FACE_FEATURE_FILE);

		if(do_face_det != 0)
		{
			//for(size_t i =0;i<faceinfo.size();i++)
			if(sum != 0)
			{
				vector<vector<float>>features;
				vector<cv::Mat>align_images;

				cv::Mat aligned(FACE_RECOG_IMG_W,FACE_RECOG_IMG_H,frame.type());
				if(face_align(frame,aligned,faceinfo_max,FACE_RECOG_IMG_W,FACE_RECOG_IMG_H)!=0)
				{
					continue;
				}
				features.clear();
				align_images.clear();
				align_images.push_back(aligned);
				bmiva_face_extractor_predict(g_Reghandle,align_images,features);
				vector<float>feature = features[0];
				vector<std::pair<string,float>>result;
				CalcNameSim(&g_name_features,feature,&result);
				std::pair<std::string,float>name_sim = FindName(result,sim_threshold);
				//cout<<"name_sim.second"<<name_sim.second<<endl;
				memset(faceinfo_max.bbox.name,0,MAX_NAME_LEN);
				string tmp = name_sim.first;
				tmp.copy(faceinfo_max.bbox.name,tmp.length(),0);
	
				//bmiva_face_info_t face = results[0][i];
				float x1 = faceinfo_max.bbox.x1;
				float y1 = faceinfo_max.bbox.y1;
				float x2 = faceinfo_max.bbox.x2;
				float y2 = faceinfo_max.bbox.y2;

				cv::rectangle(frame, cv::Point(x1, y1),cv::Point(x2, y2), cv::Scalar(255, 255, 0), 3, 8, 0);

				if(faceinfo_max.bbox.name != (string)"unknown")
				{
					int pos_x = std::max(int(x1 - 5), 0);
					int pos_y = std::max(int(y1 - 10), 0);
					cv::putText(frame, faceinfo_max.bbox.name , cv::Point(pos_x, pos_y),
					cv::FONT_HERSHEY_PLAIN, 4.0, CV_RGB(255,255,0), 2.0);

					//cv::imwrite(record_path+"/"+person_name+"/"+faceinfo_max.bbox.name+"_"+to_string(do_face_det)+".jpg",frame.clone());
					cv::imwrite(record_path+"/"+person_name+"/"+faceinfo_max.bbox.name+"_"+to_string(do_face_det)+".jpg",frame.clone());
					//cout<<do_face_det<<", "<<record_path+"/"+person_name+"/"+faceinfo_max.bbox.name+"_"+to_string(do_face_det)+".jpg"<<endl;

					char record_info[200];
					sprintf(record_info,"echo %d: %s, name: %s, name_sim.second = %f >> %s/%s.txt",\
						do_face_det,(faceinfo_max.bbox.name == person_name)?"Pass":"Fail",((string)faceinfo_max.bbox.name).c_str(),\
						name_sim.second,(record_path+"/"+person_name).c_str(),person_name.c_str());
					LOG(LOG_DEBUG_NORMAL,cout<<"record_info: "<<record_info<<endl);
					system(record_info);
					do_face_det--;
				}
				
			}
			
		}
		else if(sum != 0)
		{
			float x1 = faceinfo_max.bbox.x1;
			float y1 = faceinfo_max.bbox.y1;
			float x2 = faceinfo_max.bbox.x2;
			float y2 = faceinfo_max.bbox.y2;

			cv::rectangle(frame, cv::Point(x1, y1),cv::Point(x2, y2), cv::Scalar(255, 255, 0), 3, 8, 0);
		}

		if(performace_test)
		{
			gettimeofday(&end_time,NULL);
			float recog_time = (end_time.tv_sec - start_time.tv_sec)*1000000.0 + end_time.tv_usec-start_time.tv_usec;
			LOG(LOG_DEBUG_NORMAL,"recognize cost time:%f us\n",recog_time);
		}

		//send data to socket ===================================
		cv::Mat img = frame.clone();
		std::lock_guard<std::mutex> locker(lock_);
			imagebuffer.push(img);

			if (imagebuffer.size() > 3)
            	imagebuffer.pop();
        

        available_.notify_one();	

		printf("queue size :%d\n",imagebuffer.size());
		
		//send_socket(&frame);
		if(performace_test)
		{
			gettimeofday(&start_time,NULL);
			float sock_time = (start_time.tv_sec - end_time.tv_sec)*1000000.0 + start_time.tv_usec-end_time.tv_usec;
			LOG(LOG_DEBUG_NORMAL,"socket cost time:%f us\n",sock_time);
		}
	}

	//close(sockfd);
	bmiva_face_extractor_destroy(g_Reghandle);
	bmiva_face_detector_destroy(g_handle);
	bmiva_deinit(g_dev);
	pthread_exit(NULL);
}

int cli_cmd_uvc_camera(int argc,char *argv[])
{
	if(argc == 1)
	{
		cout<<"Usage: uvc [open|stop]"<<endl;
		return 0;
	}
	else if(argc == 2)
	{
		if(!strcmp(argv[1],"open"))
		{
			pthread_create(&uvc_tid,NULL,openuvc_thread,NULL);
			cout<<"Open uvc camera ."<<endl;
			return 0;
		}
		else if(!strcmp(argv[1],"stop"))
		{
			cout<<"Stop uvc camera ."<<endl;
			return 0;
		}
	}
	
	cout<<"Input Error !!"<<endl;
	return -1;
}

int cli_cmd_do_facedet(int argc,char *argv[])
{
	char _cmd[200];
	int value;
	if(argc == 1)
	{
		cout<<"Usage: facedet [num] [name]"<<endl;
		return 0;
	}
	else if(argc == 3)
	{
		value = atoi(argv[1]);
		person_name = argv[2];

		sprintf(_cmd,"mkdir -p %s",(record_path+"/"+person_name).c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);
		sprintf(_cmd,"rm %s/*",(record_path+"/"+person_name).c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);
		sprintf(_cmd,"touch %s/%s.txt",(record_path+"/"+person_name).c_str(),person_name.c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);

		do_face_det = value;
	}

	cout<<" face detect is :"<<(do_face_det?"Enable":"Disable")<<" ."<<endl;
	return 0;
}

int  cli_cmd_threshold_value(int argc, char *argv[])
{
	//float f;
	if(argc == 1)
	{
		cout<<"Usage: threshold [get|set]"<<endl;
		return 0;
	}
	else if(argc == 2)
	{
		if(!strcmp(argv[1],"get"))
		{
			cout<<"sim_threshold = "<<sim_threshold<<" ."<<endl;
			return 0;
		}
	}
	else if(argc == 3)
	{
		if(!strcmp(argv[1],"set"))
		{
			sim_threshold = strtof(argv[2],NULL);
			cout<<"sim_threshold = "<<sim_threshold<<" ."<<endl;
			return 0;
		}
	}

	cout<<"Input Error !!"<<endl;
	return -1;
}


int cli_cmd_record_path(int argc, char* argv[])
{
	if(argc == 2)
	{
		record_path = argv[1];
	}
	cout<<"Test result will in: "<<record_path<<" ."<<endl;
	return 0;
}

int cli_cmd_variance(int argc, char *argv[])
{
	if(argc == 2)
	{
		add_variance = atoi(argv[1]);
	}
	cout<<"variance is "<<(add_variance?"Enable":"Disable")<<",  "<<add_variance<<" ."<<endl;
	return 0;
}


int cli_cmd_face_algorithm(int argc, char *argv[])
{
	if(argc == 1)
	{
		cout<<"Usage: algorithm [change]"<<endl;
	}
	else if(argc == 2)
	{
		if(!strcmp(argv[1],"change"))
		{
			if(algo == BMIVA_FD_TINYSSH)
			{
				algo = BMIVA_FD_MTCNN;
			}
			else
			{
				algo = BMIVA_FD_TINYSSH;
			}
			cout<<"You have changed bmiva face algorithm, please stop uvc and open again !"<<endl;
		}
	}
	cout<<"Current face algorithm is : "<<((algo==BMIVA_FD_TINYSSH)?"tiny_ssh":"mtcnn")<<" ."<<endl;
	return 0;
}


