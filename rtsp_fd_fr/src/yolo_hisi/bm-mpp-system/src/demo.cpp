#include <iostream>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <chrono>
#include "bm_mpp.h"
#include "yolo.hpp"

using namespace bm_mpp;
using std::cout;
using std::endl;
using std::unique_ptr;
using std::vector;
using ms = std::chrono::duration<float, std::milli>;
std::chrono::high_resolution_clock timer;


int curl_post(const char* url, vector<unsigned char> data){
    // curl post jpeg
    CURL *curl;
    CURLcode res;
    struct curl_slist *hs=NULL;
    curl_global_init(CURL_GLOBAL_ALL);
    hs = curl_slist_append(hs, "Content-Type: application/octet-stream");
    curl = curl_easy_init();
    if(curl){
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reinterpret_cast<char*>(data.data()));
        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}

void print_help(char **argv){
    cout <<  argv[0] << " 0/1 " << " [restful web post url] "<< endl;
    cout << "Usage: 0 only stream" << endl 
         << "       1 do yolo " << endl 
         << "ex:./bm_mpp_system 0 " << endl
         << "ex: ./bm_mpp_system 1 http://10.34.36.152:5000/api/v1/streams/front_in" << endl;
}

vector<unsigned char> readfile(const char* name){
    std::ifstream input(name, std::ios::binary);
    vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
    return buffer;
}

int main(int argc, char *argv[]){
    if (argc < 2){
        print_help(argv);
        return 0;
    }
    // count fps
    std::unique_ptr<BM_MPP_SYSTEM> bm = BM_MPP_SYSTEM::get_instance(2);
    std::vector<object_detect_rect_t> results;
    std::size_t pos;
    std::string arg = argv[1];
    vector<unsigned char> image;
    if (arg == "1"){
        remote_yolo_init();
        while(true){
            image = bm->get();
            // image = readfile("test2.jpg");
            if(image.size()== 0) continue; 
            remote_yolo_send(std::move(image));
            break;
        }
    }
    while(true){
        auto start = timer.now();
        image = bm->get();
        // image = readfile("test2.jpg");
        if(image.size() ==0) continue;
        if(arg == "1"){
            remote_yolo_send(std::move(image));
            remote_yolo_receive(image, results);
            if(image.size() ==0) continue;
        }
        auto stop = timer.now();
        auto deltaTime = std::chrono::duration_cast<ms>(stop - start).count();
        cout << endl << deltaTime << "ms" << endl;
        cout << "fps: " << 1000.0/deltaTime << endl;
        if(argc == 3)
            curl_post(argv[2], image);
    }
    remote_yolo_free();
    return 0;
}
