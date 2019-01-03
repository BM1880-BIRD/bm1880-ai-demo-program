#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#define  MAX_SIZE_C  1920
#define  MAX_SIZE_R  1440


bool StrCmpIgnoreCases(const string &a, const string &b)
{
	int i,j;
	if(a.length() != b.length())
	{
		return false;
	}
	else
	{
		for(i=0; i<a.length(); i++)
		{
			if((a[i] == b[i]) || (abs(a[i] - b[i]) == 32))
			{
				;
			}
			else
				return false;
		}
	}
	return true;
}

int main(int argc,char**argv)
{
		string file_name,record_path ;
		cv::Mat imageSource,imageResize;
		char _cmd[300];
		
		if(argc != 3)
		{
			cout<<"Usage: img_resize [file name] [record path]"<<endl;
			return -EIO;
		}
		else
		{
			file_name = argv[1];
			record_path = argv[2];
		}
		cout<<"---------------------------------------------------"<<endl;
		if(access(file_name.c_str(),0) == -1)
		{
			cout<<"File: \""<<file_name<<"\""<<" does not exist !!"<<endl;
			return -ENOENT;
		}
		if(access(record_path.c_str(),0) == -1)
		{
			cout<<"Path: \""<<record_path<<"\""<<" does not exist !!"<<endl;
			return -ENOENT;
		}
		
		string suffixStr = file_name.substr(file_name.find_last_of('.') + 1);

		if(StrCmpIgnoreCases(suffixStr,"jpg") \
			|| StrCmpIgnoreCases(suffixStr,"jpeg") \
			|| StrCmpIgnoreCases(suffixStr,"png") )
		{
			cout<<"image: "<<file_name<<endl;
		}
		else
		{
			return -EPERM;
		}

		imageSource = imread(file_name);
		//imshow("Source", imageSource);

		if(imageSource.empty())
		{
			cout<<"File: "<<file_name<<"not exist.";
			return -ENOENT;
		}
		// resize
		cout<<"frame_src width: "<<imageSource.cols<<" height: "<<imageSource.rows<<" ."<<endl;

		if( imageSource.cols > MAX_SIZE_C || imageSource.rows > MAX_SIZE_R)
		{
			int scale_c = (imageSource.cols/MAX_SIZE_C) + 1 ;
			int scale_r = (imageSource.rows/MAX_SIZE_R) + 1 ;

			int scale = (scale_c > scale_r)? scale_c:scale_r;
			
			int x = round(imageSource.cols/scale) ;
			int y = round(imageSource.rows/scale);
			cout<<"scale: "<<scale<<", x= "<<x<<", y= "<<y<<" ."<<endl;
			cv::resize(imageSource, imageResize,  Size(x, y) );
			
			cout<<record_path + '/' + file_name<<endl;
			sprintf(_cmd,"dirname \"%s\" | xargs mkdir -p ",(record_path + '/' + file_name).c_str());
			system(_cmd);
			cv::imwrite(record_path + '/' + file_name, imageResize);
			
			//imshow("Resize", imageResize);
		}

		#if 0
		Mat imageWB;
		if( imageSource.cols > 1280)
		{
			imageWB = imageResize;
		}
		imageWB = imageSource;
		// white balance
		vector<Mat> imageRGB;
		//RGB三通道分离
		split(imageWB, imageRGB);

		//求原始图像的RGB分量的均值
		double R, G, B;
		B = mean(imageRGB[0])[0];
		G = mean(imageRGB[1])[0];
		R = mean(imageRGB[2])[0];

		//需要调整的RGB分量的增益
		double KR, KG, KB;
		KB = (R + G + B) / (3 * B);
		KG = (R + G + B) / (3 * G);
		KR = (R + G + B) / (3 * R);

		//调整RGB三个通道各自的值
		imageRGB[0] = imageRGB[0] * KB;
		imageRGB[1] = imageRGB[1] * KG;
		imageRGB[2] = imageRGB[2] * KR;

		//RGB三通道图像合并
		merge(imageRGB, imageWB);
		//imshow("白平衡调整后", imageWB);
		#endif
		waitKey();
		return 0;
}
