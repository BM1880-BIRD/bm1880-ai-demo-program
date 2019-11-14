#include <cassert>
#include "db_repo.hpp"
#include "cached_repo.hpp"
#include "repository.hpp"
#include "fdfr_version.h"

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    if (argc < 3) {
        return 0;
    }

    {
        CachedRepo<FACE_FEATURES_DIM> src(argv[1]);
        DBRepo<FACE_FEATURES_DIM> dst(argv[2]);

        dst.remove_all();

        for (auto id : src.id_list()) {
            auto p = src.get(id);
            if (p.has_value()) {
                dst.set(id, p->first, p->second);
            }
        }
    }
    {
        CachedRepo<FACE_FEATURES_DIM> src(argv[1]);
        DBRepo<FACE_FEATURES_DIM> dst(argv[2]);

        assert(src.id_list() == dst.id_list());

        for (auto id : src.id_list()) {
            auto p = src.get(id);
            auto q = dst.get(id);
            assert(p.has_value());
            assert(q.has_value());
            assert(p.value() == q.value());
        }
    }

}
