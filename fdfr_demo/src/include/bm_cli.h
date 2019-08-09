/*
 ********************************************************************
 * Demo program on Bitmain BM1880
 *
 * Copyright © 2018 Bitmain Technologies. All Rights Reserved.
 *
 * Author:  liang.wang02@bitmain.com
 *
 ********************************************************************
*/
#ifndef __BM_CLI_H__
#define __BM_CLI_H__

extern void BmCliListCliBase(void);
extern void BmCliListCliHelp(void);
extern void BmCliInit(void);
extern void BmCliMain(int argc, char *argv[]);
//=================================================
extern int BmCliCmdGetFrame(int argc,char *argv[]);
extern int CliCmdfdfr(int argc,char *argv[]);
extern int CliCmdTestFr(int argc,char *argv[]);
extern int CliCmdDoFaceRegister(int argc,char *argv[]);
extern int CliCmdRepoManage(int argc,char *argv[]);
extern int BmCliCmdPrintCfg(int argc, char *argv[]);
extern int BmCliCmdSetCfg(int argc, char *argv[]);

#endif
