#include <signal.h>
#include <stream_cmd_server.hpp>
#include "fdfr_version.h"

using namespace std;

StreamCmdServer server("../../../src/bm1880/stream_cmd_server.conf");

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void {
        server.stop();
    });

    if (argc == 2) {
        server.setConfigPath(argv[1]);
    }

    // Add custom command to server
    server.addCmd("test", [](tuple<string>) -> tuple<string> {
        return {"hello world"};
    });

    server.run();

    return 0;
}