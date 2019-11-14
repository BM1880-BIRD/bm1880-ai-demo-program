#pragma once

#include <memory>
#include <string>
#include <map>
#include <type_traits>
#include <functional>
#include "json.hpp"
#include "vector_pack.hpp"
#include "pipe_vector_wrap.hpp"

namespace pipeline {
namespace preset {

class ComponentFactory {
public:
    ComponentFactory(const json &conf);

    std::shared_ptr<TypeErasedComponent> get(const std::string &name) {
        auto iter = stash.find(name);
        if (iter != stash.end()) {
            return iter->second;
        } else {
            auto comp = create(name);
            if (config.at(name).contains("shareable") && config.at(name).at("shareable").get<bool>()) {
                stash[name] = comp;
            }
            return comp;
        }
    }

    json get_config(const std::string &name) {
        return config.at(name);
    }

private:
    std::shared_ptr<TypeErasedComponent> create(const std::string &name);

    using create_func_t = typename std::function<std::shared_ptr<TypeErasedComponent>(const json &)>;
    std::map<std::string, create_func_t> creators;
    json config;
    std::map<std::string, std::shared_ptr<TypeErasedComponent>> stash;
};

}
}
