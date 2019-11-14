#include <gtest/gtest.h>
#include <set>
#include <utility>
#include <string>
#include "pipeline_core.hpp"

using namespace std;

template <typename T>
class Variable {
public:
    static set<void*> &valid() {
        static set<void*> s;
        return s;
    }
    static set<void*> &invalid() {
        static set<void*> s;
        return s;
    }
    static size_t &move_count() {
        static size_t s;
        return s;
    }

    Variable(const T &d) : data(d) {
        valid().emplace(this);
    }
    Variable(const Variable &other) = delete;
    Variable(Variable &&other) : data(std::move(other.data)) {
        if (valid().count(&other) != 1) {
            throw std::runtime_error("error in move");
        }
        valid().insert(this);
        valid().erase(&other);
        invalid().insert(&other);
        move_count()++;
    }
    ~Variable() {
        assert(valid().count(this) + invalid().count(this) == 1);
        valid().erase(this);
        invalid().erase(this);
    }

private:
    T data;
};

TEST(PipelineZeroCopy, lambda) {
    Variable<int>::move_count() = 0;
    auto p = pipeline::make(
        []() {
            return Variable<int>(3);
        },
        [](Variable<int> &) {
        },
        [](const Variable<int> &) {
        },
        [](Variable<int> &&t) {
            auto a = move(t);
        }
    );

    ASSERT_TRUE(p->execute_once());
    ASSERT_LE(Variable<int>::move_count(), 2);
}

TEST(PipelineZeroCopy, lambda_seq) {
    Variable<int>::move_count() = 0;

    auto p = pipeline::make(
        pipeline::make_sequence(
            []() {
                return Variable<int>(3);
            },
            [](Variable<int> &) {
            },
            [](const Variable<int> &) {
            }
        ),
        [](Variable<int> &) {
        },
        [](const Variable<int> &) {
        },
        [](Variable<int> &&t) {
            auto a = move(t);
        }
    );

    ASSERT_TRUE(p->execute_once());
    ASSERT_LE(Variable<int>::move_count(), 4);
}

TEST(PipelineZeroCopy, lambda_seq_depth2) {
    Variable<int>::move_count() = 0;

    auto p = pipeline::make(
        pipeline::make_sequence(
            pipeline::make_sequence(
                []() {
                    return Variable<int>(3);
                },
                [](Variable<int> &) {
                },
                [](const Variable<int> &) {
                }
            ),
            [](Variable<int> &) {
            },
            [](const Variable<int> &) {
            }
        ),
        [](Variable<int> &) {
        },
        [](const Variable<int> &) {
        },
        [](Variable<int> &&t) {
            auto a = move(t);
        }
    );

    ASSERT_TRUE(p->execute_once());
    ASSERT_LE(Variable<int>::move_count(), 6);
}

TEST(PipelineZeroCopy, lambda_borrow_depth2) {
    Variable<int>::move_count() = 0;

    auto p = pipeline::make(
        []() {
            return Variable<int>(3);
        },
        pipeline::make_sequence(
            pipeline::make_sequence(
                [](Variable<int> &) {
                },
                [](const Variable<int> &) {
                }
            ),
            [](Variable<int> &) {
            },
            [](const Variable<int> &) {
            }
        ),
        [](Variable<int> &) {
        },
        [](const Variable<int> &) {
        },
        [](Variable<int> &&t) {
            auto a = move(t);
        }
    );

    ASSERT_TRUE(p->execute_once());
    ASSERT_LE(Variable<int>::move_count(), 2);
}

TEST(PipelineZeroCopy, lambda_switch) {
    Variable<int>::move_count() = 0;

    auto p = pipeline::make(
        []() {
            return true;
        },
        []() {
            return Variable<int>(3);
        },
        pipeline::make_switch<bool>(
            make_pair(
                true,
                pipeline::make_sequence(
                    [](Variable<int> &) {
                    },
                    [](const Variable<int> &) {
                    }
                )
            ),
            make_pair(
                false,
                pipeline::make_sequence(
                    [](Variable<int> &) {
                    },
                    [](const Variable<int> &) {
                    }
                )
            )
        ),
        [](Variable<int> &) {
        },
        [](const Variable<int> &) {
        },
        [](Variable<int> &&t) {
            auto a = move(t);
        }
    );

    ASSERT_TRUE(p->execute_once());
    ASSERT_LE(Variable<int>::move_count(), 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
