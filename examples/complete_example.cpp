// examples/complete_example.cpp - 全面演示MMLogger所有功能和参数的示例

#include <Log.hpp>
#include <LoggerManager.hpp>
#include <LogBaseDef.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <filesystem>

// 自定义帮助信息函数
void showHelp() {
    std::cout << "MMLogger 完整示例应用\n"
              << "=======================\n\n"
              << "该示例演示了MMLogger库的各种功能和配置选项。\n\n"
              << "命令行参数:\n"
              << "  --appid=NAME           设置应用标识符\n"
              << "  --sinktype=TYPE        设置日志后端类型 (Stdout, GLog, OptimizedGLog)\n"
              << "  --console=BOOL         启用/禁用控制台输出 (true, false, 默认:false)\n"
              << "  --toTerm=LEVEL         设置控制台日志级别 (verbose, debug, info, warn, error, fatal)\n"
              << "  --file=BOOL            启用/禁用文件日志 (true, false)\n"
              << "  --filepath=PATH        设置日志文件路径\n"
              << "  --toFile=LEVEL         设置文件日志级别 (verbose, debug, info, warn, error, fatal)\n"
              << "  --debugSwitch=BOOL     启用/禁用调试日志 (true, false)\n"
              << "  --demo-mode=MODE       演示模式 (basic, threads, rate-limited, stress-test, all)\n"
              << "\n"
              << "OptimizedGLog 特有参数:\n"
              << "  --batchSize=N          批处理大小 (默认:100)\n"
              << "  --queueCapacity=N      队列容量 (默认:10000)\n"
              << "  --numWorkers=N         工作线程数 (默认:2)\n"
              << "  --poolSize=N           内存池大小 (默认:10000)\n"
              << "\n"
              << "示例:\n"
              << "  ./complete_example --sinktype=OptimizedGLog --console=true --toTerm=info --file=true --filepath=./logs\n"
              << "  ./complete_example --sinktype=GLog --file=true --filepath=/var/log/myapp\n"
              << "  ./complete_example --demo-mode=threads --console=true\n"
              << "  ./complete_example --demo-mode=all --console=true\n";
}

// 输出当前配置信息
void showCurrentConfig(const mm::LogConfig& config) {
    std::cout << "\n当前MMLogger配置:\n"
              << "----------------------\n"
              << "应用ID: " << (config.appId_.empty() ? "(默认)" : config.appId_) << "\n"
              << "日志后端: ";
              
    switch (config.logSinkType_) {
        case mm::detail::LogSinkType_Stdout: std::cout << "Stdout"; break;
        case mm::detail::LogSinkType_GLog: std::cout << "GLog"; break;
        case mm::detail::LogSinkType_OptimizedGLog: std::cout << "OptimizedGLog"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << "\n";
    
    std::cout << "控制台输出: " << (config.logToConsole_ ? "启用" : "禁用") << "\n";
    
    std::cout << "控制台日志级别: ";
    switch (config.logLevelToStderr_) {
        case mm::detail::LogLevel_Verbose: std::cout << "verbose"; break;
        case mm::detail::LogLevel_Debug: std::cout << "debug"; break;
        case mm::detail::LogLevel_Info: std::cout << "info"; break;
        case mm::detail::LogLevel_Warn: std::cout << "warn"; break;
        case mm::detail::LogLevel_Error: std::cout << "error"; break;
        case mm::detail::LogLevel_Fatal: std::cout << "fatal"; break;
        default: std::cout << "unknown"; break;
    }
    std::cout << "\n";
    
    std::cout << "文件日志: " << (config.logToFile_ ? "启用" : "禁用") << "\n";
    if (config.logToFile_) {
        std::cout << "文件路径: " << config.logFilePath_ << "\n";
        
        std::cout << "文件日志级别: ";
        switch (config.logLevelToFile_) {
            case mm::detail::LogLevel_Verbose: std::cout << "verbose"; break;
            case mm::detail::LogLevel_Debug: std::cout << "debug"; break;
            case mm::detail::LogLevel_Info: std::cout << "info"; break;
            case mm::detail::LogLevel_Warn: std::cout << "warn"; break;
            case mm::detail::LogLevel_Error: std::cout << "error"; break;
            case mm::detail::LogLevel_Fatal: std::cout << "fatal"; break;
            default: std::cout << "unknown"; break;
        }
        std::cout << "\n";
    }
    
    std::cout << "调试日志: " << (config.logDebugSwitch_ ? "启用" : "禁用") << "\n";
    
    if (config.logSinkType_ == mm::detail::LogSinkType_OptimizedGLog) {
        std::cout << "\nOptimizedGLog 特有配置:\n";
        std::cout << "批处理大小: " << config.optimizationConfig_.batchSize << "\n";
        std::cout << "队列容量: " << config.optimizationConfig_.queueCapacity << "\n";
        std::cout << "工作线程数: " << config.optimizationConfig_.numWorkers << "\n";
        std::cout << "内存池大小: " << config.optimizationConfig_.poolSize << "\n";
    }
    std::cout << "----------------------\n\n";
}

// 演示基本日志功能
void demoBasicLogging() {
    std::cout << "\n[演示] 基本日志功能\n";
    
    MM_INFO("基本日志演示开始");
    
    // 不同级别的日志
    MM_VERBOSE("这是一个VERBOSE级别的日志");
    MM_DEBUG("这是一个DEBUG级别的日志");
    MM_INFO("这是一个INFO级别的日志");
    MM_WARN("这是一个WARN级别的日志");
    MM_ERROR("这是一个ERROR级别的日志");
    // 注意：故意不演示FATAL，因为它会终止程序
    
    // 带格式化的日志
    MM_INFO("支持格式化: 整数=%d, 字符串=%s, 浮点数=%.2f", 42, "Hello World", 3.14159);
    
    // 带文件、函数、行号信息的日志
    MM_INFO("日志会自动包含文件、函数和行号信息");
    
    MM_INFO("基本日志演示结束");
}

// 演示多线程日志记录
void demoThreadedLogging() {
    const int numThreads = 4;
    const int logsPerThread = 10;
    
    std::cout << "\n[演示] 多线程日志记录\n";
    
    MM_INFO("多线程日志演示开始 (%d线程, 每个线程%d条日志)", numThreads, logsPerThread);
    
    std::vector<std::thread> threads;
    for (int threadId = 0; threadId < numThreads; ++threadId) {
        threads.emplace_back([threadId, logsPerThread]() {
            for (int i = 0; i < logsPerThread; ++i) {
                MM_INFO("线程%d: 日志消息 #%d", threadId, i);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    MM_INFO("多线程日志演示结束");
}

// 演示速率限制日志
void demoRateLimitedLogging() {
    std::cout << "\n[演示] 速率限制日志\n";
    
    MM_INFO("速率限制日志演示开始");
    
    // 创建速率限制器，限制为每秒1条日志
    mm::detail::RateLimitedLog rateLimitedLogger(std::chrono::milliseconds(1000));
    
    MM_INFO("下面将快速产生10条日志，但由于速率限制，只有部分会实际输出");
    
    for (int i = 0; i < 10; ++i) {
        // 这将被限制为每秒只输出一条
        rateLimitedLogger.Log(
            mm::detail::LogLevel_Info, "这是速率限制的日志 #%d", i);
        
        // 这些非速率限制的日志将全部输出
        MM_INFO("这是常规日志 #%d", i);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    MM_INFO("速率限制日志演示结束");
}

// 压力测试 - 产生大量日志
void demoStressTest() {
    const int totalLogs = 1000;
    const int batchSize = 100;
    
    std::cout << "\n[演示] 日志压力测试\n";
    
    MM_INFO("日志压力测试开始 (%d条日志)", totalLogs);
    
    auto startTime = std::chrono::steady_clock::now();
    
    for (int i = 0; i < totalLogs; ++i) {
        if (i % batchSize == 0) {
            MM_INFO("正在生成日志: %d/%d", i, totalLogs);
        }
        
        // 使用不同级别的日志
        if (i % 100 == 0) {
            MM_ERROR("错误日志 #%d: 模拟错误情况", i);
        } else if (i % 20 == 0) {
            MM_WARN("警告日志 #%d: 潜在问题", i);
        } else {
            MM_INFO("信息日志 #%d: 正常执行", i);
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    MM_INFO("日志压力测试结束 - %d条日志用时%ld毫秒，大约每秒%.2f条日志",
        totalLogs, duration, totalLogs * 1000.0 / duration);
}

// 创建自定义命令行参数 - 用更强大的方式覆盖默认参数
void setupCustomArgs(int& argc, char**& argv, bool enableConsole = true) {
    bool hasConsoleArg = false;
    
    // 检查是否已有console参数
    for (int i = 1; i < argc; ++i) {
        if (strstr(argv[i], "--console=") == argv[i]) {
            hasConsoleArg = true;
            break;
        }
    }
    
    // 如果没有，添加一个
    if (!hasConsoleArg && enableConsole) {
        // 创建一个新的参数数组
        int newArgc = argc + 1;
        char** newArgv = new char*[newArgc + 1];  // +1 for NULL terminator
        
        // 复制原始参数
        for (int i = 0; i < argc; ++i) {
            newArgv[i] = argv[i];
        }
        
        // 添加console参数
        newArgv[argc] = new char[15];  // 足够存储 "--console=true"
        strcpy(newArgv[argc], "--console=true");
        
        // 添加NULL结束符
        newArgv[newArgc] = nullptr;
        
        // 更新参数
        argc = newArgc;
        argv = newArgv;
    }
}

// 解析命令行自定义参数
std::string parseDemoMode(int argc, char* argv[]) {
    std::string demoMode = "basic"; // 默认值
    
    for (int i = 1; i < argc; ++i) {
        if (strstr(argv[i], "--demo-mode=") == argv[i]) {
            const char* value = strchr(argv[i], '=') + 1;
            demoMode = value;
            break;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            showHelp();
            exit(0);
        }
    }
    
    return demoMode;
}

// 确保目录存在
void ensureDirectoryExists(const std::string& path) {
    if (!path.empty()) {
        std::filesystem::create_directories(path);
    }
}

// 主函数
int main(int argc, char* argv[]) {
    // 添加控制台输出参数（如果没有指定）
    setupCustomArgs(argc, argv);
    
    // 解析自定义参数
    std::string demoMode = parseDemoMode(argc, argv);
    
    std::cout << "启动MMLogger完整示例，演示模式: " << demoMode << std::endl;
    
    // 获取日志管理器实例
    mm::LoggerManager& logManager = mm::LoggerManager::instance();

    // 检查文件路径参数，确保目录存在
    for (int i = 1; i < argc; ++i) {
        if (strstr(argv[i], "--filepath=") == argv[i]) {
            const char* filepath = strchr(argv[i], '=') + 1;
            ensureDirectoryExists(filepath);
            break;
        }
    }

    
    // 配置和启动日志器
    int result = logManager.setup(argc, argv);
    if (result != 0) {
        std::cerr << "日志初始化失败，错误码: " << result << std::endl;
        return 1;
    }

    // 设置日志基础设施
    logManager.setupLogger();
    
    // 显示当前配置
    showCurrentConfig(logManager.config());
    
    // 启动日志器
    logManager.Start();
    
    // 记录应用启动日志
    MM_INFO("应用启动，PID: %d", getpid());
    
    // 根据指定的演示模式运行相应的演示
    if (demoMode == "basic" || demoMode == "all") {
        demoBasicLogging();
    }
    
    if (demoMode == "threads" || demoMode == "all") {
        demoThreadedLogging();
    }
    
    if (demoMode == "rate-limited" || demoMode == "all") {
        demoRateLimitedLogging();
    }
    
    if (demoMode == "stress-test" || demoMode == "all") {
        demoStressTest();
    }
    
    if (demoMode != "basic" && demoMode != "threads" && 
        demoMode != "rate-limited" && demoMode != "stress-test" && 
        demoMode != "all") {
        MM_WARN("未知的演示模式: %s，将运行基本演示", demoMode.c_str());
        demoBasicLogging();
    }
    
    // 记录应用关闭日志
    MM_INFO("应用关闭");
    
    // 清理
    logManager.teardown();
    
    std::cout << "\n示例完成。要查看完整的命令行选项，请使用 --help 参数运行。" << std::endl;
    
    return 0;
}
