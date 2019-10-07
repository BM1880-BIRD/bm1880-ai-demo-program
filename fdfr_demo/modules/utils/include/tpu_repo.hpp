#pragma once

#include "cached_repo.hpp"
#include "db_repo.hpp"
#include "core/net_loader.hpp"
#include <libbmblas/bmblas_api.h>
#include <libbmruntime/bmruntime.h>
#include <libbmruntime/bmruntime_bmkernel.h>

template <size_t DIM>
class TpuRepo : public MutableRepo<DIM> {
public:
    using typename Repo<DIM>::feature_t;

public:
    explicit TpuRepo(const std::string &path) {
        if (::utils::file::is_directory(path)) {
            underlying_.reset(new CachedRepo<DIM>(path, false));
        } else {
            underlying_.reset(new DBRepo<DIM>(path));
        }

        tpu_init();
    }

    explicit TpuRepo(std::unique_ptr<MutableRepo<DIM>> &&underlying_) : underlying_(std::move(underlying_)) {
        tpu_init();
    }

    TpuRepo(TpuRepo &&rhs) :
          underlying_(std::move(rhs.underlying_))
        , face_cnt_(rhs.face_cnt_)
        , faces_data_1_(std::move(rhs.faces_data_1_))
        , faces_data_2_(std::move(rhs.faces_data_2_))
        , face_data_len_(std::move(rhs.face_data_len_))
        , tpu_out_(std::move(rhs.tpu_out_))
        , ctx_(rhs.ctx_)
        , bk_ctx_(rhs.bk_ctx_)
        , gaddr_a_(rhs.gaddr_a_)
        , gaddr_b_(rhs.gaddr_b_)
        , gaddr_r_(rhs.gaddr_r_)
        , devmem_a_(rhs.devmem_a_)
        , devmem_b_(rhs.devmem_b_)
        , devmem_r_(rhs.devmem_r_) {}

    virtual ~TpuRepo() {
        tpu_deinit();
    }

    typename Repo<DIM>::match_result match(const feature_t &features, double threshold) override {
        assert(features.size() == DIM);

        if (is_using_cpu()) {
            return underlying_->match(features, threshold);
        }

        if (cpu_handle_ids.size() > (size_t)cpu_handle_threshold_) {
            update_tpu_data();
        }

        /* Compute src dot faces_data */
        std::vector<int> result(face_cnt_, 0);
        float src_len = 0;
        std::array<int8_t, DIM> src_1, src_2;

        for (size_t i = 0; i < DIM; i++) {
            src_1[i] = int(features[i]) / 16;
            src_2[i] = int(features[i]) % 16;
            src_len += features[i] * features[i];
        }
        src_len = sqrt(src_len);

        tpu_compute(src_1.data(), faces_data_1_.data(), result, 128);
        tpu_compute(src_1.data(), faces_data_2_.data(), result, 16);
        tpu_compute(src_2.data(), faces_data_1_.data(), result, 8);
        tpu_compute(src_2.data(), faces_data_2_.data(), result, 1);

        /* Find best match */
        typename Repo<DIM>::match_result best{false, 0, 0};
        const float score_threshold = threshold * src_len;

        for (size_t i = 0; i < face_cnt_; i++) {
            const double partial_score = result[i] / face_data_len_[i];
            size_t face_id = index_to_id[i];

            if (partial_score > best.score && underlying_->get_name(face_id).has_value()) {
                best.score = partial_score;
                best.id = face_id;

                if (partial_score >= score_threshold) {
                    best.matched = true;
                }
            }
        }
        best.score /= src_len;

        for (auto &face_id : cpu_handle_ids) {
            double score = underlying_->compute(face_id, features);
            if (score > best.score) {
                best.score = score;
                best.id = face_id;
                if (score > threshold) {
                    best.matched = true;
                }
            }
        }

        return best;
    }

    double compute(size_t id, const feature_t &features) override {
        return underlying_->compute(id, features);
    }

    void update_tpu_data() {
        tpu_deinit();
        tpu_init();
    }

    size_t add(const std::string &name, const feature_t &features) override {
        auto id = underlying_->add(name, features);
        cpu_handle_ids.emplace_back(id);
        return id;
    }

    void build() override {
        underlying_->build();
        update_tpu_data();
    }

    void set(size_t id, const std::string &name, const feature_t &features) override {
        underlying_->set(id, name, features);
        auto iter = id_to_index.find(id);
        if (iter != id_to_index.end()) {
            auto index = iter->second;
            set_face_data(index, features);
        } else {
            cpu_handle_ids.emplace_back(id);
        }
    }

    size_t size() const override {
        return underlying_->size();
    }

    std::vector<size_t> id_list() const override {
        return underlying_->id_list();
    }

    mp::optional<std::pair<std::string, feature_t>> get(size_t id) const {
        return underlying_->get(id);
    }

    mp::optional<std::string> get_name(size_t id) const {
        return underlying_->get_name(id);
    }

    void remove(size_t id) override {
        underlying_->remove(id);

        if ((underlying_->size() < face_cnt_ / 2) && (face_cnt_ > (size_t)face_min_cnt_)) {
            update_tpu_data();
        }
    }

    void remove_all() override {
        underlying_->remove_all();
        update_tpu_data();
    }

private:

    bool is_using_cpu() const {
        return (face_cnt_ < (size_t)face_min_cnt_);
    }

    void set_face_data(size_t index, const feature_t &features) {
        face_data_len_[index] = 0;
        for (size_t i = 0; i < DIM; i++) {
            const int val = features[i];

            faces_data_1_[i * face_cnt_ + index] = val / 8;
            faces_data_2_[i * face_cnt_ + index] = val % 8;
            face_data_len_[index] += val * val;
        }

        face_data_len_[index] = sqrt(face_data_len_[index]);
    }

    void tpu_init() {
        face_cnt_ = underlying_->size();
        index_to_id.resize(face_cnt_);
        id_to_index.clear();
        cpu_handle_ids.clear();

        if (is_using_cpu()) {
            return;
        }

        faces_data_1_.resize(DIM * face_cnt_);
        faces_data_2_.resize(DIM * face_cnt_);
        tpu_out_.resize(2 * face_cnt_);
        face_data_len_.resize(face_cnt_);

        auto id_list = underlying_->id_list();
        for (size_t i = 0; i < face_cnt_; i++) {
            index_to_id[i] = id_list[i];
            id_to_index[id_list[i]] = i;
            auto name_features = underlying_->get(id_list[i]);
            set_face_data(i, name_features->second);
        }

        ctx_ = qnn::NetLoader::Get().GetBmCtxt();
        if (!ctx_) {
            assert(false);
            return;
        }

        bmshape_t bms_a = BM_TENSOR_INT8(1, DIM, 1, 1);
        bmshape_t bms_b = BM_TENSOR_INT8(DIM, (int)face_cnt_, 1, 1);
        bmshape_t bms_r = BM_TENSOR_INT8(2, (int)face_cnt_, 1, 1);

        devmem_a_ = bmmem_device_alloc(ctx_, &bms_a);
        devmem_b_ = bmmem_device_alloc(ctx_, &bms_b);
        devmem_r_ = bmmem_device_alloc(ctx_, &bms_r);

        gaddr_a_ = bmmem_device_addr(ctx_, devmem_a_);
        gaddr_b_ = bmmem_device_addr(ctx_, devmem_b_);
        gaddr_r_ = bmmem_device_addr(ctx_, devmem_r_);

        bmruntime_bmkernel_create(ctx_, &bk_ctx_);
    }

    void tpu_deinit() {
        if (is_using_cpu()) {
            return;
        }

        bmruntime_bmkernel_destroy(ctx_);

        bmmem_device_free(ctx_, devmem_a_);
        bmmem_device_free(ctx_, devmem_b_);
        bmmem_device_free(ctx_, devmem_r_);

        tpu_out_.clear();
        faces_data_1_.clear();
        faces_data_2_.clear();
        face_data_len_.clear();

        index_to_id.clear();
        id_to_index.clear();
        cpu_handle_ids.clear();
    }

    void tpu_compute(int8_t *src_a, int8_t *src_b, std::vector<int> &result, int multiple) {
        bm_memcpy_s2d(ctx_, devmem_a_, (uint8_t *)src_a);
        bm_memcpy_s2d(ctx_, devmem_b_, (uint8_t *)src_b);

        bmblas_gemm(
            (bmk1880_context_t *)bk_ctx_,
            1, face_cnt_, DIM,
            gaddr_a_, gaddr_b_,
            gaddr_r_);

        bmruntime_bmkernel_submit(ctx_);
        bm_memcpy_d2s(ctx_, (uint8_t*)tpu_out_.data(), devmem_r_);

        int16_t ans;
        uint8_t *ptr = (uint8_t *)&ans;

        for (size_t i = 0; i < face_cnt_; ++i) {
            ptr[1] = tpu_out_[face_cnt_ + i];
            ptr[0] = tpu_out_[i];

            result[i] += (ans * multiple);
        }
    }

private:
    const int cpu_handle_threshold_ = 32;
    const int face_min_cnt_ = 32;
    std::unique_ptr<MutableRepo<DIM>> underlying_;
    size_t face_cnt_;
    std::map<size_t, size_t> id_to_index;
    std::vector<size_t> index_to_id;
    std::vector<size_t> cpu_handle_ids;
    std::vector<int8_t> faces_data_1_;
    std::vector<int8_t> faces_data_2_;
    std::vector<float> face_data_len_;
    std::vector<uint8_t> tpu_out_;
    bmctx_t ctx_;
    void *bk_ctx_;
    gaddr_t gaddr_a_;
    gaddr_t gaddr_b_;
    gaddr_t gaddr_r_;
    bmmem_device_t devmem_a_;
    bmmem_device_t devmem_b_;
    bmmem_device_t devmem_r_;
};
