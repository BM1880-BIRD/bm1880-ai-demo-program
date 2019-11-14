#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <signal.h>
#include "json.hpp"
#include "pipeline_preset.hpp"
#include "component_factory.hpp"
#include "pipe_executor.hpp"
#include "fdfr_version.h"

using namespace std;

map<string, pipeline::Executor> runners;

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void {
        for (auto &pair : runners) {
            pair.second.stop();
        }
    });

    if (argc != 2) {
        fprintf(stderr, "usage: %s <json file>\n", argv[0]);
        return -1;
    }

    ifstream fin(argv[1]);
    json config;
    fin >> config;

    pipeline::preset::ComponentFactory factory(config.at("components"));

    auto configs = config.at("pipeline_groups").get<map<string, json>>();
    for (auto &pair : configs) {
        auto pipeline_group = pipeline::preset::build(factory, pair.second);
        runners[pair.first].add_pipes(move(pipeline_group));
        runners.at(pair.first).start();
    }

    for (auto &pair : runners) {
        pair.second.join();
    }
}