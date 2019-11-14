#include "stream_cmd_client.hpp"
#include <signal.h>
#include "bt.hpp"
#include "fdfr_version.h"

using namespace std;

StreamCmdClient client;

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void { client.stop(); });
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); });
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); });

    client.addStream("stream_1");
    client.addStream("stream_2");
    client.run();

    return 0;
}