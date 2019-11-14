#include "pipeline_preset.hpp"
#include "pipeline_core.hpp"
#include "vector_pack.hpp"

using namespace std;

namespace pipeline {
namespace preset {

shared_ptr<pipeline::Pipeline> build_pipeline(ComponentFactory &factory, const json &config, shared_ptr<DataPipe<VectorPack>> input_pipe, shared_ptr<DataPipe<VectorPack>> output_pipe) {
    auto comp_names = config.at("components").get<vector<string>>();
    vector<shared_ptr<pipeline::TypeErasedComponent>> comps;

    for (auto &name : comp_names) {
        comps.emplace_back(factory.get(name));
    }

    if (input_pipe == nullptr && output_pipe == nullptr) {
        return pipeline::make(
            []() -> VectorPack {
                return VectorPack();
            },
            [comps](VectorPack &vp) {
                for (auto &comp : comps) {
                    comp->process(forward_as_tuple(vp));
                }
            }
        );
    } else if (input_pipe == nullptr) {
        return pipeline::make(
            []() -> VectorPack {
                return VectorPack();
            },
            [comps](VectorPack &vp) {
                for (auto &comp : comps) {
                    comp->process(forward_as_tuple(vp));
                }
            },
            pipeline::wrap_sink(output_pipe)
        );
    } else if (output_pipe == nullptr) {
        return pipeline::make(
            pipeline::wrap_source(input_pipe),
            [comps](VectorPack &vp) {
                for (auto &comp : comps) {
                    comp->process(forward_as_tuple(vp));
                }
            }
        );
    } else {
        return pipeline::make(
            pipeline::wrap_source(input_pipe),
            [comps](VectorPack &vp) {
                for (auto &comp : comps) {
                    comp->process(forward_as_tuple(vp));
                }
            },
            pipeline::wrap_sink(output_pipe)
        );
    }
}

vector<shared_ptr<pipeline::Pipeline>> build(ComponentFactory &factory, const json &config) {
    vector<shared_ptr<pipeline::Pipeline>> pipelines;
    shared_ptr<DataPipe<VectorPack>> data_pipe;

    vector<string> input_type;
    for (int i = 0; i < config.at("pipelines").size(); i++) {
        auto &pipe_conf = config.at("pipelines")[i];
        vector<string> output_type;
        size_t buffer_size = 0;
        data_pipe_mode pipe_mode;

        if (pipe_conf.contains("buffer_size")) {
            pipe_conf.at("buffer_size").get_to(buffer_size);
        }
        pipe_mode = pipe_conf.contains("buffer_mode") && pipe_conf.at("buffer_mode") == "blocking"
                    ? data_pipe_mode::blocking : data_pipe_mode::dropping;

        shared_ptr<DataPipe<VectorPack>> next_data_pipe;
        if (i + 1 < config.at("pipelines").size()) {
            next_data_pipe = make_shared<DataPipe<VectorPack>>(buffer_size, pipe_mode);
        }

        pipelines.emplace_back(build_pipeline(factory, pipe_conf, data_pipe, next_data_pipe));
        data_pipe = next_data_pipe;
    }

    return pipelines;
}

}
}
