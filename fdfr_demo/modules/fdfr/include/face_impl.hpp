#pragma once

#include "face_api.hpp"
#include "fdfr.hpp"
#include "repository.hpp"
#include "image.hpp"

class FaceImpl : public FaceAPI {
public:
    FaceImpl(FDFR &fdfr, Repository &&repo) : fdfr_(fdfr), repo_(std::move(repo)) {}

    Memory::SharedView<fr_result_t> faceid(const image_t &image) override;
    bool register_face(const std::string &name, image_t &&image) override;
    Memory::SharedView<fd_result_t> detect_face(const image_t &image) override;
    Memory::SharedView<fr_result_t> faceinfo(const image_t &image ,double* score);

protected:
    static constexpr double similarity_threshold = 0.5;
    FDFR &fdfr_;
    Repository repo_;
};
