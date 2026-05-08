#!/bin/bash
# build.sh - 构建 Linux 64bit 和/或 Windows 64bit 二进制文件，并运行对应平台的单元测试
#
# 用法:
#   ./build.sh           # 同时构建 lin64 + win64
#   ./build.sh lin64     # 仅构建 Linux 64bit
#   ./build.sh win64     # 仅构建 Windows 64bit
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
TOOLCHAIN="${PROJECT_ROOT}/toolchain-mingw64.cmake"

# ======================== 解析参数 ========================
ARCH=""
if [ $# -ge 1 ]; then
    case "$1" in
        lin64|win64) ARCH="$1" ;;
        *)
            echo "用法: $0 [lin64|win64]"
            echo "  不带参数：同时构建两个平台"
            echo "  lin64   ：仅构建 Linux 64bit"
            echo "  win64   ：仅构建 Windows 64bit"
            exit 1
            ;;
    esac
fi

# 确定要构建的平台列表
if [ -n "$ARCH" ]; then
    PLATFORMS=("$ARCH")
else
    PLATFORMS=(lin64 win64)
fi

# ======================== 清理旧构建 ========================
rm -rf "${PROJECT_ROOT}/build"

# ======================== 逐平台构建与测试 ========================
for PLAT in "${PLATFORMS[@]}"; do
    echo ""
    echo "=========================================="
    echo "  构建 ${PLAT}"
    echo "=========================================="

    BUILD_DIR="${PROJECT_ROOT}/build/${PLAT}"
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    if [ "$PLAT" = "win64" ]; then
        cmake "${PROJECT_ROOT}" \
            -Darch=win64 \
            -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
            -DCMAKE_BUILD_TYPE=Release
    else
        cmake "${PROJECT_ROOT}" \
            -Darch=lin64 \
            -DCMAKE_BUILD_TYPE=Release
    fi

    cmake --build . --parallel

    echo ""
    echo "----- 运行 ${PLAT} 单元测试 -----"
    ctest --output-on-failure

    echo ""
    echo "${PLAT} 构建和测试完成。"
done

# ======================== 汇总 ========================
echo ""
echo "=========================================="
echo "  构建汇总"
echo "=========================================="
for PLAT in "${PLATFORMS[@]}"; do
    BUILD_DIR="${PROJECT_ROOT}/build/${PLAT}"
    if [ "$PLAT" = "win64" ]; then
        echo "Windows 64bit: ${BUILD_DIR}/json_cmd_win64.exe"
    else
        echo "Linux  64bit : ${BUILD_DIR}/json_cmd_lin64"
    fi
done
echo "所有请求平台的构建和测试均已完成。"
