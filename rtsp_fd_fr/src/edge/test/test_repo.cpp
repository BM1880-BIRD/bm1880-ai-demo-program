#include <memory>

#include "repository.hpp"
#include "gtest/gtest.h"


TEST(TestRepoCompareByCpu, test_repo_compare_by_cpu) {
    std::unique_ptr<Repo<8>> repo(new CachedRepo<8>("./"));
    std::vector<float> features_a{0, 1, 2, 3, 4, 5, 6, 7};
    auto a_id = repo->add("a", features_a);

    std::vector<float> features_b{1, 3, 4, 1, 2, 1, 6, 7};
    auto b_id = repo->add("b", features_b);

    std::vector<float> features_c{3, 3, 3, 3, 3, 3, 3, 3};
    auto c_id = repo->add("c", features_c);
    repo->build();

    vector<typename Repo<8>::match_result> results = {
        repo->match(features_a, 0.1),
        repo->match(features_b, 0.1),
        repo->match(features_c, 0.1)
    };
    vector<size_t> ids = {a_id, b_id, c_id};

    for (size_t i = 0; i < results.size(); i++) {
        EXPECT_TRUE(results[i].matched);
        EXPECT_EQ(results[i].id, ids[i]);
    }
}

TEST(TestRepoCompareByAnnoy, test_repo_compare_by_annoy) {
    std::unique_ptr<Repo<8>> repo(new AnnoyRepo<float, 8>("./"));
    std::vector<float> features_a{0, 1, 2, 3, 4, 5, 6, 7};
    auto a_id = repo->add("a", features_a);

    std::vector<float> features_b{1, 3, 4, 1, 2, 1, 6, 7};
    auto b_id = repo->add("b", features_b);

    std::vector<float> features_c{3, 3, 3, 3, 3, 3, 3, 3};
    auto c_id = repo->add("c", features_c);
    repo->build();

    vector<typename Repo<8>::match_result> results = {
        repo->match(features_a, 0.1),
        repo->match(features_b, 0.1),
        repo->match(features_c, 0.1)
    };
    vector<size_t> ids = {a_id, b_id, c_id};

    for (size_t i = 0; i < results.size(); i++) {
        EXPECT_TRUE(results[i].matched);
        EXPECT_EQ(results[i].id, ids[i]);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
