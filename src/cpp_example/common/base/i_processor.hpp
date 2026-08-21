/**
 * @file i_processor.hpp
 * @brief Abstract interfaces for pre-processor and post-processor
 * 
 * These interfaces define the contract for all preprocessing and postprocessing operations.
 */

#ifndef DXAPP_I_PROCESSOR_HPP
#define DXAPP_I_PROCESSOR_HPP

#include <dxrt/dxrt_api.h>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <utility>

namespace dxapp {

/**
 * @brief Preprocessing context containing metadata needed for postprocessing
 * 
 * This structure holds information about the preprocessing transformations
 * that need to be reversed during postprocessing (e.g., letterbox padding).
 */
struct PreprocessContext {
    int pad_x{0};
    int pad_y{0};
    float scale{1.0f};
    float scale_x{0.0f};
    float scale_y{0.0f};

    int original_width{0};
    int original_height{0};

    int input_width{0};
    int input_height{0};

    cv::Mat source_image;

    PreprocessContext() = default;
    ~PreprocessContext() = default;
    PreprocessContext(const PreprocessContext&) = default;
    PreprocessContext& operator=(const PreprocessContext&) = default;
    PreprocessContext(PreprocessContext&&) noexcept = default;
    PreprocessContext& operator=(PreprocessContext&&) noexcept = default;

    PreprocessContext(int px, int py, float s, int ow, int oh, int iw, int ih)
        : pad_x(px), pad_y(py), scale(s), 
          original_width(ow), original_height(oh),
          input_width(iw), input_height(ih) {}
};

/**
 * @brief Abstract interface for preprocessors
 * 
 * Preprocessors transform input images into the format expected by the model.
 */
class IPreprocessor {
public:
    virtual ~IPreprocessor() = default;

    /**
     * @brief Preprocess an input image for model inference
     * @param input Original input image (BGR format)
     * @param output Preprocessed image ready for inference
     * @param ctx Output preprocessing context for postprocessing
     */
    virtual void process(const cv::Mat& input, cv::Mat& output, PreprocessContext& ctx) = 0;

    /**
     * @brief Get the expected input width for the model
     * @return Model input width
     */
    virtual int getInputWidth() const = 0;

    /**
     * @brief Get the expected input height for the model
     * @return Model input height
     */
    virtual int getInputHeight() const = 0;

    /**
     * @brief Get the expected color space conversion code
     * @return OpenCV color conversion code (e.g., cv::COLOR_BGR2RGB)
     */
    virtual int getColorConversion() const = 0;
};

/**
 * @brief Base detection result structure
 * 
 * This is a common base for all detection-based results.
 * Specific models may extend this with additional fields.
 */
struct DetectionResult {
    std::vector<float> box;  // x1, y1, x2, y2
    float confidence{0.0f};
    int class_id{0};
    std::string class_name;

    DetectionResult() = default;
    
    DetectionResult(std::vector<float> b, float conf, int cls_id, const std::string& cls_name)
        : box(std::move(b)), confidence(conf), class_id(cls_id), class_name(cls_name) {}

    float area() const {
        if (box.size() < 4) return 0.0f;
        return (box[2] - box[0]) * (box[3] - box[1]);
    }

    float iou(const DetectionResult& other) const {
        if (box.size() < 4 || other.box.size() < 4) return 0.0f;
        
        float x_left = std::max(box[0], other.box[0]);
        float y_top = std::max(box[1], other.box[1]);
        float x_right = std::min(box[2], other.box[2]);
        float y_bottom = std::min(box[3], other.box[3]);

        if (x_right < x_left || y_bottom < y_top) return 0.0f;

        float intersection = (x_right - x_left) * (y_bottom - y_top);
        float union_area = area() + other.area() - intersection;
        
        return union_area > 0 ? intersection / union_area : 0.0f;
    }
};

/**
 * @brief Classification result structure
 */
struct ClassificationResult {
    int class_id{0};
    std::string class_name;
    float confidence{0.0f};
    std::vector<std::pair<int, float>> top_k;  // top-k predictions

    ClassificationResult() = default;
};

/**
 * @brief Keypoint structure for pose/face landmarks
 */
struct Keypoint {
    float x{0.0f};
    float y{0.0f};
    float confidence{0.0f};

    Keypoint() = default;
    Keypoint(float x_, float y_, float conf = 1.0f) : x(x_), y(y_), confidence(conf) {}
};

/**
 * @brief Pose estimation result with keypoints
 */
struct PoseResult {
    std::vector<float> box;  // x1, y1, x2, y2 (optional bounding box)
    float confidence{0.0f};
    std::vector<Keypoint> keypoints;  // 17 keypoints for COCO pose

    PoseResult() = default;
    
    float area() const {
        if (box.size() < 4) return 0.0f;
        return (box[2] - box[0]) * (box[3] - box[1]);
    }
};

/**
 * @brief Instance segmentation result
 */
struct InstanceSegmentationResult {
    std::vector<float> box;  // x1, y1, x2, y2
    float confidence{0.0f};
    int class_id{0};
    std::string class_name;
    cv::Mat mask;  // Instance mask
    int track_id{-1};  // stable multi-frame id assigned by a tracker (-1 = untracked)

    InstanceSegmentationResult() = default;
    ~InstanceSegmentationResult() = default;
    InstanceSegmentationResult(const InstanceSegmentationResult&) = default;
    InstanceSegmentationResult& operator=(const InstanceSegmentationResult&) = default;
    InstanceSegmentationResult(InstanceSegmentationResult&&) noexcept = default;
    InstanceSegmentationResult& operator=(InstanceSegmentationResult&&) noexcept = default;

    float area() const {
        if (box.size() < 4) return 0.0f;
        return (box[2] - box[0]) * (box[3] - box[1]);
    }
};

/**
 * @brief Depth estimation result
 */
struct DepthResult {
    cv::Mat depth_map;    // Float depth map (H x W)
    int width{0};
    int height{0};
    float min_depth{0.0f};
    float max_depth{0.0f};

    DepthResult() = default;
};

/**
 * @brief Abstract interface for postprocessors
 *
 * Postprocessors transform model outputs into usable detection/segmentation results.
 * This is a template interface to support different result types.
 */
template <typename ResultType>
class IPostprocessor {
public:
    virtual ~IPostprocessor() = default;

    /**
     * @brief Process model outputs into results
     * @param outputs Model output tensors
     * @param ctx Preprocessing context for coordinate transformation
     * @return Vector of processed results
     */
    virtual std::vector<ResultType> process(const dxrt::TensorPtrs& outputs, 
                                            const PreprocessContext& ctx) = 0;

    /**
     * @brief Get the model name this postprocessor is designed for
     * @return Model name string
     */
    virtual std::string getModelName() const = 0;
};

// Type aliases for common postprocessor types
using DetectionPostprocessor = IPostprocessor<DetectionResult>;
using ClassificationPostprocessor = IPostprocessor<ClassificationResult>;
using PosePostprocessor = IPostprocessor<PoseResult>;
using InstanceSegmentationPostprocessor = IPostprocessor<InstanceSegmentationResult>;
using DepthPostprocessor = IPostprocessor<DepthResult>;

// Smart pointer aliases
using PreprocessorPtr = std::unique_ptr<IPreprocessor>;
template <typename T>
using PostprocessorPtr = std::unique_ptr<IPostprocessor<T>>;

}  // namespace dxapp

#endif  // DXAPP_I_PROCESSOR_HPP
