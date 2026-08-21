/**
 * @file verify_serialize.hpp
 * @brief Serialize postprocess results to JSON for numerical verification.
 *
 * C++ port of Python's verify_serialize.py.
 * Activated by DXAPP_VERIFY=1 environment variable.
 * Writes one JSON per inference to logs/verify/{model_stem}.json.
 *
 * Requires nlohmann/json (bundled at common/third_party/nlohmann_json.hpp).
 */

#ifndef VERIFY_SERIALIZE_HPP
#define VERIFY_SERIALIZE_HPP

#include <cmath>
#include <cstdlib>
#include <set>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include <opencv2/opencv.hpp>
#include <tuple>
#include "common/third_party/nlohmann_json.hpp"
#include "common/base/i_processor.hpp"

namespace dxapp {
namespace verify {

using json = nlohmann::json;

// ============================================================================
// Environment check
// ============================================================================

inline bool isVerifyEnabled() {
    const char* v = std::getenv("DXAPP_VERIFY");
    return v && std::string(v) == "1";
}

inline std::string getVerifyDir() {
    const char* d = std::getenv("DXAPP_VERIFY_DIR");
    std::string dir = d ? d : "logs/verify";
    fs::create_directories(dir);
    return dir;
}

// ============================================================================
// cv::Mat statistics (mirrors Python's _np_stats)
// ============================================================================

inline json matStats(const cv::Mat& mat) {
    if (mat.empty()) return json::object();

    cv::Mat f64;
    mat.convertTo(f64, CV_64F);
    f64 = f64.reshape(1);  // flatten channels

    double minVal = 0;
    double maxVal = 0;
    cv::minMaxLoc(f64, &minVal, &maxVal);
    cv::Scalar mean;
    cv::Scalar stddev;
    cv::meanStdDev(f64, mean, stddev);

    bool has_nan = false;
    bool has_inf = false;
    for (int r = 0; r < f64.rows; ++r) {
        const double* row = f64.ptr<double>(r);
        for (int c = 0; c < f64.cols; ++c) {
            if (std::isnan(row[c])) has_nan = true;
            if (std::isinf(row[c])) has_inf = true;
            if (has_nan && has_inf) break;
        }
        if (has_nan && has_inf) break;
    }

    std::vector<int> shape;
    shape.push_back(mat.rows);
    shape.push_back(mat.cols);
    if (mat.channels() > 1) shape.push_back(mat.channels());

    std::string dtype_str;
    if (mat.depth() == CV_32F) dtype_str = "float32";
    else if (mat.depth() == CV_64F) dtype_str = "float64";
    else dtype_str = "uint8";

    return {
        {"shape", shape},
        {"dtype", dtype_str},
        {"min", minVal},
        {"max", maxVal},
        {"mean", mean[0]},
        {"std", stddev[0]},
        {"has_nan", has_nan},
        {"has_inf", has_inf}
    };
}

// ============================================================================
// Serializers per result type
// ============================================================================

inline json serializeDetection(const std::vector<DetectionResult>& items, int img_h, int img_w) {
    json dets = json::array();
    for (const auto& d : items) {
        dets.push_back({
            {"bbox", d.box},
            {"conf", d.confidence},
            {"class_id", d.class_id},
            {"class_name", d.class_name}
        });
    }
    return {{"image_height", img_h}, {"image_width", img_w}, {"detections", dets}};
}

inline json serializePose(const std::vector<PoseResult>& items, int img_h, int img_w) {
    json dets = json::array();
    for (const auto& d : items) {
        json kpts = json::array();
        for (const auto& kp : d.keypoints) {
            kpts.push_back({{"x", kp.x}, {"y", kp.y}, {"conf", kp.confidence}});
        }
        dets.push_back({
            {"bbox", d.box},
            {"conf", d.confidence},
            {"keypoints", kpts}
        });
    }
    return {{"image_height", img_h}, {"image_width", img_w}, {"detections", dets}};
}

inline json serializeInstanceSeg(const std::vector<InstanceSegmentationResult>& items, int img_h, int img_w) {
    json dets = json::array();
    for (const auto& d : items) {
        bool has_mask = !d.mask.empty();
        std::vector<int> mask_shape;
        if (has_mask) { mask_shape.push_back(d.mask.rows); mask_shape.push_back(d.mask.cols); }
        dets.push_back({
            {"bbox", d.box},
            {"conf", d.confidence},
            {"class_id", d.class_id},
            {"class_name", d.class_name},
            {"has_mask", has_mask},
            {"mask_shape", mask_shape}
        });
    }
    return {{"image_height", img_h}, {"image_width", img_w}, {"detections", dets}};
}

inline json serializeDepth(const std::vector<DepthResult>& items, int img_h, int img_w) {
    json result = {{"image_height", img_h}, {"image_width", img_w}};
    if (!items.empty()) {
        result["output_stats"] = matStats(items[0].depth_map);
    }
    return result;
}

inline json serializeClassification(const std::vector<ClassificationResult>& items, int img_h, int img_w) {
    json result = {{"image_height", img_h}, {"image_width", img_w}};
    if (!items.empty()) {
        const auto& c = items[0];
        result["classifications"] = json::array({
            {{"class_id", c.class_id}, {"class_name", c.class_name}, {"conf", c.confidence}}
        });
        json top_k_confs = json::array();
        for (const auto& p : c.top_k) {
            top_k_confs.push_back(p.second);
        }
        result["top_k_confs"] = top_k_confs;
    }
    return result;
}

// ============================================================================
// Internal write helper
// ============================================================================

namespace detail {

inline std::string writeVerifyJson(
    json data,
    const std::string& modelPath,
    const std::string& task) {

    data["task"] = task;
    data["model"] = fs::path(modelPath).filename().string();
    data["model_path"] = modelPath;

    std::string verifyDir = getVerifyDir();
    std::string modelStem = fs::path(modelPath).stem().string();
    std::string jsonPath = verifyDir + "/" + modelStem + ".json";

    std::ofstream ofs(jsonPath);
    if (!ofs.is_open()) {
        std::cerr << "[DXAPP] [WARN] verify_serialize: cannot open " << jsonPath << std::endl;
        return "";
    }
    ofs << data.dump(2) << std::endl;
    ofs.close();

    std::cout << "[VERIFY] Dumped -> " << jsonPath << std::endl;
    return jsonPath;
}

}  // namespace detail

// ============================================================================
// Overloaded dump functions per result type
// ============================================================================

#define DXAPP_VERIFY_DUMP_IMPL(ResultType, serializeFn) \
inline std::string dumpVerifyJson( \
    const std::vector<ResultType>& results, \
    const std::string& modelPath, \
    const std::string& task, \
    int img_h, int img_w) { \
    if (!isVerifyEnabled()) return ""; \
    try { \
        return detail::writeVerifyJson(serializeFn(results, img_h, img_w), modelPath, task); \
    } catch (const std::exception& e) { \
        std::cerr << "[DXAPP] [WARN] verify_serialize failed: " << e.what() << std::endl; \
        return ""; \
    } \
}

DXAPP_VERIFY_DUMP_IMPL(DetectionResult, serializeDetection)
DXAPP_VERIFY_DUMP_IMPL(PoseResult, serializePose)
DXAPP_VERIFY_DUMP_IMPL(InstanceSegmentationResult, serializeInstanceSeg)
DXAPP_VERIFY_DUMP_IMPL(DepthResult, serializeDepth)
DXAPP_VERIFY_DUMP_IMPL(ClassificationResult, serializeClassification)

#undef DXAPP_VERIFY_DUMP_IMPL

}  // namespace verify
}  // namespace dxapp

#endif  // VERIFY_SERIALIZE_HPP
