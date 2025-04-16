/**
 * 用于检测终端输出并处理颜色的辅助代码
 */
#ifndef CPP_COLOR_FIX_H
#define CPP_COLOR_FIX_H

#include <string>
#include <iostream>
#include <unistd.h>

namespace benchmark {
namespace color {

// 检测是否输出到终端
inline bool is_terminal_output(FILE* file = stdout) {
    return isatty(fileno(file));
}

// 颜色代码
static const std::string GREEN = is_terminal_output() ? "\033[0;32m" : "";
static const std::string YELLOW = is_terminal_output() ? "\033[1;33m" : "";
static const std::string RED = is_terminal_output() ? "\033[0;31m" : "";
static const std::string NC = is_terminal_output() ? "\033[0m" : "";  // 无颜色

// 着色辅助函数
inline std::string green(const std::string& text) {
    return GREEN + text + NC;
}

inline std::string yellow(const std::string& text) {
    return YELLOW + text + NC;
}

inline std::string red(const std::string& text) {
    return RED + text + NC;
}

} // namespace color
} // namespace benchmark

#endif // CPP_COLOR_FIX_H
