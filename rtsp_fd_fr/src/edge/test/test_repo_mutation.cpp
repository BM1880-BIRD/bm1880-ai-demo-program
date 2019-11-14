#include <memory>
#include <string>
#include <fstream>
#include <set>
#include <map>
#include "file_utils.hpp"
#include "db_repo.hpp"
#include "repository.hpp"
#include "gtest/gtest.h"

constexpr size_t DIM = 512;

template <typename T>
class RepoTest : public ::testing::Test {
public:
    static std::unique_ptr<MutableRepo<DIM>> init_repo();
};

struct tpu_db_repo_t {};
using RepoTypes = ::testing::Types<CachedRepo<DIM>, TpuRepo<DIM>, DBRepo<DIM>, tpu_db_repo_t>;

template <>
std::unique_ptr<MutableRepo<DIM>> RepoTest<CachedRepo<DIM>>::init_repo() {
    return std::unique_ptr<MutableRepo<DIM>>(new CachedRepo<DIM>("/tmp/repository"));
}
template <>
std::unique_ptr<MutableRepo<DIM>> RepoTest<TpuRepo<DIM>>::init_repo() {
    return std::unique_ptr<MutableRepo<DIM>>(new TpuRepo<DIM>(std::unique_ptr<MutableRepo<DIM>>(new CachedRepo<DIM>("/tmp/repository"))));
}
template <>
std::unique_ptr<MutableRepo<DIM>> RepoTest<DBRepo<DIM>>::init_repo() {
    return std::unique_ptr<MutableRepo<DIM>>(new DBRepo<DIM>("/tmp/repository.bin"));
}
template <>
std::unique_ptr<MutableRepo<DIM>> RepoTest<tpu_db_repo_t>::init_repo() {
    return std::unique_ptr<MutableRepo<DIM>>(new TpuRepo<DIM>(std::unique_ptr<MutableRepo<DIM>>(new DBRepo<DIM>("/tmp/repository.bin"))));
}

using feature_t = typename MutableRepo<DIM>::feature_t;
TYPED_TEST_CASE(RepoTest, RepoTypes);

template <typename ResultT>
static bool is_match_id(const ResultT &result, size_t id) {
    return result.matched && result.id == id;
}

template <size_t DIM>
static std::vector<typename Repo<DIM>::feature_t> feature_set(size_t n) {
    using namespace std;
    static_assert(DIM > 0, "DIM must > 0");
    using feature_t = typename Repo<DIM>::feature_t;
    vector<feature_t> f(n, feature_t(DIM));
    for (size_t i = 0; i < f.size(); i++) {
        auto t = i + 1;
        for (size_t j = 0; j < DIM && t > 0; j++, t /= 2) {
            f[i][j] = t % 2;
        }
    }

    return f;
}

TEST(TestRepoLegacy, test_handle_legacy) {
    using namespace std;

    {
        std::unique_ptr<MutableRepo<DIM>> repo(new CachedRepo<DIM>("/tmp/repository"));
        repo->remove_all();
    }

    set<string> names;
    for (int i = 0; i < 10; i++) {
        string name(i + 1, 'A' + i);
        ofstream fout("/tmp/repository/" + name + ".face");
        for (int j = 0; j < DIM; j++) {
            fout << i + 1 << endl;
        }
        names.insert(name);
    }

    auto check_repo = [&]() {
        set<string> found_names;
        std::unique_ptr<MutableRepo<DIM>> repo(new CachedRepo<DIM>("/tmp/repository"));
        auto ids = repo->id_list();
        ASSERT_EQ(names.size(), ids.size());
        for (auto &id : ids) {
            auto pair = repo->get(id);
            ASSERT_TRUE(pair.has_value());
            found_names.insert(pair.value().first);
            ASSERT_EQ(pair.value().second, CachedRepo<DIM>::feature_t(DIM, pair.value().first[0] - 'A' + 1));
        }

        ASSERT_EQ(names, found_names);
    };

    check_repo();
    for (int i = 0; i < 10; i++) {
        string name(i + 1, 'A' + i);
        ASSERT_FALSE(utils::file::exists("/tmp/repository/" + name + ".face"));
    }
    check_repo();
}

TYPED_TEST(RepoTest, remove_all) {
    auto repo = this->init_repo();
    repo->add("1234", vector<float>(DIM, 1));
    ASSERT_FALSE(repo->id_list().empty());
    repo.reset();

    repo = this->init_repo();
    ASSERT_FALSE(repo->id_list().empty());
    repo.reset();

    repo = this->init_repo();
    repo->remove_all();
    ASSERT_EQ(repo->id_list(), vector<size_t>());
    ASSERT_TRUE(repo->id_list().empty());
    repo.reset();

    repo = this->init_repo();
    ASSERT_EQ(repo->id_list(), vector<size_t>());
    ASSERT_TRUE(repo->id_list().empty());
}

TYPED_TEST(RepoTest, add) {
    using namespace std;
    {
        auto repo = this->init_repo();
        repo->remove_all();
    }

    auto universe = feature_set<DIM>(64);
    map<size_t, size_t> id_to_index;
    vector<size_t> index_to_id(universe.size());
    auto check = [&](std::unique_ptr<MutableRepo<DIM>> &repo) {
        auto ids = repo->id_list();
        ASSERT_EQ(ids.size(), id_to_index.size());
        for (auto id : ids) {
            auto p = repo->get(id);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(p.value().second, universe[id_to_index[id]]);
        }
    };

    {
        auto repo = this->init_repo();
        ASSERT_TRUE(repo->id_list().empty());

        for (size_t i = 0; i < universe.size(); i++) {
            ASSERT_FALSE(repo->match(universe[i], 0.95).matched);
            index_to_id[i] = repo->add(to_string(i), universe[i]);
            id_to_index[index_to_id[i]] = i;
            ASSERT_TRUE(is_match_id(repo->match(universe[i], 0.95), index_to_id[i]));
        }

        ASSERT_EQ(id_to_index.size(), universe.size());
        check(repo);
    }

    {
        auto repo = this->init_repo();
        check(repo);
    }
}

TYPED_TEST(RepoTest, add_with_set) {
    using namespace std;
    {
        auto repo = this->init_repo();
        repo->remove_all();
    }

    auto universe = feature_set<DIM>(64);
    map<size_t, size_t> id_to_index;
    vector<size_t> index_to_id(universe.size());
    auto check = [&](std::unique_ptr<MutableRepo<DIM>> &repo) {
        auto ids = repo->id_list();
        ASSERT_EQ(ids.size(), id_to_index.size());
        for (auto id : ids) {
            auto p = repo->get(id);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(p.value().second, universe[id_to_index[id]]);
        }
    };

    {
        auto repo = this->init_repo();
        ASSERT_TRUE(repo->id_list().empty());

        for (size_t i = 0; i < universe.size(); i++) {
            ASSERT_FALSE(repo->match(universe[i], 0.95).matched);
            repo->set(i + 1, to_string(i), universe[i]);
            index_to_id[i] = i + 1;
            id_to_index[index_to_id[i]] = i;
            ASSERT_TRUE(is_match_id(repo->match(universe[i], 0.95), index_to_id[i]));
        }

        ASSERT_EQ(id_to_index.size(), universe.size());
        check(repo);
    }

    {
        auto repo = this->init_repo();
        check(repo);
    }
}

TYPED_TEST(RepoTest, set) {
    using namespace std;
    {
        auto repo = this->init_repo();
        repo->remove_all();
    }

    auto universe = feature_set<DIM>(64);
    map<size_t, size_t> id_to_index;
    vector<size_t> index_to_id(universe.size());

    auto check = [&](std::unique_ptr<MutableRepo<DIM>> &repo) {
        auto ids = repo->id_list();
        ASSERT_EQ(ids.size(), id_to_index.size());
        for (auto id : ids) {
            auto p = repo->get(id);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(p->second, universe[id_to_index[id]]);
        }
    };

    {
        auto repo = this->init_repo();
        ASSERT_TRUE(repo->id_list().empty());

        for (size_t i = 0; i < universe.size(); i++) {
            index_to_id[i] = repo->add(to_string(i), universe[0]);
            id_to_index[index_to_id[i]] = i;
        }

        ASSERT_EQ(id_to_index.size(), universe.size());
    }

    {
        auto repo = this->init_repo();

        ASSERT_TRUE(is_match_id(repo->match(universe[0], 0.95), index_to_id[0]));;
        for (size_t i = 1; i < universe.size(); i++) {
            ASSERT_FALSE(repo->match(universe[i], 0.95).matched);
            repo->set(index_to_id[i], to_string(i), universe[i]);
        }

        for (size_t i = 0; i < universe.size(); i++) {
            ASSERT_TRUE(is_match_id(repo->match(universe[i], 0.95), index_to_id[i]));
        }
    }

    {
        auto repo = this->init_repo();
        check(repo);
        for (size_t i = 0; i < universe.size(); i++) {
            ASSERT_TRUE(is_match_id(repo->match(universe[i], 0.95), index_to_id[i]));
        }
    }
}

TYPED_TEST(RepoTest, remove) {
    using namespace std;
    {
        auto repo = this->init_repo();
        repo->remove_all();
    }

    auto universe = feature_set<DIM>(128);
    map<size_t, size_t> id_to_index;
    vector<size_t> index_to_id(universe.size());

    {
        auto repo = this->init_repo();
        ASSERT_TRUE(repo->id_list().empty());

        for (size_t i = 0; i < universe.size(); i++) {
            index_to_id[i] = repo->add(to_string(i), universe[i]);
            id_to_index[index_to_id[i]] = i;
        }
        for (size_t i = 0; i < universe.size(); i++) {
            ASSERT_TRUE(is_match_id(repo->match(universe[i], 0.95), index_to_id[i]));
            repo->remove(index_to_id[i]);
            ASSERT_FALSE(repo->match(universe[i], 0.95).matched);
        }

        ASSERT_TRUE(repo->id_list().empty());
    }

    {
        auto repo = this->init_repo();
        ASSERT_TRUE(repo->id_list().empty());
    }
}

TYPED_TEST(RepoTest, mixed_op) {
    using namespace std;
    {
        auto repo = this->init_repo();
        repo->remove_all();
    }

    srand(time(NULL));
    auto universe = feature_set<DIM>(255);
    map<size_t, size_t> index_to_id;
    map<size_t, size_t> id_to_index;

    auto repo = this->init_repo();

    for (size_t i = 0; i < 10000; i++) {
        auto index = rand() % universe.size();
        auto op = rand() % 10;
        if (op == 0) {
            repo->remove_all();
            index_to_id.clear();
            id_to_index.clear();
        }
        if (index_to_id.count(index) == 0) {
            ASSERT_FALSE(repo->match(universe[index], 0.95).matched);

            auto id = repo->add(to_string(index), universe[index]);
            index_to_id[index] = id;
            id_to_index[id] = index;

            ASSERT_TRUE(repo->match(universe[index], 0.95).matched);
            ASSERT_EQ(repo->match(universe[index], 0.95).id, index_to_id[index]);
        } else {
            ASSERT_TRUE(repo->match(universe[index], 0.95).matched);

            auto id = index_to_id[index];
            repo->remove(id);
            index_to_id.erase(index);
            id_to_index.erase(id);

            ASSERT_FALSE(repo->match(universe[index], 0.95).matched);
        }
    }
}

TYPED_TEST(RepoTest, mass) {
    using namespace std;

    constexpr size_t count = 5000;

    {
        auto repo = this->init_repo();
        repo->remove_all();
    }
    {
        auto repo = this->init_repo();
        for (size_t i = 0; i < count; i++) {
            repo->add("person_" + to_string(i), (typename MutableRepo<DIM>::feature_t)(DIM));
        }
        auto features = feature_set<DIM>(1);
        ASSERT_FALSE(repo->match(features[0], 0.1).matched);
    }
    {
        auto repo = this->init_repo();
        ASSERT_EQ(repo->id_list().size(), count);
        auto features = feature_set<DIM>(1);
        ASSERT_FALSE(repo->match(features[0], 0.1).matched);
        repo->remove_all();
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
