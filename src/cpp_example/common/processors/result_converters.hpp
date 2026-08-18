/**
 * @file result_converters.hpp
 * @brief Conversion functions between legacy postprocess results and common result types
 * 
 * These functions convert model-specific result structures to common
 * result types for unified processing.
 */

#ifndef RESULT_CONVERTERS_HPP
#define RESULT_CONVERTERS_HPP

#include "common/base/i_processor.hpp"

// Merged postprocess headers
#include "anchorless_dfl_detection_postprocessor.hpp"
// Unique postprocess headers
#include "anchorless_instance_seg_postprocessor.hpp"

namespace dxapp {

// ============================================================================
// Common Coordinate Scaling Utilities
//
// Generic helpers for the recurring pattern:
//   coord = (coord - pad) / scale;  coord = clamp(coord, 0, original_dim);
//
// Used by scaleDetectionResults, scaleFaceResults, scalePoseResults,
// scaleInstanceSegResults.
// ============================================================================
namespace detail {

/**
 * @brief Scale a [x1, y1, x2, y2] bounding box from preprocessed to original coords.
 *
 * When pad_x==0 && pad_y==0 (SimpleResize, no letterbox), uses per-axis
 * scaling to correctly handle non-square aspect ratios.
 */
inline void scaleBox(std::vector<float>& box, const PreprocessContext& ctx) {
    if (box.size() < 4) return;

    if (ctx.pad_x == 0 && ctx.pad_y == 0 && ctx.scale_x > 0 && ctx.scale_y > 0) {
        // SimpleResize: per-axis scaling
        box[0] = box[0] / ctx.scale_x;
        box[1] = box[1] / ctx.scale_y;
        box[2] = box[2] / ctx.scale_x;
        box[3] = box[3] / ctx.scale_y;
    } else {
        // Letterbox: uniform scaling with padding removal
        box[0] = (box[0] - ctx.pad_x) / ctx.scale;
        box[1] = (box[1] - ctx.pad_y) / ctx.scale;
        box[2] = (box[2] - ctx.pad_x) / ctx.scale;
        box[3] = (box[3] - ctx.pad_y) / ctx.scale;
    }

    float w = static_cast<float>(ctx.original_width);
    float h = static_cast<float>(ctx.original_height);
    box[0] = std::max(0.0f, std::min(box[0], w));
    box[1] = std::max(0.0f, std::min(box[1], h));
    box[2] = std::max(0.0f, std::min(box[2], w));
    box[3] = std::max(0.0f, std::min(box[3], h));
}

/**
 * @brief Scale a single keypoint from preprocessed to original coords.
 */
inline void scaleKeypoint(Keypoint& kp, const PreprocessContext& ctx) {
    if (ctx.pad_x == 0 && ctx.pad_y == 0 && ctx.scale_x > 0 && ctx.scale_y > 0) {
        kp.x = kp.x / ctx.scale_x;
        kp.y = kp.y / ctx.scale_y;
    } else {
        kp.x = (kp.x - ctx.pad_x) / ctx.scale;
        kp.y = (kp.y - ctx.pad_y) / ctx.scale;
    }
}

}  // namespace detail

inline DetectionResult convert(const AnchorlessYOLOResult& src) {
    return DetectionResult(src.box, src.confidence, src.class_id, src.class_name);
}

// ============================================================================
// Instance Segmentation Result Converters
// Legacy struct uses: box, confidence, class_id, class_name,
//   mask(vector<float>), mask_height, mask_width
// ============================================================================

inline InstanceSegmentationResult convertToInstanceSeg(const YOLOv8SegResult& src) {
    InstanceSegmentationResult result;
    result.box = src.box;
    result.confidence = src.confidence;
    result.class_id = src.class_id;
    result.class_name = src.class_name;
    
    // Convert flat mask vector to cv::Mat (read-only wrap avoids allocation)
    if (!src.mask.empty() && src.mask_height > 0 && src.mask_width > 0) {
        cv::Mat mask_float(src.mask_height, src.mask_width, CV_32FC1,
                           const_cast<float*>(src.mask.data()));
        mask_float.convertTo(result.mask, CV_8UC1, 255.0);
    }
    
    return result;
}

// ============================================================================
// Batch Conversion Helpers
// ============================================================================

template <typename SrcType>
std::vector<DetectionResult> convertAll(const std::vector<SrcType>& src_results) {
    std::vector<DetectionResult> results;
    results.reserve(src_results.size());
    for (const auto& src : src_results) {
        results.push_back(convert(src));
    }
    return results;
}

template <typename SrcType, typename ConvertFunc>
auto convertAllWith(const std::vector<SrcType>& src_results, ConvertFunc convert_fn) 
    -> std::vector<decltype(convert_fn(std::declval<SrcType>()))> {
    std::vector<decltype(convert_fn(std::declval<SrcType>()))> results;
    results.reserve(src_results.size());
    for (const auto& src : src_results) {
        results.push_back(convert_fn(src));
    }
    return results;
}

}  // namespace dxapp

#endif  // RESULT_CONVERTERS_HPP
