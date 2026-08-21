/**
 * @file segmentation_postprocessor.hpp
 * @brief Unified Segmentation Postprocessors for v3 interface
 * 
 * Groups all segmentation postprocessors:
 *   - DeepLabv3 (Semantic Segmentation)
 *   - YOLOv8Seg (Instance Segmentation)
 */

#ifndef SEGMENTATION_POSTPROCESSOR_HPP
#define SEGMENTATION_POSTPROCESSOR_HPP

#include "common/base/i_processor.hpp"
#include "common/processors/result_converters.hpp"
#include "common/processors/roi_instance_mask.hpp"
#include "common_util.hpp"

#include <set>
#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

// Postprocess headers
#include "anchorless_instance_seg_postprocessor.hpp"

namespace dxapp {

// ============================================================================
// Instance Segmentation Base Template
// ============================================================================
namespace detail {

inline void scaleInstanceSegResults(std::vector<InstanceSegmentationResult>& results,
                                    const PreprocessContext& ctx) {
    for (auto& seg : results) {
        scaleBox(seg.box, ctx);
        // Crop padding from mask, then resize to original image size (matching original)
        if (seg.mask.empty() || ctx.original_width <= 0 || ctx.original_height <= 0) continue;
        cv::Mat cropped_mask = seg.mask;
        if (ctx.pad_x > 0 || ctx.pad_y > 0) {
            int unpad_w = seg.mask.cols - 2 * ctx.pad_x;
            int unpad_h = seg.mask.rows - 2 * ctx.pad_y;
            if (unpad_w > 0 && unpad_h > 0) {
                cv::Rect crop_region(ctx.pad_x, ctx.pad_y, unpad_w, unpad_h);
                cropped_mask = seg.mask(crop_region).clone();
            }
        }
        cv::resize(cropped_mask, seg.mask,
                  cv::Size(ctx.original_width, ctx.original_height));
    }
}

}  // namespace detail

// ============================================================================
// YOLOv8Seg Instance Segmentation Postprocessor
// ============================================================================
class YOLOv8SegPostprocessor : public IPostprocessor<InstanceSegmentationResult> {
public:
    YOLOv8SegPostprocessor(int input_width = 640, int input_height = 640,
                           float score_threshold = 0.45f, float nms_threshold = 0.4f,
                           bool is_ort_configured = false, int num_classes = 80,
                           const std::vector<std::string>& class_names = {})
        : impl_(input_width, input_height, score_threshold, nms_threshold,
                is_ort_configured, num_classes, class_names) {}

    std::vector<InstanceSegmentationResult> process(const dxrt::TensorPtrs& outputs,
                                                    const PreprocessContext& ctx) override {
        std::vector<InstanceSegmentationResult> results;

        // Align once, then decode detections WITHOUT materialising a full-frame
        // float mask per instance. The old path built an input-resolution float
        // mask in the impl AND resized every mask to the full original frame in
        // scaleInstanceSegResults() — the multi-GB peak-memory hotspot for
        // instance-heavy inputs (e.g. FastSAM 1024x1024).
        dxrt::TensorPtrs aligned = impl_.get_is_ort_configured()
                                       ? outputs : impl_.align_tensors(outputs);
        if (aligned.size() < 2) return results;

        auto detections = impl_.decode_detections(aligned);
        if (detections.empty()) return results;

        // Prototype masks: layout [1, C, H, W] (or [C, H, W]).
        const auto& proto = aligned[1];
        auto ps = proto->shape();
        int proto_c = static_cast<int>(ps.size() == 4 ? ps[1] : ps[0]);
        int proto_h = static_cast<int>(ps.size() == 4 ? ps[2] : ps[1]);
        int proto_w = static_cast<int>(ps.size() == 4 ? ps[3] : ps[2]);
        const float* proto_data = static_cast<const float*>(proto->data());

        const int in_w = impl_.get_input_width();
        const int in_h = impl_.get_input_height();

        results.reserve(detections.size());
        for (const auto& d : detections) {
            if (d.box.size() < 4 || d.seg_mask_coef.size() != static_cast<size_t>(proto_c))
                continue;

            // Box is in model-input (letterboxed) coordinates.
            const float x1 = d.box[0], y1 = d.box[1], x2 = d.box[2], y2 = d.box[3];

            // Box-ROI mask: sigmoid(coefs . proto) + a single aligned resize over
            // only the bbox region, directly to original resolution. Output is a
            // CV_8UC1 mask zeroed outside the bbox — matching the YOLOv5-Seg path
            // and what InstanceSegmentationVisualizer already expects.
            cv::Mat binary_mask = roiInstanceMask(
                d.seg_mask_coef, proto_data, proto_c, proto_h, proto_w,
                x1, y1, x2, y2, in_w, in_h, ctx);

            const float fx1 = std::max(0.0f, std::min((x1 - ctx.pad_x) / ctx.scale, static_cast<float>(ctx.original_width)));
            const float fy1 = std::max(0.0f, std::min((y1 - ctx.pad_y) / ctx.scale, static_cast<float>(ctx.original_height)));
            const float fx2 = std::max(0.0f, std::min((x2 - ctx.pad_x) / ctx.scale, static_cast<float>(ctx.original_width)));
            const float fy2 = std::max(0.0f, std::min((y2 - ctx.pad_y) / ctx.scale, static_cast<float>(ctx.original_height)));

            InstanceSegmentationResult seg;
            seg.box = {fx1, fy1, fx2, fy2};
            seg.confidence = d.confidence;
            seg.class_id = d.class_id;
            seg.class_name = d.class_name;
            seg.mask = binary_mask;
            results.push_back(std::move(seg));
        }
        return results;
    }

    std::string getModelName() const override { return "YOLOv8-Seg"; }

private:
    YOLOv8SegPostProcess impl_;
};

}  // namespace dxapp

#endif  // SEGMENTATION_POSTPROCESSOR_HPP
