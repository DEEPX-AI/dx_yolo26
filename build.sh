#!/bin/bash
# Build the YOLO26 applications.
#
#   ./build.sh                                  # native build, sync + async
#   ./build.sh --variants async                 # async applications only
#   ./build.sh --arch aarch64                   # cross build (cmake/toolchain.aarch64.cmake)
#   ./build.sh --install-prefix /usr --install   # build and install
set -e

DX_YOLO26_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

arch="$(uname -m)"
build_type="Release"
variants="both"
install_prefix=""
dxrt_dir="/usr/local"
do_install="false"
clean="false"
jobs="$(nproc)"

usage() {
    cat <<EOF
Usage: $0 [options]

  --arch <name>            Target architecture (default: $(uname -m)). A value other than the
                           host architecture selects cmake/toolchain.<name>.cmake.
  --variants <v>           both | sync | async   (default: both)
  --type <t>               Release | Debug | RelWithDebInfo  (default: Release)
  --dxrt-dir <path>        DXRT install prefix (default: /usr/local)
  --install-prefix <path>  CMAKE_INSTALL_PREFIX (default: <build dir>/release)
  --install                Run the install step after building
  --clean                  Remove the build directory first
  -j <n>                   Parallel jobs (default: $(nproc))
  -h, --help               Show this help
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --arch)           arch="$2"; shift 2 ;;
        --variants)       variants="$2"; shift 2 ;;
        --type)           build_type="$2"; shift 2 ;;
        --dxrt-dir)       dxrt_dir="$2"; shift 2 ;;
        --install-prefix) install_prefix="$2"; shift 2 ;;
        --install)        do_install="true"; shift ;;
        --clean)          clean="true"; shift ;;
        -j)               jobs="$2"; shift 2 ;;
        -h|--help)        usage; exit 0 ;;
        *) echo "[DX_YOLO26] [ERROR] Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

[ "${arch}" = "arm64" ] && arch="aarch64"

build_dir="${DX_YOLO26_PATH}/build_${arch}"
[ "${clean}" = "true" ] && rm -rf "${build_dir}"
[ -z "${install_prefix}" ] && install_prefix="${build_dir}/release"

cmake_args=(
    -S "${DX_YOLO26_PATH}"
    -B "${build_dir}"
    -DCMAKE_BUILD_TYPE="${build_type}"
    -DCMAKE_INSTALL_PREFIX="${install_prefix}"
    -DDXRT_INSTALLED_DIR="${dxrt_dir}"
    -DDXYOLO26_VARIANTS="${variants}"
)

if [ "${arch}" != "$(uname -m)" ]; then
    toolchain="${DX_YOLO26_PATH}/cmake/toolchain.${arch}.cmake"
    if [ ! -f "${toolchain}" ]; then
        echo "[DX_YOLO26] [ERROR] No toolchain file for '${arch}': ${toolchain}" >&2
        exit 1
    fi
    cmake_args+=(-DCMAKE_TOOLCHAIN_FILE="${toolchain}")
fi

command -v ninja >/dev/null 2>&1 && cmake_args+=(-G Ninja)

cmake "${cmake_args[@]}"
cmake --build "${build_dir}" --parallel "${jobs}"

if [ "${do_install}" = "true" ]; then
    cmake --install "${build_dir}"
    echo "[DX_YOLO26] [INFO] Installed into ${install_prefix}"
else
    echo "[DX_YOLO26] [INFO] Binaries are in ${build_dir}/src/cpp_example"
fi
