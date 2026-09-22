# dx_yolo26

YOLO26 application package for the DEEPX M1 NPU. Eight C++ applications built on top
of DXRT (DeepX Runtime): five for the regular models and three for the pre-optimized
models (see [Pre-optimized models](#pre-optimized-models)). The YOLO26 postprocessing is compiled into the
applications - no shared library is produced.

| Application | Task | Source directory | Default model |
| :--- | :--- | :--- | :--- |
| `yolo26n` | Object detection | `src/cpp_example/object_detection/yolo26n` | `yolo26-n-od_640x640.dxnn` |
| `yolo26n_cls` | Classification | `src/cpp_example/classification/yolo26n_cls` | `yolo26-n-cls_224x224.dxnn` |
| `yolo26n_pose` | Pose estimation | `src/cpp_example/pose_estimation/yolo26n_pose` | `yolo26-n-pose_640x640.dxnn` |
| `yolo26n_seg` | Instance segmentation | `src/cpp_example/instance_segmentation/yolo26n_seg` | `yolo26-n-seg_640x640.dxnn` |
| `yolo26n_depth` | Depth estimation | `src/cpp_example/depth_estimation/yolo26n_depth` | `yolo26-depth-n_768x768.dxnn` |
| `yolo26n_preopt` | Object detection, pre-optimized model | `src/cpp_example/object_detection/yolo26n_preopt` | `pre_optimized_yolo26-n-od.dxnn` |
| `yolo26n_pose_preopt` | Pose estimation, pre-optimized model | `src/cpp_example/pose_estimation/yolo26n_pose_preopt` | `pre_optimized_yolo26n-pose.dxnn` |
| `yolo26n_seg_preopt` | Instance segmentation, pre-optimized model | `src/cpp_example/instance_segmentation/yolo26n_seg_preopt` | `pre_optimized_yolo26n-seg.dxnn` |

Each application is built in two runner variants: `<name>_sync` and `<name>_async`.

## Requirements

* DXRT (DeepX Runtime) - `libdxrt` and the `dxrt/` headers, `/usr/local` by default
* OpenCV 4.2 or newer (4.5.5 recommended)
* CMake 3.14+, a C++17 compiler, Ninja or Make

## Build

```bash
./build.sh                      # native build, sync + async
./build.sh --variants async     # async applications only
./build.sh --clean -j 8         # rebuild from scratch
./build.sh --arch aarch64       # cross build, see cmake/toolchain.aarch64.cmake
./build.sh --help
```

Binaries land in `build_<arch>/src/cpp_example/`. `./build.sh --install-prefix /usr --install`
installs them.

Direct CMake use, with the options this project adds:

```bash
cmake -S . -B build -G Ninja \
    -DDXRT_INSTALLED_DIR=/usr/local \
    -DDXYOLO26_VARIANTS=async \
    -DDXYOLO26_DATA_DIR=/usr/share/dx_yolo26
cmake --build build -j
```

| Option | Default | Meaning |
| :--- | :--- | :--- |
| `DXRT_INSTALLED_DIR` | `/usr/local` | DXRT install prefix (`lib/libdxrt.so`, `include/dxrt/`) |
| `DXYOLO26_VARIANTS` | `both` | Applications to build: `both`, `sync` or `async` |
| `DXYOLO26_DATA_DIR` | source tree | Installed data directory the applications point at in their error hints |
| `DXYOLO26_INSTALL_SAMPLES` | `ON` | Install `sample/img/` (about 1 MB) |

## Run

```bash
# explicit model, video input
yolo26n_async -m yolo26-n-od_640x640.dxnn -v input.mp4

# image directory, no display, postprocess thresholds from a config file
yolo26n_seg_sync -m yolo26-n-seg_640x640.dxnn -i ./images --no-display \
    --config /usr/share/dx_yolo26/examples/instance_segmentation/yolo26n_seg/config.json

yolo26n_async --help
```

With no `-i/-v/-c/-r` an application falls back to a bundled sample image
(`sample/img/...`, resolved relative to the working directory), so on a target this
works:

```bash
cd /usr/share/dx_yolo26
yolo26n_async -m /path/to/yolo26-n-od_640x640.dxnn
```

`-m` may be omitted: every application declares its own default model (see
`getDefaultModel()` in its factory) and looks for it under `assets/models/` relative
to the working directory. The `.dxnn` models are not part of this repository - get
them from [DX-ModelZoo](https://developer.deepx.ai/modelzoo/).

## Pre-optimized models

A *pre-optimized* `.dxnn` (`pre_optimized_yolo26-n-od.dxnn`, `pre_optimized_yolo26n-pose.dxnn`,
`pre_optimized_yolo26n-seg.dxnn`)
carries the tail of the YOLO26 end-to-end head inside the model. The NPU already
applies the DFL integration, and a CPU task - run by DXRT through ONNX Runtime -
applies sigmoid to the class logits, keeps the **top-k = 300** anchors by their best
class score, keeps the top-k (anchor, class) pairs of those, decodes the boxes and
keypoints and gathers the mask coefficients. The application receives a fixed-size
table sorted by score, in model-input (letterboxed) pixel coordinates:

| Model | Output | Row layout |
| :--- | :--- | :--- |
| `pre_optimized_yolo26-n-od.dxnn` | `preopt_output [1, 300, 6]` | `x1 y1 x2 y2 score class_id` |
| `pre_optimized_yolo26n-pose.dxnn` | `preopt_output [1, 300, 57]` | `x1 y1 x2 y2 score class_id` + 17 x `kx ky visibility` |
| `pre_optimized_yolo26n-seg.dxnn` | `preopt_output [1, 300, 38]`, `output1 [1, 32, 160, 160]` | `x1 y1 x2 y2 score class_id` + 32 mask coefficients; `output1` holds the mask prototypes |

The `*_preopt` applications therefore only filter the rows by `score_threshold`, map
the coordinates back to the source image and, for segmentation, multiply the mask
coefficients with the prototypes over the box region. All three share
`src/cpp_example/common/processors/preopt_topk_postprocessor.hpp`.

Two details of this scheme are worth knowing:

* The second top-k runs over the flattened (anchor x class) scores, as in
  Ultralytics' end-to-end postprocess, so one anchor can appear several times with
  different classes (the same box as `car` and `truck`). The model is NMS-free; the
  class-agnostic NMS controlled by `nms_threshold` in the `config.json` only merges
  these duplicates. Set `nms_threshold` to `1.0` to keep every row.
* When DXRT is built without ONNX Runtime the CPU task is skipped and the
  application receives the NPU tensors instead (`preopt_bbox_tr_i [1, H, W, 4]`,
  `preopt_cls_tr_i [1, H, W, nc]`, `preopt_kpt_tr_i [1, H, W, 51]`,
  `preopt_mask_tr_i [1, H, W, 32]` for strides 8/16/32). The postprocessor then does
  the same top-k selection itself (`top_k` in the `config.json`, 300 by default) and
  produces the identical rows, so the applications behave the same either way.

```bash
yolo26n_preopt_sync      -m assets/models/pre_optimized_yolo26-n-od.dxnn
yolo26n_pose_preopt_sync -m assets/models/pre_optimized_yolo26n-pose.dxnn -i sample/img/sample_people.jpg
yolo26n_seg_preopt_async -m assets/models/pre_optimized_yolo26n-seg.dxnn -v input.mp4 \
    --config src/cpp_example/instance_segmentation/yolo26n_seg_preopt/config.json
```

## Install layout

```
<prefix>/bin/yolo26n_async, yolo26n_cls_async, ...
<prefix>/share/dx_yolo26/examples/<category>/<model>/config.json
<prefix>/share/dx_yolo26/sample/img/*.jpg, *.png
```

## Yocto

`meta-deepx-m1` packages this repository as `dx-yolo26`:

```
IMAGE_INSTALL:append = " dx-yolo26 dx-yolo26-sample"
```

`dx-yolo26` holds the applications with their postprocess parameters;
`dx-yolo26-sample` is a separate recipe carrying the asset bundle (models, video
clips and sample images) under `/etc/dx-yolo26-sample`, which is where an
application runs with no arguments. The recipe builds the async variants only,
passes `-DDXYOLO26_DATA_DIR=${datadir}/dx_yolo26`, and leaves the in-tree sample
images out (`-DDXYOLO26_INSTALL_SAMPLES=OFF`) so they are not shipped twice.

## Adding another YOLO26 variant

CMake discovers applications from the directory layout: drop
`src/cpp_example/<category>/<model>/{<model>_sync.cpp,<model>_async.cpp,factory/<model>_factory.hpp}`
in place - the factory declares its own default model through `getDefaultModel()` -
and it is built with no build-file change.

## Provenance

Part of the source code originates from
[dx_app](https://github.com/DEEPX-AI/dx_app), but this package is maintained on its
own: it is not a fork and is not kept in sync with it. The shared
runner/processor/visualizer framework under `src/cpp_example/common/` carries only
what the five applications need - code for other model families and other tasks is
not part of this package.
