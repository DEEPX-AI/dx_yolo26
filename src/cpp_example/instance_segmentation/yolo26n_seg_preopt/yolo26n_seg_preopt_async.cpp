/**
 * @file yolo26n_seg_preopt_async.cpp
 * @brief Yolo26n_seg pre-optimized (top-k in the model) asynchronous instance segmentation example
 */

#include <memory>
#include <utility>
#include "factory/yolo26n_seg_preopt_factory.hpp"
#include "common/runner/async_segmentation_runner.hpp"

int main(int argc, char* argv[]) {
    auto factory = std::make_unique<dxapp::Yolo26n_seg_preoptFactory>();
    dxapp::AsyncInstanceSegRunner<dxapp::Yolo26n_seg_preoptFactory> runner(std::move(factory));
    return runner.run(argc, argv);
}
