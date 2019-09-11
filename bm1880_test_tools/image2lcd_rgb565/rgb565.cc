#include <iostream>
#include <opencv2/opencv.hpp>


using namespace std;
using namespace cv;


#ifdef HEX_16
unsigned short c_buf_hex_16[320*240];
#else
unsigned char c_buf_hex_8[320*240*2];
#endif

int main(int argc,char**argv)
{
		string pic_name,name ;
		cv::Mat imageSource = imread("logo_320x240.jpg",IMREAD_COLOR);
		cv::Mat imageResize;
		cv::Mat imageBGR,imageBGR565;

		if(imageSource.empty())
		{
			cout<<"File not exist ."<<endl;
			return -1;
		}
		// resize
		cout<<"frame_src width:"<<imageSource.cols<<"height: "<<imageSource.rows<<endl;

		if( imageSource.cols > 320 || imageSource.rows > 240)
		{
			int x = 240;
			int y = 320;
			cout<<" x= "<<x<<", y= "<<y<<endl;
			cv::resize(imageSource, imageResize,  Size(x, y) );
			//cv::imwrite(pic_resize_path + file_path + '/' + pic_name, imageResize);
			cout<<"frame width:%d"<<imageResize.cols<<" height: "<<imageResize.rows<<endl;
			//imshow("Resize", imageResize);
			imageBGR = imageResize;
		}
		else {
			imageBGR = imageSource;
		}

		cout<<"imageBGR.channels :  "<<imageBGR.channels()<<endl;
		cout<<"imageBGR.size :  "<<imageBGR.size()<<endl;
		cout<<"elem 1 : "<<hex<<imageBGR.at<int>(0,0)<<endl;
		cout<<"imageBGR.depth :  "<<imageBGR.depth()<<endl;

		cv:cvtColor(imageBGR,imageBGR565,COLOR_RGB2BGR565);
		//imshow("imageRGB565", imageRGB565);
		cout<<"imageBGR565.channels :  "<<imageBGR565.channels()<<endl;
		cout<<"imageBGR565.size :  "<<imageBGR565.size()<<endl;
		cout<<"imageBGR565.depth :  "<<imageBGR565.depth()<<endl;
		cout<<"elem 1 : "<<hex<<imageBGR565.at<int>(0,0)<<endl;
		cout<<"imageBGR565.type :  "<<imageBGR565.type()<<endl;

		int colNumber = imageBGR.cols;
		int rowNumber = imageBGR.rows;
		int i,j;

		#if 0 
		for (i = 0; i < rowNumber; i++)  
		{  
		    for (j= 0; j < colNumber; j++) 	
		    {  
		        unsigned short B = srcImage.at<Vec3b>(i, j)[0];   //B	     
		        unsigned short G = srcImage.at<Vec3b>(i, j)[1]; //G
		        unsigned short R = srcImage.at<Vec3b>(i, j)[2];   //R
		        //cout<<" R="<<R<<" G="<<G<<" B="<<B<<endl;
		        //cout<<"result = "<<(((R << 8) & 0xF800) | ((G << 3) & 0x07E0) | ((B >> 3) & 0x001F))<<endl;
		        //cout<<"1 = "<<((R << 8)&0xF800)<<endl;
		        //cout<<"2 = "<<(G << 3)<<endl;
		        //cout<<"3 = "<<(B >> 3)<<endl;
		        //cout<<"number : "<<(i*colNumber + j)<<endl;
		        //c_buf[i*colNumber + j] = ((R << 8) & 0xF800) | ((G << 6) & 0x07E0) | (B & 0x001F);
		        #ifdef HEX_16
		        c_buf_hex_16[i*colNumber + j] = ((R << 8) & 0xF800) | ((G << 3) & 0x07E0) | ((B >> 3) & 0x001F);
		        #else
		        unsigned short pix_data = ((R << 8) & 0xF800) | ((G << 3) & 0x07E0) | ((B >> 3) & 0x001F);
		        c_buf_hex_8[i*colNumber*2 + j*2 ] = (char)((pix_data >> 8) & 0xFF); 
		        c_buf_hex_8[i*colNumber*2 + j*2 + 1] = (char)(pix_data & 0xFF); 
		        #endif
		    }  
		}  

		//imshow("处理后的图像", srcImage); 
		#endif

		cv::cvtColor(imageBGR,imageBGR565,cv::COLOR_BGR2BGR565);
		memcpy(c_buf_hex_8,imageBGR565.data,colNumber*rowNumber*2);

#ifdef HEX_16
		for(i=0; i<rowNumber; i++)
		{
			for(j=0; j<colNumber; j++)
			{
				printf(" 0x%04X, ",c_buf_hex_16[i*colNumber + j]);
			}	
			printf("\n");
		}
#else
		for(i=0; i<rowNumber; i++)
		{
			for(j=0; j<colNumber; j++)
			{
				printf(" 0x%02X, 0x%02X,",c_buf_hex_8[i*colNumber*2 + j*2],c_buf_hex_8[i*colNumber*2 + (j*2+1)]);
			}	
			printf("\n");
		}
#endif
		waitKey();
		return 0;
}
