#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <memory>
#include "json.hpp"
#include "pipeline_core.hpp"
#include "component_factory.hpp"
#include "data_pipe.hpp"
#include "vector_pack.hpp"

namespace pipeline {
namespace preset {

std::shared_ptr<pipeline::Pipeline> build_pipeline(ComponentFactory &factory, const json &config, std::shared_ptr<DataPipe<VectorPack>>, std::shared_ptr<DataPipe<VectorPack>>);
std::vector<std::shared_ptr<pipeline::Pipeline>> build(ComponentFactory &factory, const json &config);

}
}