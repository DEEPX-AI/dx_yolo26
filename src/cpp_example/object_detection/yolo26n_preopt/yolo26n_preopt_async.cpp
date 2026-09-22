/**
 * @file yolo26n_preopt_async.cpp
 * @brief Yolo26n pre-optimized (top-k in the model) asynchronous inference example
 */

#include <memory>
#include <utility>
#include "factory/yolo26n_preopt_factory.hpp"
#include "common/runner/async_detection_runner.hpp"

int main(int argc, char* argv[]) {
    auto factory = std::make_unique<dxapp::Yolo26n_preoptFactory>();
    dxapp::AsyncDetectionRunner<dxapp::Yolo26n_preoptFactory> runner(std::move(factory));
    return runner.run(argc, argv);
}
