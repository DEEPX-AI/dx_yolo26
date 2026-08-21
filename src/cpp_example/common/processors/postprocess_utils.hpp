/**
 * @file postprocess_utils.hpp
 * @brief Common postprocessing utility functions
 *
 * Extracted shared logic that was duplicated across multiple legacy postprocess
 * headers: sigmoid, IoU computation, and Non-Maximum Suppression (NMS).
 *
 * 
 * NMS selection guide:
 *   - Legacy postprocessors (own Result structs with iou() method):
 *       use postprocess_utils::apply_nms<T>(dets, threshold)
 *   - v3-native postprocessors (working with cv::Rect arrays):
 *       use postprocess_utils::nms_indices(boxes, scores, score_thr, nms_thr)
 *   Both implement the same greedy NMS algorithm.
 */
#ifndef POSTPROCESS_UTILS_HPP
#define POSTPROCESS_UTILS_HPP

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <dxrt/dxrt_api.h>
#include <opencv2/core.hpp>
#include <cstddef>

/**
 * @brief Dedicated exception for postprocessing configuration errors.
 *
 * Thrown when model output tensors do not match the expected shape or type.
 * Satisfies SonarQube rule: "Define and throw a dedicated exception."
 */
class PostprocessConfigError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace postprocess_utils {

// ============================================================================
// Format tensor shapes for error messages
// Eliminates deeply-nested shape-printing loops at every call site.
// ============================================================================

/**
 * @brief Format tensor shapes for error messages (shape only).
 *
 * Produces lines like: "    Output shape [0]: (1, 3, 640, 640)\n"
 */
inline std::string format_tensor_shapes(const dxrt::TensorPtrs& outputs) {
    std::ostringstream msg;
    int idx = 0;
    for (const auto& o : outputs) {
        msg << "    Output shape [" << idx << "]: (";
        ++idx;
        const auto& sh = o->shape();
        for (size_t j = 0; j < sh.size(); ++j) {
            msg << sh[j];
            if (j != sh.size() - 1) msg << ", ";
        }
        msg << ")\n";
    }
    return msg.str();
}

/**
 * @brief Format tensor shapes with type info for error messages.
 *
 * Produces lines like: "    Output shape [0]: (1, 3, 640, 640), Type = BBOX\n"
 */
inline std::string format_tensor_shapes_with_type(const dxrt::TensorPtrs& outputs) {
    std::ostringstream msg;
    int idx = 0;
    for (const auto& o : outputs) {
        msg << "    Output shape [" << idx << "]: (";
        ++idx;
        const auto& sh = o->shape();
        for (size_t j = 0; j < sh.size(); ++j) {
            msg << sh[j];
            if (j != sh.size() - 1) msg << ", ";
        }
        msg << "), Type = " << o->type() << "\n";
    }
    return msg.str();
}

// ============================================================================
// Sigmoid activation
// Previously duplicated in: anchor_yolo, anchorless_yolo, yolov5face,
//     yolov7face, yolov8seg postprocess headers
// ============================================================================
inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }

// ============================================================================
// IoU (Intersection over Union) for axis-aligned bounding boxes
// box format: [x1, y1, x2, y2]
// Previously duplicated (with identical logic) in 10 Result struct types
// ============================================================================
inline float compute_iou(const std::vector<float>& a,
                         const std::vector<float>& b) {
    float x_left   = std::max(a[0], b[0]);
    float y_top    = std::max(a[1], b[1]);
    float x_right  = std::min(a[2], b[2]);
    float y_bottom = std::min(a[3], b[3]);

    if (x_right < x_left || y_bottom < y_top) return 0.0f;

    float inter = (x_right - x_left) * (y_bottom - y_top);
    float area_a = (a[2] - a[0]) * (a[3] - a[1]);
    float area_b = (b[2] - b[0]) * (b[3] - b[1]);

    return inter / (area_a + area_b - inter);
}

// ============================================================================
// Generic Non-Maximum Suppression
// T must have: float confidence, float iou(const T&) const
// Previously duplicated (with identical logic) in 10 PostProcess classes
// ============================================================================
template <typename T>
std::vector<T> apply_nms(const std::vector<T>& dets, float threshold) {
    if (dets.empty()) return {};

    auto sorted = dets;
    std::sort(sorted.begin(), sorted.end(),
              [](const T& a, const T& b) { return a.confidence > b.confidence; });

    std::vector<bool> suppressed(sorted.size(), false);
    std::vector<T> result;

    for (size_t i = 0; i < sorted.size(); ++i) {
        if (suppressed[i]) continue;
        result.push_back(sorted[i]);
        for (size_t j = i + 1; j < sorted.size(); ++j) {
            if (!suppressed[j] && sorted[i].iou(sorted[j]) >= threshold)
                suppressed[j] = true;
        }
    }

    return result;
}

// ============================================================================
// Index-based Non-Maximum Suppression over parallel box/score arrays.
//
// Same greedy algorithm and the same selection rules as cv::dnn::NMSBoxes
// (score strictly above score_threshold, stable sort by descending score,
// suppress when IoU strictly above nms_threshold), implemented here so the
// applications do not need the OpenCV dnn module - Yocto/embedded OpenCV
// builds routinely ship without it.
// ============================================================================
inline std::vector<int> nms_indices(const std::vector<cv::Rect>& boxes,
                                   const std::vector<float>& scores,
                                   float score_threshold,
                                   float nms_threshold) {
    std::vector<int> order;
    order.reserve(scores.size());
    for (size_t i = 0; i < scores.size(); ++i) {
        if (scores[i] > score_threshold) order.push_back(static_cast<int>(i));
    }
    std::stable_sort(order.begin(), order.end(),
                     [&scores](int a, int b) { return scores[a] > scores[b]; });

    std::vector<int> keep;
    for (int idx : order) {
        bool suppressed = false;
        for (int kept : keep) {
            const float inter = static_cast<float>((boxes[idx] & boxes[kept]).area());
            const float uni = static_cast<float>(boxes[idx].area() + boxes[kept].area()) - inter;
            if (uni > 0.0f && inter / uni > nms_threshold) {
                suppressed = true;
                break;
            }
        }
        if (!suppressed) keep.push_back(idx);
    }
    return keep;
}

}  // namespace postprocess_utils

#endif  // POSTPROCESS_UTILS_HPP
