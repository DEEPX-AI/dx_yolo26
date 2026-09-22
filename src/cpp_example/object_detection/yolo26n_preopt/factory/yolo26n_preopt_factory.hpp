/**
 * @file yolo26n_preopt_factory.hpp
 * @brief Yolo26n pre-optimized (top-k in the model) Abstract Factory implementation
 *
 * For pre_optimized_yolo26-n-od.dxnn: the model emits the 300 best (anchor, class)
 * rows already decoded, see common/processors/preopt_topk_postprocessor.hpp.
 */

#ifndef YOLO26N_PREOPT_FACTORY_HPP
#define YOLO26N_PREOPT_FACTORY_HPP

#include <memory>
#include <string>
#include <vector>
#include "common/base/i_factory.hpp"
#include "common/processors/letterbox_preprocessor.hpp"
#include "common/processors/preopt_topk_postprocessor.hpp"
#include "common/visualizers/detection_visualizer.hpp"
#include "common/config/model_config.hpp"

namespace dxapp {

class Yolo26n_preoptFactory : public IDetectionFactory {
public:
    Yolo26n_preoptFactory(float score_threshold = 0.3f,
                          float nms_threshold = 0.45f)
        : score_threshold_(score_threshold),
          nms_threshold_(nms_threshold) {}

    PreprocessorPtr createPreprocessor(int input_width, int input_height) override {
        return std::make_unique<DetectionPreprocessor>(input_width, input_height);
    }

    // The postprocessor recognises both the ORT output (preopt_output) and the
    // raw NPU tensors by shape, so is_ort_configured is not needed.
    PostprocessorPtr<DetectionResult> createPostprocessor(
        int input_width, int input_height, bool is_ort_configured = false) override {
        (void)is_ort_configured;
        return std::make_unique<PreoptDetectionPostprocessor>(
            input_width, input_height,
            score_threshold_, nms_threshold_,
            top_k_, class_names_
        );
    }

    VisualizerPtr<DetectionResult> createVisualizer() override {
        return std::make_unique<DetectionVisualizer>();
    }

    void loadConfig(const dxapp::ModelConfig& config) override {
        score_threshold_ = config.get<float>("score_threshold", score_threshold_);
        nms_threshold_ = config.get<float>("nms_threshold", nms_threshold_);
        top_k_ = config.get<int>("top_k", top_k_);
        class_names_ = config.get_string_list("class_names");
    }

    std::string getModelName() const override { return "Yolo26n_preopt"; }
    std::string getTaskType() const override { return "object_detection"; }
    std::string getDefaultModel() const override {
        return "assets/models/pre_optimized_yolo26-n-od.dxnn";
    }

private:
    float score_threshold_;
    float nms_threshold_;
    int top_k_{300};   // used when the top-k has to be done here (no ORT)
    std::vector<std::string> class_names_;
};

}  // namespace dxapp

#endif  // YOLO26N_PREOPT_FACTORY_HPP
