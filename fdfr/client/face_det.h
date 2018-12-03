#ifndef __FACE_DET_H__
#define __FACE_DET_H__


#define UVC_PR_X  (1280)
#define UVC_PR_Y  (720)

#define FACE_RECOG_IMG_W 112
#define FACE_RECOG_IMG_H 112
#define MAX_NAME_LEN 256

#define DET1_MODEL_CFG_FILE "/system/data/bmodel/mtcnn/det1.bmodel"
#define DET2_MODEL_CFG_FILE "/system/data/bmodel/mtcnn/det2.bmodel"
#define DET3_MODEL_CFG_FILE "/system/data/bmodel/mtcnn/det3.bmodel"
#define EXTRACTOR_MODEL_CFG_FILE "/system/data/bmodel/bmface.bmodel"
//#define EXTRACTOR_MODEL_CFG_FILE  "/usr/data/bmodel/bmface_1_3_112_112_shift6.bmodel"
#define SSH_MODEL_CFG_FILE  "/system/data/bmodel/tiny_ssh.bmodel"
#define ONET_MODEL_CFG_FILE "/system/data/bmodel/det3.bmodel"
#define FACE_FEATURE_FILE "/system/data/bmodel/features.txt"



int cli_cmd_uvc_camera(int argc,char *argv[]);
int cli_cmd_do_facedet(int argc,char *argv[]);
int cli_cmd_do_facedet(int argc,char *argv[]);
int cli_cmd_threshold_value(int argc, char *argv[]);
int cli_cmd_record_path(int argc, char *argv[]);
int cli_cmd_variance(int argc, char *argv[]);
int cli_cmd_face_algorithm(int argc, char *argv[]);
#endif
