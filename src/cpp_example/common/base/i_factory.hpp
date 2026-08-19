/**
 * @file i_factory.hpp
 * @brief Abstract Factory interface for model component creation
 * 
 * This interface defines the Abstract Factory pattern for creating
 * matching sets of preprocessor, postprocessor, and visualizer components.
 */

#ifndef DXAPP_I_FACTORY_HPP
#define DXAPP_I_FACTORY_HPP

#include <array>
#include <memory>
#include <string>

#include "i_processor.hpp"
#include "i_visualizer.hpp"

namespace dxapp {

// Forward declaration for config loading
class ModelConfig;

/**
 * @brief Input normalization parameters for float-input models.
 *
 * When @c apply_mean_std is true, the runner feeds the model
 * (pixel/255 - mean) / std per channel. mean/std are in the preprocessor's
 * output channel order (RGB when BGR->RGB conversion is applied).
 * When false, float-input models receive plain /255 normalization and
 * uint8-input models are unaffected.
 */
struct InputNormalizationParams {
    bool apply_mean_std = false;
    std::array<float, 3> mean{0.f, 0.f, 0.f};
    std::array<float, 3> std{1.f, 1.f, 1.f};
};

/**
 * @brief Abstract Factory interface for object detection models
 * 
 * Creates matching sets of components for object detection models.
 * Each concrete factory (e.g., YOLOv5Factory) creates components
 * that are guaranteed to work together correctly.
 */
class IDetectionFactory {
public:
    virtual ~IDetectionFactory() = default;

    /**
     * @brief Create a preprocessor for this model
     * @param input_width Model input width
     * @param input_height Model input height
     * @return Unique pointer to preprocessor
     */
    virtual PreprocessorPtr createPreprocessor(int input_width, int input_height) = 0;

    /**
     * @brief Create a postprocessor for this model
     * @param input_width Model input width
     * @param input_height Model input height
     * @param is_ort_configured Whether ORT inference is configured
     * @return Unique pointer to detection postprocessor
     */
    virtual PostprocessorPtr<DetectionResult> createPostprocessor(
        int input_width, int input_height, bool is_ort_configured = false) = 0;

    /**
     * @brief Create a visualizer for this model
     * @return Unique pointer to detection visualizer
     */
    virtual VisualizerPtr<DetectionResult> createVisualizer() = 0;

    /**
     * @brief Get the model name this factory is for
     * @return Model name string (e.g., "YOLOv5", "YOLOv8")
     */
    virtual std::string getModelName() const = 0;

    /**
     * @brief Get the task type this factory is for
     * @return Task type string (e.g., "object_detection")
     */
    virtual std::string getTaskType() const = 0;

    // Model this application runs when -m is omitted, relative to the working
    // directory (e.g. "assets/models/yolo26-n-od_640x640.dxnn").
    virtual std::string getDefaultModel() const = 0;

    /**
     * @brief Load configuration from an external JSON file
     * @param config Parsed ModelConfig instance
     *
     * Override in concrete factories to apply runtime parameters.
     * Default implementation is a no-op (uses constructor defaults).
     */
    virtual void loadConfig(const ModelConfig& /*config*/) { /* No-op: subclasses override to apply runtime parameters */ }
};

/**
 * @brief Abstract Factory interface for classification models
 */
class IClassificationFactory {
public:
    virtual ~IClassificationFactory() = default;

    virtual PreprocessorPtr createPreprocessor(int input_width, int input_height) = 0;
    
    virtual PostprocessorPtr<ClassificationResult> createPostprocessor(
        int input_width, int input_height) = 0;
    
    virtual VisualizerPtr<ClassificationResult> createVisualizer() = 0;

    virtual std::string getModelName() const = 0;
    virtual std::string getTaskType() const = 0;

    // Model this application runs when -m is omitted, relative to the working
    // directory (e.g. "assets/models/yolo26-n-od_640x640.dxnn").
    virtual std::string getDefaultModel() const = 0;

    virtual void loadConfig(const ModelConfig& /*config*/) { /* No-op: subclasses override to apply runtime parameters */ }
};

/**
 * @brief Abstract Factory interface for pose estimation models
 */
class IPoseFactory {
public:
    virtual ~IPoseFactory() = default;

    virtual PreprocessorPtr createPreprocessor(int input_width, int input_height) = 0;
    
    virtual PostprocessorPtr<PoseResult> createPostprocessor(
        int input_width, int input_height, bool is_ort_configured = false) = 0;
    
    virtual VisualizerPtr<PoseResult> createVisualizer() = 0;

    virtual std::string getModelName() const = 0;
    virtual std::string getTaskType() const = 0;

    // Model this application runs when -m is omitted, relative to the working
    // directory (e.g. "assets/models/yolo26-n-od_640x640.dxnn").
    virtual std::string getDefaultModel() const = 0;

    virtual void loadConfig(const ModelConfig& /*config*/) { /* No-op: subclasses override to apply runtime parameters */ }
};

/**
 * @brief Abstract Factory interface for instance segmentation models
 */
class IInstanceSegmentationFactory {
public:
    virtual ~IInstanceSegmentationFactory() = default;

    virtual PreprocessorPtr createPreprocessor(int input_width, int input_height) = 0;
    
    virtual PostprocessorPtr<InstanceSegmentationResult> createPostprocessor(
        int input_width, int input_height, bool is_ort_configured = false) = 0;
    
    virtual VisualizerPtr<InstanceSegmentationResult> createVisualizer() = 0;

    virtual std::string getModelName() const = 0;
    virtual std::string getTaskType() const = 0;

    // Model this application runs when -m is omitted, relative to the working
    // directory (e.g. "assets/models/yolo26-n-od_640x640.dxnn").
    virtual std::string getDefaultModel() const = 0;

    virtual void loadConfig(const ModelConfig& /*config*/) { /* No-op: subclasses override to apply runtime parameters */ }
};

/**
 * @brief Abstract Factory interface for depth estimation models
 */
class IDepthEstimationFactory {
public:
    virtual ~IDepthEstimationFactory() = default;

    virtual PreprocessorPtr createPreprocessor(int input_width, int input_height) = 0;
    
    virtual PostprocessorPtr<DepthResult> createPostprocessor(
        int input_width, int input_height) = 0;
    
    virtual VisualizerPtr<DepthResult> createVisualizer() = 0;

    virtual std::string getModelName() const = 0;
    virtual std::string getTaskType() const = 0;

    // Model this application runs when -m is omitted, relative to the working
    // directory (e.g. "assets/models/yolo26-n-od_640x640.dxnn").
    virtual std::string getDefaultModel() const = 0;

    virtual void loadConfig(const ModelConfig& /*config*/) { /* No-op: subclasses override to apply runtime parameters */ }

    /**
     * @brief Input normalization for float-input depth models.
     *
     * Default: no mean/std (runner uses plain /255 for float inputs, or raw
     * uint8 for uint8 inputs). Override for models such as Depth Anything V2
     * that require ImageNet mean/std normalization.
     */
    virtual InputNormalizationParams getInputNormalization() const { return {}; }
};

// Smart pointer aliases for factories
using DetectionFactoryPtr = std::unique_ptr<IDetectionFactory>;
using ClassificationFactoryPtr = std::unique_ptr<IClassificationFactory>;
using PoseFactoryPtr = std::unique_ptr<IPoseFactory>;
using InstanceSegmentationFactoryPtr = std::unique_ptr<IInstanceSegmentationFactory>;
using DepthEstimationFactoryPtr = std::unique_ptr<IDepthEstimationFactory>;

}  // namespace dxapp

#endif  // DXAPP_I_FACTORY_HPP
