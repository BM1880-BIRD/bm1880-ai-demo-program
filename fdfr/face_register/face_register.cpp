#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <queue>
#include <mutex>
#include <thread>

#include "bmiva.hpp"
#include "bmiva_util.hpp"
#include "bmiva_face.hpp"

using namespace std;
using namespace cv;

#define DET1_MODEL_CFG_FILE "/system/data/bmodel/mtcnn/det1.bmodel"
#define DET2_MODEL_CFG_FILE "/system/data/bmodel/mtcnn/det2.bmodel"
#define DET3_MODEL_CFG_FILE "/system/data/bmodel/mtcnn/det3.bmodel"
#define EXTRACTOR_MODEL_CFG_FILE "/system/data/bmodel/bmface.bmodel"
#define SSH_MODEL_CFG_FILE   "/system/data/bmodel/tiny_ssh.bmodel"
#define ONET_MODEL_CFG_FILE  "/system/data/bmodel/det3.bmodel"
#define FACE_FEATURE_FILE 	 "/demo_program/face_register/features.txt"
#define FACE_RECOG_IMG_W 112
#define FACE_RECOG_IMG_H 112
#define MAX_NAME_LEN 256

//#define DEBUG
//#define PERF_TEST

#ifdef DEBUG
#define dbg_printf(fmt...)  printf(fmt)
#else
#define dbg_printf(fmt...)  ((void)0)
#endif


string registered_pic_path = "/mnt/usb/registered_pic";
char _cmd[200];

vector<string> models;
vector<string> extractor_models;
bmiva_face_algorithm_t algo = BMIVA_FD_TINYSSH;
//bmiva_face_algorithm_t algo = BMIVA_FD_MTCNN;
bmiva_dev_t g_dev;
bmiva_face_handle_t g_handle;
bmiva_face_handle_t g_Reghandle;
string g_feature_file;
BmivaFeature g_name_features;

static std::queue<cv::Mat>imagebuffer;
static std::mutex mtx;

void do_recognize_register(cv::Mat &frame,bmiva_face_info_t faceinfo, string name)
{
	#if 0
	Mat grayImage;
	cv::cvtColor(frame, grayImage, COLOR_BGR2GRAY);
	cv::Laplacian(grayImage, grayImage, CV_8U);

	Mat tmp_mean, tmp_stddev;
	cv::meanStdDev(grayImage, tmp_mean, tmp_stddev);
	double m = tmp_mean.at<double>(0,0);
	double variance = pow(m, 2);
	if (50 > variance) 
	{
		dbg_printf("%s's image quality is bad. variance=%f\n", name.c_str(), variance);
	    return;
	}
	#endif
	dbg_printf("string name:%s\n",name.c_str());
	std::fstream feature_fp(FACE_FEATURE_FILE,std::ios::app);
	//bmiva_face_info_t* faceinfo = &results[0].at(0);
	cv::Mat aligned;
	if(face_align(frame,aligned,faceinfo,FACE_RECOG_IMG_W,FACE_RECOG_IMG_H)!=0)
	{
		cout<<"face align fail"<<endl;
		return;
	}
	vector<cv::Mat>align_images;
	vector<vector<float>>features;
	align_images.push_back(aligned);
	bmiva_face_extractor_predict(g_Reghandle,align_images,features);
	std::ostringstream save_name_feature;
	save_name_feature << name.c_str();
	for(auto value:features[0]){
		save_name_feature << " " <<value;
	}
	save_name_feature << endl;
	feature_fp << save_name_feature.str();
	feature_fp.close();
	//fg_register = 0;
	cout<<"do_recognize_register OK"<<endl;
}

std::string  get_name(char *p_name)
{
		char __name[21];
		string name;
		char a;
		int i;
		for(i=0;i<20;i++)
		{
			a = *(p_name + i);
			//printf("%d, a = %c \n",i,a);
			if( (a == ' ') || (a == '-') || (a == '.') || (a == '_'))
			{
					break;
			}
			else
			{
				__name[i]=a;
			}
		}
		__name[i]=0;
		//printf("__name : %s \n",__name);
		name.assign(__name);
		//cout<<"name: "<<name.c_str()<<endl;
		return name;
}

void showAllFiles( string dir_name )
{
		#ifdef PERF_TEST
		struct timeval start_time,end_time;
		#endif
		string name;
		// check the parameter !
		if( dir_name.empty() )
		{
			cout<<" dir_name is null ! "<<endl;
			return;
		}

		// check if dir_name is a valid dir
		struct stat s;
		lstat( dir_name.c_str() , &s );
		if( ! S_ISDIR( s.st_mode ) )
		{
			cout<<"dir_name is not a valid directory !"<<endl;
			return;
		}
		else
		{
			sprintf(_cmd,"mkdir -p %s",(registered_pic_path+dir_name).c_str());
			cout<<"system cmd :"<<_cmd<<endl;
			system(_cmd);
		}

		struct dirent * filename;    // return value for readdir()
	 	DIR * dir;                   // return value for opendir()
		dir = opendir( dir_name.c_str() );
		if( NULL == dir )
		{
			cout<<"Can not open dir "<<dir_name<<endl;
			return;
		}
		//cout<<"Successfully opened the dir !"<<endl;

		/* read all the files in the dir ~ */
		while( ( filename = readdir(dir) ) != NULL )
		{
			// get rid of "." and ".."
			if( strcmp( filename->d_name , "." ) == 0 ||
				strcmp( filename->d_name , "..") == 0    )
				continue;
			struct stat s_buf;
			string dir_or_file;
			dir_or_file = dir_name +'/'+(filename->d_name);
			stat(dir_or_file.c_str() ,&s_buf);
			if(S_ISDIR(s_buf.st_mode))
			{
					//cout<<dir_or_file.c_str()<<endl;
					//cout<<" is a dir "<<endl;
					showAllFiles(dir_or_file);

			}
			else
			{
				cout<<"<=============================+++++++++++++++===========>"<<endl;
				cout<<dir_or_file.c_str()<<"  is a file "<<endl;

				//name.assign(filename->d_name);
				name = get_name(filename->d_name);
				cout<<"name:"<<name<<endl;
				#ifdef PERF_TEST
				gettimeofday(&start_time,NULL);
				#endif
				static Mat  frame;
		    	frame = cv::imread(dir_or_file.c_str());

				#ifdef DEBUG
				dbg_printf("frame width:%d height:%d\n",frame.cols,frame.rows);
				#endif

				#ifdef DEBUG
				dbg_printf("get frame\n");
				#endif
				if(frame.empty())
				{
						dbg_printf("frame empty, %s  !!\n",dir_or_file.c_str());
						return ;
				}

				#ifdef PERF_TEST
		    	gettimeofday(&end_time,NULL);
		    	float cap_cost = (end_time.tv_sec - start_time.tv_sec)*1000000.0 + end_time.tv_usec-start_time.tv_usec;
		    	dbg_printf("cap costtime:%f us\n",cap_cost);
		    	#endif

				dbg_printf("start to detect\n");
				vector<cv::Mat>images;
				vector<vector<bmiva_face_info_t>> results;
				images.push_back(frame);
				bmiva_face_detector_detect(g_handle,images,results);
				#ifdef DEBUG
				dbg_printf("detect done\n");
				#endif

				#ifdef PERF_TEST
				gettimeofday(&start_time,NULL);
				float det_cost = (start_time.tv_sec - end_time.tv_sec)*1000000.0 + start_time.tv_usec-end_time.tv_usec;
		                	dbg_printf("det costtime:%f us\n",det_cost);
				#endif

				// print postion=========================
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
					cout<<" max_size="<<max_size<<",max_size_id="<<max_size_id<<endl;
		  		}
		  		output << "     Total " << sum << " faces detected." << endl;
		 		cout << output.str()<< endl;
		 		//dbg_printf(" 		Total %d faces detected .\n",sum);


		 		#if 0
		 		int size = results[0].size();
		 		vector<bmiva_face_info_t>faceinfo =results[0];
                for (int i=0;i<size;i++)
                {
                        //bmiva_face_info_t face = results[0][i];
                        float x1 = faceinfo[i].bbox.x1;
                        float y1 = faceinfo[i].bbox.y1;
                        float x2 = faceinfo[i].bbox.x2;
                        float y2 = faceinfo[i].bbox.y2;


                        cv::rectangle(frame, cv::Point(x1, y1),cv::Point(x2, y2), cv::Scalar(255, 255, 0), 3, 8, 0);
                        int pos_x = std::max(int(x1 - 5), 0);
                        int pos_y = std::max(int(y1 - 10), 0);
                        cv::putText(frame, to_string(i)/*faceinfo[i].bbox.name*/ , cv::Point(pos_x, pos_y),
                        cv::FONT_HERSHEY_PLAIN, 4.0, CV_RGB(255,255,0), 2.0);
                        cv::imwrite(record_path+dir_or_file,frame);
                }
				#endif

				//do recognize register==================
				if(sum != 0)
				{
					bmiva_face_info_t faceinfo_max;
					faceinfo_max = results[0][max_size_id];
					//cout<<"Face info score : "<<faceinfo_max.bbox.score<<endl;
					do_recognize_register(frame,faceinfo_max,name);
					dbg_printf("detect ok , %s \n",dir_or_file.c_str());

					cv::imwrite(registered_pic_path + dir_or_file, frame);
				}
				else
				{
					dbg_printf("detect fail , %s \n",dir_or_file.c_str());
				}

			}

		}
}

int main(int argc,char**argv)
{
	string model_dir = "";
	string dir;



	if((argc == 3)&&(!strcmp(argv[1],"FR"))){
		dbg_printf("Recognize register\n");
		dir = argv[2];
		dbg_printf("input dir name:%s\n",dir.c_str());
	}

	if (algo == BMIVA_FD_MTCNN)
	{
	      	string det1(model_dir), det2(model_dir), det3(model_dir);
	      	det1.append(DET1_MODEL_CFG_FILE);
	      	det2.append(DET2_MODEL_CFG_FILE);
	      	det3.append(DET3_MODEL_CFG_FILE);
	      	models.push_back(det1);
	      	models.push_back(det2);
	      	models.push_back(det3);
	      	cout<<"use mtcnn"<<endl;
	}
	else if (algo == BMIVA_FD_TINYSSH)
	{
	      	string ssh(model_dir), onet(model_dir);
	      	ssh.append(SSH_MODEL_CFG_FILE);
	      	onet.append(ONET_MODEL_CFG_FILE);
	      	models.push_back(ssh);
	      	models.push_back(onet);
	      	cout<<"use tiny ssh"<<endl;
	}

	string ext(model_dir);
	ext.append(EXTRACTOR_MODEL_CFG_FILE);
	extractor_models.push_back(ext);

	g_name_features.clear();

	g_feature_file = model_dir;
	g_feature_file.append(FACE_FEATURE_FILE);

	ifstream fin(g_feature_file.c_str());
	if(fin)
	{
		NameLoad(&g_name_features,g_feature_file);
	}
	else
	{
		dbg_printf("feature txt not exist\n");
		return -1;
	}

	bmiva_init(&g_dev);
	bmiva_face_handle_t *detector = &g_handle;
	bmiva_face_detector_create(detector,g_dev,models,algo);
	bmiva_face_extractor_create(&g_Reghandle,g_dev,extractor_models,BMIVA_FR_BMFACE);


	showAllFiles( dir );






	bmiva_face_extractor_destroy(g_Reghandle);
	bmiva_face_detector_destroy(g_handle);
	bmiva_deinit(g_dev);
	return 0;
}
