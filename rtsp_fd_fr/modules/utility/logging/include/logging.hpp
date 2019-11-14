#pragma once

#ifdef GLOG_DISABLED
    #include <iostream>
    #define LOGD (std::cerr << std::endl << __FILE__ << ":" << __LINE__ << "] ")
    #define LOGI LOGD
    #define LOGW LOGD
    #define LOGE LOGD
    #define LOGF LOGD
#else
    #include "log_common.h"
#endif