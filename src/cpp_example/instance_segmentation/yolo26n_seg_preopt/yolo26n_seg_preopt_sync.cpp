/**
 * @file yolo26n_seg_preopt_sync.cpp
 * @brief Yolo26n_seg pre-optimized (top-k in the model) synchronous instance segmentation example
 */

#include <memory>
#include <utility>
#include "factory/yolo26n_seg_preopt_factory.hpp"
#include "common/runner/sync_segmentation_runner.hpp"

int main(int argc, char* argv[]) {
    auto factory = std::make_unique<dxapp::Yolo26n_seg_preoptFactory>();
    dxapp::SyncInstanceSegRunner<dxapp::Yolo26n_seg_preoptFactory> runner(std::move(factory));
    return runner.run(argc, argv);
}
