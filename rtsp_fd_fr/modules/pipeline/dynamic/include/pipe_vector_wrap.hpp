#pragma once

#include <tuple>
#include <vector>
#include <memory>
#include "pipeline_core.hpp"
#include "vector_pack.hpp"

namespace pipeline {

    class TypeErasedComponent {
    public:
        virtual ~TypeErasedComponent() {}
        virtual std::tuple<> process(std::tuple<VectorPack &>) = 0;
        virtual void set_context(std::shared_ptr<Context> context) = 0;
        virtual void close() = 0;
    };

    namespace detail {
        template <typename T>
        struct VectorWrapHelper {
            static T &get(VectorPack &vp) {
                auto &vec = vp.get<T>();
                if (vec.empty()) {
                    throw std::out_of_range("");
                }
                return vec[0];
            }
            static void add(VectorPack &vp, T &&value) {
                vp.add(std::vector<T>{value});
            }
        };

        template <>
        struct VectorWrapHelper<VectorPack> {
            static VectorPack &get(VectorPack &vp) {
                return vp;
            }
        };

        template <typename T>
        struct VectorWrapHelper<std::vector<T>> {
            static std::vector<T> &get(VectorPack &vp) {
                return vp.get<T>();
            }
            static void add(VectorPack &vp, std::vector<T> &&vec) {
                vp.add(std::move(vec));
            }
        };

        template <typename Tuple, size_t I, size_t Size>
        struct VectorAddHelper {
            static void add(VectorPack &vp, Tuple &tup) {
                VectorWrapHelper<typename std::tuple_element<I, Tuple>::type>::add(vp, std::move(std::get<I>(tup)));
                VectorAddHelper<Tuple, I + 1, Size>::add(vp, tup);
            }
        };

        template <typename Tuple, size_t Size>
        struct VectorAddHelper<Tuple, Size, Size> {
            static void add(VectorPack &vp, Tuple &tup) {}
        };
    }

    template <typename Comp, typename InputType>
    class VectorPackWrap;

    template <typename Comp, typename... InputTypes>
    class VectorPackWrap<Comp, std::tuple<InputTypes...>> : public TypeErasedComponent {
    public:
        explicit VectorPackWrap(std::shared_ptr<Comp> comp) : comp_(comp) {}

        std::tuple<> process(std::tuple<VectorPack &> tup) override {
            auto &vp = std::get<0>(tup);
            auto args = std::forward_as_tuple(detail::VectorWrapHelper<typename std::decay<InputTypes>::type>::get(vp)...);
            auto result = comp_->process(mp::subtuple_convert<std::tuple<InputTypes...>>(args));
            detail::VectorAddHelper<decltype(result), 0, std::tuple_size<decltype(result)>::value>::add(vp, result);

            return std::make_tuple();
        }

        void set_context(std::shared_ptr<Context> context) override {
            comp_->set_context(context);
        }
        void close() override {
            comp_->close();
        }

    private:
        std::shared_ptr<Comp> comp_;
    };

    template <typename Comp>
    std::shared_ptr<TypeErasedComponent> make_type_erased(std::shared_ptr<Comp> comp) {
        return std::make_shared<VectorPackWrap<Comp, typename detail::ResolveInterface<Comp>::input_type>>(comp);
    }
}