
#ifndef BM_STORAGE_HPP
#define BM_STORAGE_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include "sqlite3.h"
#include "ipc_configs.h"
#include "bmiva_util.hpp"

#define USER_NAME_LEN 20
#define USER_INFO_LEN 20

typedef enum
{
    RET_OK,
    RET_FULL,
    RET_ID_NOT_EXIST,
    RET_ERROR,
} StorageRetCode;

/*
typedef struct
{
    unsigned int user_id;
    char user_name[USER_NAME_LEN];
    char user_info[USER_INFO_LEN];
} __attribute__((packed)) BitMainUsr;
*/

class BmStorage {
 public:
  static BmStorage &get_instance()
  {
    static BmStorage instance;
    return instance;
  }

  /*
  StorageRetCode add_user(unsigned int user_id, char *name, char *addi_info, cv::Mat &image, std::vector<float> feature);
  StorageRetCode delete_user(unsigned int user_id);
  StorageRetCode search_user(unsigned int user_id, unsigned int max_event_num, long long start, long long end, long long *events);
  StorageRetCode add_event(unsigned int user_id, long long event);
  
  unsigned int get_user_count() { return count; }
  StorageRetCode list_users(unsigned int user_id, unsigned int *users);
  StorageRetCode get_image(unsigned int user_id, cv::Mat &image);
  StorageRetCode get_info(unsigned int user_id, BitMainUsr &info);
  */

  StorageRetCode add_user(unsigned int id, char *name, char *addi_info, cv::Mat &image, std::vector<float> &feature);
  StorageRetCode update_user(unsigned int id, char *name, char *addi_info);
  StorageRetCode delete_user(unsigned int id);
  StorageRetCode list_users(unsigned int &count, unsigned int *id);
  
  StorageRetCode match_feature(std::vector<float> &feature, unsigned int &id, float &similarity);
  
  StorageRetCode add_event(unsigned int id, long long event);
  StorageRetCode search_event(unsigned int id, unsigned int max_event_num, long long start, long long end, unsigned int &event_count, long long *events);
  
  StorageRetCode get_image(unsigned int id, cv::Mat &image);
  StorageRetCode get_info(unsigned int id, BitMainUsr &info);

  StorageRetCode get_latest_event_time(unsigned int id, long long *latest_time);

 private:
  BmStorage();
  ~BmStorage();

  struct sqlite3 *database_handle_;
};

#endif // BM_STORAGE_HPP

