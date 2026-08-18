/**
 * @file yolo_detection_postprocessor.hpp
 * @brief Unified YOLO Detection Postprocessors for v3 interface
 * 
 * Groups all YOLO-based object detection postprocessors:
 *   - YOLOv5 family (6 args): YOLOv5, YOLOv7, YOLOX
 *   - YOLOv8 family (5 args): YOLOv8, YOLOv9, YOLOv10, YOLOv11, YOLOv12, YOLOv26
 */

#ifndef YOLO_DETECTION_POSTPROCESSOR_HPP
#define YOLO_DETECTION_POSTPROCESSOR_HPP

#include "common/base/i_processor.hpp"
#include "common/processors/result_converters.hpp"

// Merged postprocess headers
#include "anchorless_dfl_detection_postprocessor.hpp"

namespace dxapp {

// ============================================================================
// Base template for coordinate scaling (shared by all detection postprocessors)
// ============================================================================
namespace detail {

inline void scaleDetectionResults(std::vector<DetectionResult>& results,
                                   const PreprocessContext& ctx) {
    for (auto& det : results) {
        scaleBox(det.box, ctx);
    }
}

}  // namespace detail

// ============================================================================
// YOLOv8 Family Postprocessor Template
//
// Each YOLOv8+ variant (v8/v9/v10/v11/v12/v26) has its own legacy
// PostProcess class with different tensor parsing logic.
// This template provides the common process() → legacy → convert → scale flow.
// Subclasses override getModelName() only.
// ============================================================================
template<typename LegacyPostProcess>
class YOLOv8FamilyPostprocessor : public IPostprocessor<DetectionResult> {
public:
    YOLOv8FamilyPostprocessor(int input_width = 640, int input_height = 640,
                              float score_threshold = 0.3f,
                              float nms_threshold = 0.45f,
                              bool is_ort_configured = false,
                              int num_classes = 80,
                              const std::vector<std::string>& class_names = {})
        : impl_(input_width, input_height, score_threshold, nms_threshold,
                is_ort_configured, num_classes, class_names) {}

    std::vector<DetectionResult> process(const dxrt::TensorPtrs& outputs,
                                         const PreprocessContext& ctx) override {
        auto legacy_results = impl_.postprocess(outputs);
        std::vector<DetectionResult> results = convertAll(legacy_results);
        detail::scaleDetectionResults(results, ctx);
        return results;
    }

    std::string getModelName() const override { return "YOLO"; }

private:
    LegacyPostProcess impl_;
};

class YOLOv26Postprocessor : public YOLOv8FamilyPostprocessor<YOLOv26PostProcess> {
public:
    using YOLOv8FamilyPostprocessor::YOLOv8FamilyPostprocessor;
    std::string getModelName() const override { return "YOLOv26"; }
};

}  // namespace dxapp

#endif  // YOLO_DETECTION_POSTPROCESSOR_HPP
