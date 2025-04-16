#!/bin/bash
# 用于构建 MMLogger 性能评估框架的脚本

set -e  # 遇到错误立即退出

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # 无颜色

# 默认参数
BUILD_TYPE="Release"
ENABLE_BENCHMARKS=ON
CLEAN_BUILD=0
RUN_AFTER_BUILD=0
BENCHMARK_ARGS=""

# 显示使用帮助
function show_help {
    echo "用法: $0 [选项]"
    echo "构建 MMLogger 性能评估框架"
    echo
    echo "选项:"
    echo "  -h, --help                显示此帮助信息"
    echo "  -d, --debug               使用 Debug 构建类型 (默认: Release)"
    echo "  -c, --clean               清理构建目录并重新构建"
    echo "  -r, --run                 构建后立即运行基准测试"
    echo "  -s, --suite=SUITE_NAME    指定要运行的测试套件（与 -r 一起使用）"
    echo "  -t, --test=TEST_ID        指定要运行的单个测试（与 -r 一起使用）"
    echo "  -p, --repeat=COUNT        指定每个测试重复次数（与 -r 一起使用）"
    echo "  -v, --verbose             启用详细输出（与 -r 一起使用）"
    echo
}

# 处理命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=1
            shift
            ;;
        -r|--run)
            RUN_AFTER_BUILD=1
            shift
            ;;
        -s|--suite=*)
            SUITE_NAME="${1#*=}"
            BENCHMARK_ARGS="$BENCHMARK_ARGS --test-suite=$SUITE_NAME"
            shift
            ;;
        -t|--test=*)
            TEST_ID="${1#*=}"
            BENCHMARK_ARGS="$BENCHMARK_ARGS --test-id=$TEST_ID"
            shift
            ;;
        -p|--repeat=*)
            REPEAT_COUNT="${1#*=}"
            BENCHMARK_ARGS="$BENCHMARK_ARGS --repeat=$REPEAT_COUNT"
            shift
            ;;
        -v|--verbose)
            BENCHMARK_ARGS="$BENCHMARK_ARGS --verbose"
            shift
            ;;
        *)
            echo -e "${RED}错误：未知选项 $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# 显示构建信息
echo -e "${GREEN}开始构建 MMLogger 性能评估框架${NC}"
echo "构建类型: $BUILD_TYPE"

# 清理构建目录（如果需要）
if [ $CLEAN_BUILD -eq 1 ]; then
    echo -e "${YELLOW}清理构建目录...${NC}"
    rm -rf build
fi

# 创建并进入构建目录
mkdir -p build
cd build

# 运行 CMake 配置
echo -e "${GREEN}配置 CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DMM_BUILD_BENCHMARKS=ON

# 编译
echo -e "${GREEN}编译...${NC}"
cmake --build . -- -j$(nproc)

# 回到根目录
cd ..

# 如果编译成功，显示成功消息
if [ $? -eq 0 ]; then
    echo -e "${GREEN}构建成功！${NC}"
    
    # 如果指定了构建后运行，则运行基准测试
    if [ $RUN_AFTER_BUILD -eq 1 ]; then
        echo -e "${GREEN}运行基准测试...${NC}"
        python3 benchmarks/run_benchmarks.py --skip-compile $BENCHMARK_ARGS
    else
        echo -e "${YELLOW}要运行基准测试，请执行:${NC}"
        echo "python3 benchmarks/run_benchmarks.py"
        echo "或"
        echo "$0 --run [其他选项]"
    fi
else
    echo -e "${RED}构建失败！${NC}"
    exit 1
fi
