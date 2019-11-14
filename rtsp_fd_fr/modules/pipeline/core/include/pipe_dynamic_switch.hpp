#pragma once

#include <memory>
#include <tuple>
#include <map>
#include <functional>
#include "pipe_component.hpp"
#include "mp/tuple.hpp"

namespace pipeline {

    template <typename TagType, typename InType, typename OutType>
    class DynamicSwitch;

    template <typename TagType, typename... InTypes, typename... OutTypes>
    class DynamicSwitch<TagType, std::tuple<InTypes...>, std::tuple<OutTypes...>> : public Component<TagType &&, InTypes...> {
        typedef std::function<std::tuple<OutTypes...>(std::tuple<InTypes...>)> func_type;
    public:

        template <typename T>
        void set_command(const TagType &tag, std::shared_ptr<T> s) {
            s->set_context(this->context_);
            comps[tag] = [s](std::tuple<InTypes...> t) {
                return mp::tuple_cast<std::tuple<OutTypes...>>(s->process(t));
            };
        }

        void set_command(const TagType &tag, func_type &&f) {
            comps[tag] = f;
        }

        template <typename T>
        void set_default_command(std::shared_ptr<T> s) {
            s->set_context(this->context_);
            default_comp = [s](std::tuple<InTypes...> &&t) {
                return mp::tuple_cast<std::tuple<OutTypes...>>(s->process(mp::tuple_cast<std::tuple<InTypes...>>(std::move(t))));
            };
        }

        void set_default_command(func_type &&f) {
            default_comp = f;
        }

        bool accepts(const TagType &tag) const {
            return comps.find(tag) != comps.end();
        }

        std::tuple<OutTypes...> process(std::tuple<TagType &&, InTypes...> args) {
            auto &tag = std::get<0>(args);
            auto iter = comps.find(tag);
            if (iter == comps.end()) {
                if (default_comp) {
                    return default_comp(mp::subtuple_convert<std::tuple<InTypes...>>(args));
                } else {
                    throw std::runtime_error("DynamicSwitch: handler of given tag is not defined and default behavior is not set.");
                }
            }
            return iter->second(mp::subtuple_convert<std::tuple<InTypes...>>(args));
        }

    private:
        func_type default_comp;
        std::map<TagType, func_type> comps;
    };

}
