# dx_yolo26

YOLO26 application package for the DEEPX M1 NPU. Five C++ applications built on top
of DXRT (DeepX Runtime). The YOLO26 postprocessing is compiled into the
applications - no shared library is produced.

| Application | Task | Source directory | Default model |
| :--- | :--- | :--- | :--- |
| `yolo26n` | Object detection | `src/cpp_example/object_detection/yolo26n` | `yolo26-n-od_640x640.dxnn` |
| `yolo26n_cls` | Classification | `src/cpp_example/classification/yolo26n_cls` | `yolo26-n-cls_224x224.dxnn` |
| `yolo26n_pose` | Pose estimation | `src/cpp_example/pose_estimation/yolo26n_pose` | `yolo26-n-pose_640x640.dxnn` |
| `yolo26n_seg` | Instance segmentation | `src/cpp_example/instance_segmentation/yolo26n_seg` | `yolo26-n-seg_640x640.dxnn` |
| `yolo26n_depth` | Depth estimation | `src/cpp_example/depth_estimation/yolo26n_depth` | `yolo26-depth-n_768x768.dxnn` |

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

## Install layout

```
<prefix>/bin/yolo26n_async, yolo26n_cls_async, ...
<prefix>/share/dx_yolo26/examples/<category>/<model>/config.json
<prefix>/share/dx_yolo26/sample/img/*.jpg, *.png
```

## Yocto

`meta-deepx-m1` packages this repository as `dx-yolo26`:

```
IMAGE_INSTALL:append = " dx-yolo26 dx-yolo26-samples"
```

`dx-yolo26` holds the applications with their postprocess parameters,
`dx-yolo26-samples` the sample images. The recipe builds the async
variants only and passes `-DDXYOLO26_DATA_DIR=${datadir}/dx_yolo26`.

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
