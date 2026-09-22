/**
 * @file yolo26n_pose_preopt_factory.hpp
 * @brief Yolo26n_pose pre-optimized (top-k in the model) Abstract Factory implementation
 *
 * For pre_optimized_yolo26n-pose-1.dxnn: the model emits the 300 best rows
 * with the box, the score and the 17 decoded keypoints, see
 * common/processors/preopt_topk_postprocessor.hpp.
 */

#ifndef YOLO26N_POSE_PREOPT_FACTORY_HPP
#define YOLO26N_POSE_PREOPT_FACTORY_HPP

#include <memory>
#include <string>
#include "common/base/i_factory.hpp"
#include "common/processors/letterbox_preprocessor.hpp"
#include "common/processors/preopt_topk_postprocessor.hpp"
#include "common/visualizers/pose_visualizer.hpp"
#include "common/config/model_config.hpp"

namespace dxapp {

class Yolo26n_pose_preoptFactory : public IPoseFactory {
public:
    Yolo26n_pose_preoptFactory(float score_threshold = 0.3f,
                               float nms_threshold = 0.45f)
        : score_threshold_(score_threshold),
          nms_threshold_(nms_threshold) {}

    PreprocessorPtr createPreprocessor(int input_width, int input_height) override {
        return std::make_unique<DetectionPreprocessor>(input_width, input_height);
    }

    PostprocessorPtr<PoseResult> createPostprocessor(
        int input_width, int input_height, bool is_ort_configured = false) override {
        (void)is_ort_configured;
        return std::make_unique<PreoptPosePostprocessor>(
            input_width, input_height,
            score_threshold_, nms_threshold_,
            top_k_, num_keypoints_
        );
    }

    VisualizerPtr<PoseResult> createVisualizer() override {
        return std::make_unique<PoseVisualizer>();
    }

    void loadConfig(const dxapp::ModelConfig& config) override {
        score_threshold_ = config.get<float>("score_threshold", score_threshold_);
        nms_threshold_ = config.get<float>("nms_threshold", nms_threshold_);
        top_k_ = config.get<int>("top_k", top_k_);
    }

    std::string getModelName() const override { return "Yolo26n_pose_preopt"; }
    std::string getTaskType() const override { return "pose_estimation"; }
    std::string getDefaultModel() const override {
        return "assets/models/pre_optimized_yolo26n-pose-1.dxnn";
    }

private:
    float score_threshold_;
    float nms_threshold_;
    int top_k_{300};   // used when the top-k has to be done here (no ORT)
    int num_keypoints_{17};
};

}  // namespace dxapp

#endif  // YOLO26N_POSE_PREOPT_FACTORY_HPP
