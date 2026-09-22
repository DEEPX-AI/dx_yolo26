/**
 * @file yolo26n_pose_preopt_sync.cpp
 * @brief Yolo26n_pose pre-optimized (top-k in the model) synchronous inference example
 */

#include <memory>
#include <utility>
#include "factory/yolo26n_pose_preopt_factory.hpp"
#include "common/runner/sync_pose_runner.hpp"

int main(int argc, char* argv[]) {
    auto factory = std::make_unique<dxapp::Yolo26n_pose_preoptFactory>();
    dxapp::SyncPoseRunner<dxapp::Yolo26n_pose_preoptFactory> runner(std::move(factory));
    return runner.run(argc, argv);
}
