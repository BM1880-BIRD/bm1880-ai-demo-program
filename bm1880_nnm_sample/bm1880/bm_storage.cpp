#include "bm_storage.hpp"

#include <vector>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include "sqlite3.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>


using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::to_string;
using std::istringstream;
using std::ostringstream;

static const string kDatabaseName = "xmface.db";
static const string kDatabasePathPrefix = "/home/";
static const string kImagePathPrefix = "/home/";


typedef struct bm_parameter_st {
  int num;
  void *para[10];
}BM_PARAMETER_ST;


static int bm_mkdir_parents(const string &path, mode_t mode, unsigned int max_depth = 10) {
  size_t position = 0;

  for (int i = 0; i < max_depth; i++) {
  	position = path.find('/', position);
	if (position == string::npos)
		break;
	position += 1;

	string substring = path.substr(0, position);

	int ret =  access(substring.c_str(), F_OK);
    if (ret != 0) {
      ret = mkdir(substring.c_str(), S_IRWXU);
	  if (ret != 0)
	  	return -1;
    }
  }

  return 0;  
}


static int bm_find_event(void *para, int argc, char **value, char **name) {
  int i;
  istringstream stream;
  long long event;
  vector<long long> *event_vector = (vector<long long> *)para;

  for(int i = 0; i < argc; i++) {
    if (strcasecmp("TIME", name[i]) == 0) {
	  stream.clear();
      stream.str(string(value[i]));
	  if (stream >> event)
	    event_vector->push_back(event);
	}	
  }
  
  return 0;
}


static int bm_list_id(void *para, int argc, char **value, char **name) {
  int i;
  istringstream stream;
  unsigned int id;
  vector<unsigned int> *id_vector = (vector<unsigned int> *)para;

  for(int i = 0; i < argc; i++) {
    if (strcasecmp("ID", name[i]) == 0) {
	  stream.clear();
      stream.str(string(value[i]));
	  if (stream >> id)
	    id_vector->push_back(id);
	}	
  }
  
  return 0;
}


static int bm_get_info(void *para, int argc, char **value, char **name) {
  int i;
  istringstream stream;
  unsigned int id;
  BitMainUsr *info = (BitMainUsr *)para;

  for(int i = 0; i < argc; i++) {
    if (strcasecmp("NAME", name[i]) == 0) {
      memset(info->user_name, 0, sizeof(info->user_name));
	  strncpy(info->user_name, value[i], sizeof(info->user_name) - 1);
    } else if (strcasecmp("EXTRA_INFO", name[i]) == 0) {
      memset(info->user_info, 0, sizeof(info->user_info));
	  strncpy(info->user_info, value[i], sizeof(info->user_info) - 1);
    }
  }
  
  return 0;
}




float bm_cosine_similarity(const vector<float> &a, const vector<float> &b) {
  if (a.size() == 0 && b.size() == 0)
  	return 1;
  else if ((a.size() == 0 && b.size() != 0) || (a.size() != 0 && b.size() == 0))
  	return 0;

  unsigned int size(a.size());
  if (size > b.size())
  	size = b.size();

  float sum_a(0), sum_b(0), cosine(0);
  for (unsigned int i = 0; i < size; i++) {
    sum_a += a[i] * a[i];
	sum_b += b[i] * b[i];
    cosine += a[i] * b[i];
  }

  sum_a = sqrt(sum_a);
  sum_b = sqrt(sum_b);

  if (sum_a < 0.0000001) {
  	if (sum_b < 0.0000001)
		return 1;
	else
		return 0;
  }

  return cosine / (sum_a * sum_b);
}



static int bm_match_feature(void *para, int argc, char **value, char **name) {
  int i;
  istringstream stream;
  unsigned int id(0);
  float feature;
  float similarity(0);
  vector<float> feature_vector;
  BM_PARAMETER_ST *parameter = (BM_PARAMETER_ST *)para;
  
  if (parameter->num != 3)
  	return -1;

  for(int i = 0; i < argc; i++) {
  	if (strcasecmp("ID", name[i]) == 0) {
	  stream.clear();
      stream.str(string(value[i]));
	  stream >> id;
	} else if (strcasecmp("FEATURE", name[i]) == 0) {
	  stream.clear();
      stream.str(string(value[i]));
	  while (stream >> feature) {
	    feature_vector.push_back(feature);
	  }
	  similarity = bm_cosine_similarity(feature_vector, *((vector<float> *)((parameter->para)[0])));
	}

	if (*((float *)((parameter->para)[2])) < similarity) {
      *((unsigned int *)((parameter->para)[1])) = id;
      *((float *)((parameter->para)[2])) = similarity;
	}
  }
  
  return 0;
}


BmStorage::BmStorage() {
  // close the old database
  if (database_handle_)
  	sqlite3_close(database_handle_);

  int ret =  access(kDatabasePathPrefix.c_str(), F_OK);
  if (ret != 0) {
    bm_mkdir_parents(kDatabasePathPrefix, S_IRWXU);
  }

  ret =  access(kImagePathPrefix.c_str(), F_OK);
  if (ret != 0) {
    bm_mkdir_parents(kImagePathPrefix, S_IRWXU);
  }

  sqlite3_open((kDatabasePathPrefix + kDatabaseName).c_str(), &database_handle_);

  char sql_people[] = "CREATE TABLE IF NOT EXISTS PEOPLE("  \
                      "ID  INT PRIMARY KEY    NOT NULL," \
                      "NAME            TEXT   NOT NULL," \
                      "EXTRA_INFO      TEXT   NOT NULL," \
                      "IMAGE_PATH      TEXT   NOT NULL," \
                      "FEATURE         TEXT   NOT NULL);";

  char *error_msg = NULL;
  sqlite3_exec(database_handle_, sql_people, NULL, NULL, &error_msg);
  if (ret != SQLITE_OK ) {
    fprintf(stderr, "\nfail to create people table, error: %s\n", error_msg);
  }

  char sql_event[] = "CREATE TABLE IF NOT EXISTS EVENT("  \
                     "TIME  INT  NOT NULL," \
                     "ID    INT  NOT NULL);";

  ret = sqlite3_exec(database_handle_, sql_event, NULL, NULL, &error_msg);
  if (ret != SQLITE_OK ) {
    fprintf(stderr, "\nfail to create event table, error: %s\n", error_msg);
  }
}


BmStorage::~BmStorage() {
  if (database_handle_)
  	sqlite3_close(database_handle_);
  database_handle_ = NULL;
}


StorageRetCode BmStorage::add_user(unsigned int user_id, char *name, char *addi_info, cv::Mat &image, vector<float> &feature) {

  if (name == NULL || addi_info == NULL)
  	return RET_ERROR;

  if (user_id == 0)
  	return RET_ERROR;

  ostringstream stream;
  stream << "INSERT INTO PEOPLE (ID, NAME, EXTRA_INFO, IMAGE_PATH, FEATURE) VALUES (";
  stream << user_id << ", ";
  stream << "\'" << name << "\'" << ", ";
  stream << "\'" << addi_info << "\'" << ", ";
  stream << "\'" << user_id << ".jpg" << "\'" << ", ";
  stream << "\'";
  for (auto &value : feature) {
    stream << value << " ";
  }
  stream << "\'";
  stream << ");";

  char *error_msg = NULL;
  int ret = sqlite3_exec(database_handle_, stream.str().c_str(), NULL, NULL, &error_msg);
  if (ret != SQLITE_OK ) {
     fprintf(stderr, "\nadd_user error: %s\n", error_msg);
    return RET_ERROR;
  }

  cv::imwrite(kImagePathPrefix + to_string(user_id) + ".jpg", image);

  return RET_OK;
}


StorageRetCode BmStorage::update_user(unsigned int id, char *name, char *addi_info) {
  if (name == NULL || addi_info == NULL)
  	return RET_ERROR;
  
  ostringstream stream;
  stream << "UPDATE PEOPLE SET NAME = \'" << name << "\', EXTRA_INFO = \'" << addi_info
         << "\' WHERE ID = " << id << ";";
  
  char *error_msg = NULL;
  int ret = sqlite3_exec(database_handle_, stream.str().c_str(), NULL, NULL, &error_msg);
  if (ret != SQLITE_OK ) {
    // fprintf(stderr, "\ndelete_user error: %s\n", error_msg);
    return RET_ERROR;
  } else {
    return RET_OK;
  }
}


StorageRetCode BmStorage::delete_user(unsigned int id) {
  ostringstream stream_p;
  ostringstream stream_e;
  stream_p << "DELETE FROM PEOPLE WHERE ID = " << id << ";";
  stream_e << "DELETE FROM EVENT WHERE ID = " << id << ";";

  char *error_msg = NULL;
  int ret = sqlite3_exec(database_handle_, stream_p.str().c_str(), NULL, NULL, &error_msg);
  if (ret != SQLITE_OK ) {
    // fprintf(stderr, "\ndelete_user error: %s\n", error_msg);
    return RET_ERROR;
  }

  ret = sqlite3_exec(database_handle_, stream_e.str().c_str(), NULL, NULL, &error_msg);
  if (ret != SQLITE_OK ) {
    // fprintf(stderr, "\ndelete_user error: %s\n", error_msg);
    return RET_ERROR;
  }
  
  ostringstream ss;
  ss << id;
  string filename = kImagePathPrefix;
  filename.append(ss.str());
  filename.append(".jpg");
  if(remove(filename.c_str())!=0)
    printf("Error deleting face jpg\n");

  return RET_OK;
}


StorageRetCode BmStorage::list_users(unsigned int &count, unsigned int *id) {
  ostringstream stream;
  stream << "SELECT ID FROM PEOPLE;";
  
  char *error_msg = NULL;
  vector<unsigned int> id_vector;
  int ret = sqlite3_exec(database_handle_, stream.str().c_str(), bm_list_id, &id_vector, &error_msg);
  if (ret != SQLITE_OK ) {
    fprintf(stderr, "\nlist_users error: %s\n", error_msg);
    return RET_ERROR;
  }

  count = id_vector.size();
  
  for (int i = 0; i < count; i++) {
    *(id + i) = id_vector[i];
  }

  return RET_OK;  
}


StorageRetCode BmStorage::match_feature(vector<float> &feature, unsigned int &id, float &similarity) {
  ostringstream stream;
  stream << "SELECT ID, FEATURE FROM PEOPLE;";

  id = 0;
  similarity = 0;
 
  BM_PARAMETER_ST parameter;
  parameter.num = 3;
  parameter.para[0] = &feature;
  parameter.para[1] = &id;
  parameter.para[2] = &similarity;
  
  char *error_msg = NULL;

  int ret = sqlite3_exec(database_handle_, stream.str().c_str(), bm_match_feature, &parameter, &error_msg);
  if (ret != SQLITE_OK ) {
    fprintf(stderr, "\nlist_users error: %s\n", error_msg);
    return RET_ERROR;
  }
}



StorageRetCode BmStorage::add_event(unsigned int id, long long event) {
  ostringstream stream;
  stream << "INSERT INTO EVENT (TIME, ID) VALUES (";
  stream << event << ", " << id << ");";

  char *error_msg = NULL;
  int ret = sqlite3_exec(database_handle_, stream.str().c_str(), NULL, NULL, &error_msg);
  if (ret != SQLITE_OK ) {
    // fprintf(stderr, "\nadd_event error: %s\n", error_msg);
    return RET_ERROR;
  }

  return RET_OK;
};


StorageRetCode BmStorage::search_event(unsigned int id, unsigned int max_event_num, long long start, long long end, unsigned int &event_count, long long *events) {
  ostringstream stream;
  if (end > 0)
  	stream << "SELECT TIME FROM EVENT WHERE ID = " << id << " AND TIME >= " << start << " AND TIME <= " << end << ";";
  else  	
    stream << "SELECT TIME FROM EVENT WHERE ID = " << id << " AND TIME >= " << start << ";";
  
  char *error_msg = NULL;
  vector<long long> event_vector;
  int ret = sqlite3_exec(database_handle_, stream.str().c_str(), bm_find_event, &event_vector, &error_msg);
  if (ret != SQLITE_OK ) {
    // fprintf(stderr, "\nsearch_event error: %s\n", error_msg);
    return RET_ERROR;
  }

  event_count = event_vector.size();
  if (event_count > max_event_num)
  	event_count = max_event_num;
  
  for (int i = 0; i < event_count; i++) {
    *(events + i) = event_vector[i];
  }

  return RET_OK;
}


StorageRetCode BmStorage::get_image(unsigned int id, cv::Mat &image) {
  image = cv::imread(kImagePathPrefix + to_string(id) + ".jpg", 1);

  if (image.empty())
  	return RET_ERROR;

  return RET_OK;
};


StorageRetCode BmStorage::get_info(unsigned int id, BitMainUsr &info) {

  ostringstream stream;
  stream << "SELECT NAME, EXTRA_INFO FROM PEOPLE WHERE ID = " << id << ";";
  
  char *error_msg = NULL;
  int ret = sqlite3_exec(database_handle_, stream.str().c_str(), bm_get_info, &info, &error_msg);

  if (ret != SQLITE_OK ) {
    // fprintf(stderr, "\nsearch_event error: %s\n", error_msg);
    return RET_ERROR;
  }

  info.user_id = id;
  
  return RET_OK;
};

static int bm_get_latest_event_time(void *para, int argc, char **value, char **name) {
  int i;
  istringstream stream;
  unsigned int id;
  long long *time = (long long *)para;

  if (strcasecmp("MAX(TIME)", name[0]) == 0) {
    if(value[0]==NULL)
	return RET_ERROR;
    else
        *time = std::stoll(value[0]);
  }

   return RET_OK;
}

StorageRetCode BmStorage::get_latest_event_time(unsigned int id, long long *latest_time) {

  ostringstream stream;
  stream << " SELECT MAX(TIME) FROM EVENT WHERE ID = " << id << ";";
  
  char *error_msg = NULL;
  int ret = sqlite3_exec(database_handle_, stream.str().c_str(), bm_get_latest_event_time, latest_time, &error_msg);

  if (ret != SQLITE_OK ) {
    // fprintf(stderr, "\nsearch_event error: %s\n", error_msg);
    return RET_ERROR;
  }

  return RET_OK;
};

