#pragma once

#include <string>
#include <mutex>
#include <map>
#include <utility>

namespace pipeline {

    class Context {
    public:
        struct VariableBase {
            virtual ~VariableBase() {}
        };

        template <typename T>
        struct Variable : public VariableBase {
            Variable(T &&in) : value(std::move(in)) {}

            T value;
            std::mutex mutex;
        };

        template <typename T, typename U>
        bool set_default(const std::string &key, U &&value) {
            std::lock_guard<std::mutex> lock(mutex);
            if (variables.find(key) == variables.end()) {
                variables[key] = std::make_shared<Variable<typename std::decay<T>::type>>(std::forward<U>(value));
                return true;
            }
            return false;
        }

        template <typename T, typename U>
        bool set(const std::string &key, U &&value) {
            std::lock_guard<std::mutex> lock(mutex);
            auto iter = variables.find(key);
            if (iter == variables.end()) {
                variables[key] = std::make_shared<Variable<typename std::decay<T>::type>>(std::forward<U>(value));
                return true;
            }

            auto var = std::dynamic_pointer_cast<Variable<T>>(iter->second);
            if (var == nullptr) {
                return false;
            } else {
                std::lock_guard<std::mutex> lock(var->mutex);
                var->value = std::forward<U>(value);
                return true;
            }
        }

        template <typename T>
        std::shared_ptr<Variable<T>> get(const std::string &key) {
            std::lock_guard<std::mutex> lock(mutex);
            auto iter = variables.find(key);
            if (iter == variables.end()) {
                return nullptr;
            } else {
                return std::dynamic_pointer_cast<Variable<T>>(iter->second);
            }
        }

        template <typename T, typename U>
        std::shared_ptr<Variable<T>> get_default(const std::string &key, U &&value) {
            std::lock_guard<std::mutex> lock(mutex);
            auto iter = variables.find(key);

            if (iter == variables.end()) {
                iter = variables.emplace(key, std::make_shared<Variable<typename std::decay<T>::type>>(std::forward<U>(value))).first;
            }

            return std::dynamic_pointer_cast<Variable<T>>(iter->second);
        }

    private:
        std::mutex mutex;
        std::map<std::string, std::shared_ptr<VariableBase>> variables;
    };

}