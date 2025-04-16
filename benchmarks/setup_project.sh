能评估框架的目录结构和文件

set -e  # 遇到错误立即退出

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # 无颜色

# 打印带颜色的提示
echo -e "${GREEN}正在创建MMLogger性能评估框架...${NC}"

# 创建目录结构
mkdir -p build results logs
mkdir -p benchmarks

# 检查文件是否已存在
if [ -f "mmlogger_benchmark.cpp" ]; then
    echo -e "${YELLOW}发现已存在的mmlogger_benchmark.cpp文件，移动到benchmarks目录${NC}"
    mv mmlogger_benchmark.cpp benchmarks/
fi

# 创建必要的目录结构，如果原始MMLogger源码目录不存在
if [ ! -d "include" ] || [ ! -d "src" ]; then
    echo -e "${YELLOW}未找到MMLogger源代码目录，创建软链接...${NC}"
    
    # 询问MMLogger源码目录
    read -p "请输入MMLogger源码根目录路径(或按Enter跳过): " mmlogger_dir
    
    if [ -n "$mmlogger_dir" ]; then
        # 创建软链接
        if [ -d "$mmlogger_dir/include" ]; then
            ln -sf "$mmlogger_dir/include" include
            echo "已链接include目录"
        else
            echo "警告: $mmlogger_dir/include 目录不存在"
            mkdir -p include
        fi
        
        if [ -d "$mmlogger_dir/src" ]; then
            ln -sf "$mmlogger_dir/src" src
            echo "已链接src目录"
        else
            echo "警告: $mmlogger_dir/src 目录不存在"
            mkdir -p src
        fi
    else
        # 如果用户跳过，创建空目录
        mkdir -p include src
        echo "已创建空的include和src目录，您需要手动添加MMLogger源代码"
    fi
fi

# 设置测试可执行程序链接
echo -e "${GREEN}设置项目完成!${NC}"
echo "接下来您应该编辑test_config.yaml定义测试场景，然后运行:"
echo "  make           # 构建并运行所有测试"
echo "或:"
echo "  python3 run_benchmarks.py  # 直接运行测试"

# 赋予执行权限
chmod +x run_benchmarks.py
if [ -f "build.sh" ]; then
    chmod +x build.sh
fi

echo -e "${GREEN}设置完成!${NC}"
