/*
 ********************************************************************
 * Demo program on Bitmain BM1880
 *
 * Copyright © 2018 Bitmain Technologies. All Rights Reserved.
 *
 * Author:  liang.wang02@bitmain.com
 *			xiaojun.zhu@bitmain.com
 ********************************************************************
*/

#ifndef __FACE_DET_H__
#define __FACE_DET_H__

#define UVC_PR_X  (640)
#define UVC_PR_Y  (480)

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

extern string host_ip_addr;


int CliCmdUvcCamera(int argc,char *argv[]);
int CliCmdDoFaceDet(int argc,char *argv[]);
int CliCmdThresholdValue(int argc,char *argv[]);
int CliCmdThresholdValue(int argc, char *argv[]);
int CliCmdRecordPath(int argc, char *argv[]);
int CliCmdVariance(int argc, char *argv[]);
int CliCmdFaceAlgorithm(int argc, char *argv[]);
#endif
