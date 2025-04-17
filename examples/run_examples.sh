#!/bin/bash
# 运行MMLogger示例程序的脚本

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # 无颜色

# 检查构建目录中示例程序是否存在
EXAMPLE_DIR="../build/examples"
if [ ! -d "$EXAMPLE_DIR" ]; then
    echo -e "${RED}错误: 找不到构建的示例程序目录 $EXAMPLE_DIR${NC}"
    echo -e "请先构建项目:"
    echo -e "${YELLOW}mkdir -p ../build && cd ../build && cmake .. -DMM_BUILD_EXAMPLES=ON && make${NC}"
    exit 1
fi

# 确保日志目录存在
mkdir -p ../build/examples/logs

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# 切换到构建目录
cd "$EXAMPLE_DIR" || exit 1

# 显示菜单
show_menu() {
    echo -e "${GREEN}MMLogger 示例程序${NC}"
    echo "=========================="
    echo "请选择要运行的示例:"
    echo "1) 基本示例"
    echo "2) 高级示例"
    echo "3) OptimizedGLog示例"
    echo "4) 完整功能示例 - 基本模式"
    echo "5) 完整功能示例 - 多线程模式"
    echo "6) 完整功能示例 - 速率限制模式"
    echo "7) 完整功能示例 - 压力测试模式"
    echo "8) 完整功能示例 - 所有模式"
    echo "9) 完整功能示例 - 显示帮助"
    echo "0) 退出"
    echo "=========================="
    echo -ne "${YELLOW}请输入选择 [0-9]: ${NC}"
}

# 运行选择的示例
run_example() {
    case $1 in
        1)
            echo -e "\n${BLUE}运行基本示例...${NC}\n"
            ./mmlogger_example --console=true
            ;;
        2)
            echo -e "\n${BLUE}运行高级示例...${NC}\n"
            ./mmlogger_advanced_example --console=true
            ;;
        3)
            echo -e "\n${BLUE}运行OptimizedGLog示例...${NC}\n"
            ./mmlogger_optimized_glog_example --console=true
            ;;
        4)
            echo -e "\n${BLUE}运行完整功能示例 - 基本模式...${NC}\n"
            ./mmlogger_complete_example --console=true --demo-mode=basic
            ;;
        5)
            echo -e "\n${BLUE}运行完整功能示例 - 多线程模式...${NC}\n"
            ./mmlogger_complete_example --console=true --demo-mode=threads
            ;;
        6)
            echo -e "\n${BLUE}运行完整功能示例 - 速率限制模式...${NC}\n"
            ./mmlogger_complete_example --console=true --demo-mode=rate-limited
            ;;
        7)
            echo -e "\n${BLUE}运行完整功能示例 - 压力测试模式...${NC}\n"
            ./mmlogger_complete_example --console=true --demo-mode=stress-test
            ;;
        8)
            echo -e "\n${BLUE}运行完整功能示例 - 所有模式...${NC}\n"
            ./mmlogger_complete_example --console=true --demo-mode=all
            ;;
        9)
            echo -e "\n${BLUE}显示完整功能示例帮助...${NC}\n"
            ./mmlogger_complete_example --help
            ;;
        0)
            echo -e "\n${GREEN}退出程序${NC}"
            exit 0
            ;;
        *)
            echo -e "\n${RED}无效选择!${NC}"
            ;;
    esac
}

# 主循环
while true; do
    show_menu
    read -r choice
    
    if [[ $choice =~ ^[0-9]$ ]]; then
        run_example "$choice"
        
        echo
        read -p "按Enter键继续..." -r
        clear
    else
        echo -e "${RED}无效输入,请输入0-9之间的数字${NC}"
        sleep 1
        clear
    fi
done
