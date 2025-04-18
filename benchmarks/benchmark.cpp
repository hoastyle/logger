/**
 * MMLogger性能基准测试工具
 *
 * 基于YAML配置的灵活性与高精度性能测量相结合的优化版本
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <signal.h>
#include <filesystem>
#include <map>
#include "cpp_color_fix.h"
#include <random>

#include <Log.hpp>
#include <LoggerManager.hpp>
#include <LogBaseDef.hpp>

std::atomic<uint64_t> g_totalLogsGenerated(0);
std::atomic<bool> g_runBenchmark(false);
std::mutex g_latencyMutex;
std::vector<uint64_t> g_latencies;

struct BenchmarkConfig {
  std::string testId;
  std::string testName;

  std::string loggerType;  // "Stdout", "GLog", "OptimizedGLog"
  bool enableConsoleOutput;
  bool enableFileOutput;
  std::string logFilePath;
  std::string logLevel;  // "debug", "info", "warn", "error"

  int numThreads;
  int logsPerThread;
  int logMessageSize;
  int logRatePerSecond;

  double debugLogPercentage;
  double infoLogPercentage;
  double warnLogPercentage;
  double errorLogPercentage;

  int batchSize;
  int queueCapacity;
  int numWorkers;
  int poolSize;

  // test execute parameters
  int warmupSeconds;
  int testDurationSeconds;
  int cooldownSeconds;
  bool measureLatency;
  bool useRateLimit;

  std::string outputFile;
  bool appendOutput;
  bool verboseOutput;

  BenchmarkConfig()
      : testId("benchmark"),
        testName("MMLogger性能测试"),
        loggerType("OptimizedGLog"),
        enableConsoleOutput(true),
        enableFileOutput(false),
        logFilePath("./logs"),
        logLevel("info"),
        numThreads(4),
        logsPerThread(100000),
        logMessageSize(128),
        logRatePerSecond(0),
        debugLogPercentage(10.0),
        infoLogPercentage(60.0),
        warnLogPercentage(20.0),
        errorLogPercentage(10.0),
        batchSize(200),
        queueCapacity(10000),
        numWorkers(4),
        poolSize(20000),
        warmupSeconds(2),
        testDurationSeconds(10),
        cooldownSeconds(2),
        measureLatency(true),
        useRateLimit(false),
        outputFile("benchmark_results.csv"),
        appendOutput(true),
        verboseOutput(true) {}
};

struct PerformanceMetrics {
  // throughput KPI
  double logsPerSecond;
  double bytesPerSecond;

  // latency KPI (us)
  double avgLatency;
  double p50Latency;
  double p90Latency;
  double p95Latency;
  double p99Latency;
  double maxLatency;

  // resource usage
  double cpuUsagePercent;
  double memoryUsageMB;
  double diskWritesBytesPerSec;
  // disk io in MB/s
  double diskWritesMBPerSec;

  // OptimizedGLog stats
  uint64_t enqueuedCount;
  uint64_t processedCount;
  uint64_t droppedCount;
  uint64_t overflowCount;
  double queueUtilization;

  PerformanceMetrics()
      : logsPerSecond(0),
        bytesPerSecond(0),
        avgLatency(0),
        p50Latency(0),
        p90Latency(0),
        p95Latency(0),
        p99Latency(0),
        maxLatency(0),
        cpuUsagePercent(0),
        memoryUsageMB(0),
        diskWritesBytesPerSec(0),
        diskWritesMBPerSec(0),
        enqueuedCount(0),
        processedCount(0),
        droppedCount(0),
        overflowCount(0),
        queueUtilization(0) {}
};

// high precision timer
class Timer {
 private:
  std::chrono::high_resolution_clock::time_point startTime;
  bool running;

 public:
  Timer() : running(false) {}

  void start() {
    startTime = std::chrono::high_resolution_clock::now();
    running   = true;
  }

  std::chrono::nanoseconds elapsed() const {
    if (!running) return std::chrono::nanoseconds(0);
    auto endTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTime - startTime);
  }

  double elapsedSeconds() const { return elapsed().count() / 1e9; }
  double elapsedMilliseconds() const { return elapsed().count() / 1e6; }
  double elapsedMicroseconds() const { return elapsed().count() / 1e3; }
};

// 性能统计收集类
class PerformanceMonitor {
 private:
  Timer timer;
  rusage startUsage;
  rusage endUsage;
  std::string statFilePath;
  uint64_t startDiskWrite;
  uint64_t endDiskWrite;
  int diskFd;
  std::string monitoredPath;

  uint64_t getDiskWriteBytes() {
    // 尝试从/proc获取I/O信息
    char statBuf[256];
    ssize_t bytesRead = pread(diskFd, statBuf, sizeof(statBuf) - 1, 0);
    if (bytesRead <= 0) return 0;

    statBuf[bytesRead] = '\0';
    char* pos          = strstr(statBuf, "write_bytes");
    if (!pos) return 0;

    uint64_t writeBytes = 0;
    sscanf(pos, "write_bytes %lu", &writeBytes);
    return writeBytes;
  }

 public:
  PerformanceMonitor(const std::string& path = "/proc/self/io",
      const std::string& monPath             = "")
      : statFilePath(path),
        startDiskWrite(0),
        endDiskWrite(0),
        diskFd(-1),
        monitoredPath(monPath) {}

  ~PerformanceMonitor() {
    if (diskFd >= 0) {
      close(diskFd);
    }
  }

  bool start() {
    // 启动计时器
    timer.start();

    // 获取初始资源使用情况
    if (getrusage(RUSAGE_SELF, &startUsage) != 0) {
      std::cerr << "获取初始资源使用信息失败" << std::endl;
      return false;
    }

    // 打开磁盘统计文件
    diskFd = open(statFilePath.c_str(), O_RDONLY);
    if (diskFd < 0) {
      std::cerr << "警告: 无法打开 " << statFilePath << " 获取磁盘I/O统计"
                << std::endl;
      // 不返回失败，因为这可能在某些系统上不可用
    } else {
      startDiskWrite = getDiskWriteBytes();
    }

    return true;
  }

  bool stop() {
    // 获取最终资源使用情况
    if (getrusage(RUSAGE_SELF, &endUsage) != 0) {
      std::cerr << "获取最终资源使用信息失败" << std::endl;
      return false;
    }

    // 获取最终磁盘统计
    if (diskFd >= 0) {
      endDiskWrite = getDiskWriteBytes();
    }

    return true;
  }

  double getCpuUsage() const {
    // 计算用户和系统CPU时间总和（秒）
    double startCpuTime =
        startUsage.ru_utime.tv_sec + startUsage.ru_stime.tv_sec +
        (startUsage.ru_utime.tv_usec + startUsage.ru_stime.tv_usec) / 1e6;

    double endCpuTime =
        endUsage.ru_utime.tv_sec + endUsage.ru_stime.tv_sec +
        (endUsage.ru_utime.tv_usec + endUsage.ru_stime.tv_usec) / 1e6;

    double cpuTimeDiff = endCpuTime - startCpuTime;
    double elapsedTime = timer.elapsedSeconds();

    // 考虑多核CPU，计算平均CPU使用率
    int numCores = sysconf(_SC_NPROCESSORS_ONLN);
    return (cpuTimeDiff / elapsedTime) * 100.0 / numCores;
  }

  double getMemoryUsage() const {
    // 返回最大内存使用量（MB）
    return endUsage.ru_maxrss / 1024.0;
  }

  double getDiskWriteRate() const {
    if (diskFd < 0) return 0.0;

    // 计算磁盘写入率（字节/秒）
    uint64_t writeBytesDiff = endDiskWrite - startDiskWrite;
    double elapsedTime      = timer.elapsedSeconds();

    return writeBytesDiff / elapsedTime;
  }

  double getElapsedTime() const { return timer.elapsedSeconds(); }

  double getDirectorySize() const {
    if (monitoredPath.empty()) return 0.0;

    uint64_t totalSize = 0;
    try {
      for (const auto& entry :
          std::filesystem::recursive_directory_iterator(monitoredPath)) {
        if (std::filesystem::is_regular_file(entry)) {
          totalSize += std::filesystem::file_size(entry);
        }
      }
    } catch (const std::exception& e) {
      std::cerr << "获取目录大小时出错: " << e.what() << std::endl;
    }

    return totalSize;
  }
};

// 生成随机日志内容
std::string generateRandomContent(size_t size) {
  static const char charset[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  // 使用更好的随机数生成器
  static std::mt19937 rng(std::random_device{}());
  static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

  std::string result;
  result.reserve(size);

  for (size_t i = 0; i < size; ++i) {
    result += charset[dist(rng)];
  }

  return result;
}

// 生成日志的线程函数
void logGenerationThread(int threadId, const BenchmarkConfig& config) {
  std::vector<double> logDistribution = {config.debugLogPercentage,
      config.infoLogPercentage, config.warnLogPercentage,
      config.errorLogPercentage};

  double sum = 0.0;
  for (double d : logDistribution) {
    sum += d;
  }
  if (sum > 0) {
    for (double& d : logDistribution) {
      d /= sum;
    }
  } else {
    for (double& d : logDistribution) {
      d = 0.25;
    }
  }

  // 创建累积分布
  for (size_t i = 1; i < logDistribution.size(); ++i) {
    logDistribution[i] += logDistribution[i - 1];
  }

  // 创建消息基础前缀
  std::string messagePrefix =
      "Thread " + std::to_string(threadId) + " - Log message ";

  // 创建一些随机内容变体
  std::vector<std::string> contentVariants;
  for (int i = 0; i < 5; ++i) {
    // 先复制前缀
    std::string variant = messagePrefix;

    // 添加随机字符直到达到目标大小
    int randomCharsNeeded = config.logMessageSize - variant.size();
    if (randomCharsNeeded > 0) {
      variant += generateRandomContent(randomCharsNeeded);
    }

    contentVariants.push_back(variant);
  }

  // 日志速率控制
  bool rateLimit = config.logRatePerSecond > 0;
  const auto logIntervalNs =
      rateLimit
          ? std::chrono::nanoseconds(
                1000000000 / (config.logRatePerSecond / config.numThreads))
          : std::chrono::nanoseconds(0);
  auto nextLogTime = std::chrono::steady_clock::now();

  // 创建速率限制器（如果需要）
  mm::detail::RateLimitedLog* rateLimiter = nullptr;
  if (config.useRateLimit) {
    int interval = (config.logRatePerSecond > 0)
                       ? (1000 / (config.logRatePerSecond / config.numThreads))
                       : 100;
    rateLimiter =
        new mm::detail::RateLimitedLog(std::chrono::milliseconds(interval));
  }

  // 主日志循环
  int logsGenerated = 0;
  std::mt19937 rng(
      std::random_device{}() + threadId);  // 线程特定的随机数生成器
  std::uniform_real_distribution<> dist(0.0, 1.0);
  std::uniform_int_distribution<> variantDist(0, contentVariants.size() - 1);

  while (g_runBenchmark.load() && logsGenerated < config.logsPerThread) {
    // 速率限制（如果需要）
    if (rateLimit) {
      auto now = std::chrono::steady_clock::now();
      if (now < nextLogTime) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(10));
        continue;
      }
      nextLogTime += logIntervalNs;
    }

    // 生成随机数决定日志级别
    double r       = dist(rng);
    int msgVariant = variantDist(rng);

    // 使用适当的级别记录日志
    if (config.measureLatency) {
      // 测量延迟
      auto startTime = std::chrono::high_resolution_clock::now();

      // 根据分布选择日志级别
      if (r < logDistribution[0]) {
        if (config.useRateLimit && rateLimiter) {
          rateLimiter->Log(mm::detail::LogLevel_Debug, "%s #%d",
              contentVariants[msgVariant].c_str(), logsGenerated);
        } else {
          MM_DEBUG(
              "%s #%d", contentVariants[msgVariant].c_str(), logsGenerated);
        }
      } else if (r < logDistribution[1]) {
        if (config.useRateLimit && rateLimiter) {
          rateLimiter->Log(mm::detail::LogLevel_Info, "%s #%d",
              contentVariants[msgVariant].c_str(), logsGenerated);
        } else {
          MM_INFO("%s #%d", contentVariants[msgVariant].c_str(), logsGenerated);
        }
      } else if (r < logDistribution[2]) {
        if (config.useRateLimit && rateLimiter) {
          rateLimiter->Log(mm::detail::LogLevel_Warn, "%s #%d",
              contentVariants[msgVariant].c_str(), logsGenerated);
        } else {
          MM_WARN("%s #%d", contentVariants[msgVariant].c_str(), logsGenerated);
        }
      } else {
        if (config.useRateLimit && rateLimiter) {
          rateLimiter->Log(mm::detail::LogLevel_Error, "%s #%d",
              contentVariants[msgVariant].c_str(), logsGenerated);
        } else {
          MM_ERROR(
              "%s #%d", contentVariants[msgVariant].c_str(), logsGenerated);
        }
      }

      auto endTime = std::chrono::high_resolution_clock::now();
      auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
          endTime - startTime)
                         .count();

      // 存储延迟
      {
        std::lock_guard<std::mutex> lock(g_latencyMutex);
        g_latencies.push_back(latency);
      }
    } else {
      // 不测量延迟，直接记录日志
      if (r < logDistribution[0]) {
        MM_DEBUG("%s #%d", contentVariants[msgVariant].c_str(), logsGenerated);
      } else if (r < logDistribution[1]) {
        MM_INFO("%s #%d", contentVariants[msgVariant].c_str(), logsGenerated);
      } else if (r < logDistribution[2]) {
        MM_WARN("%s #%d", contentVariants[msgVariant].c_str(), logsGenerated);
      } else {
        MM_ERROR("%s #%d", contentVariants[msgVariant].c_str(), logsGenerated);
      }
    }

    logsGenerated++;
    g_totalLogsGenerated++;
  }

  // 清理速率限制器
  if (rateLimiter) {
    delete rateLimiter;
  }
}

// 计算延迟百分位数
void calculateLatencyPercentiles(
    const std::vector<uint64_t>& latencies, PerformanceMetrics& metrics) {
  if (latencies.empty()) {
    metrics.avgLatency = 0;
    metrics.p50Latency = 0;
    metrics.p90Latency = 0;
    metrics.p95Latency = 0;
    metrics.p99Latency = 0;
    metrics.maxLatency = 0;
    return;
  }

  // 创建一个副本用于排序
  std::vector<uint64_t> sortedLatencies = latencies;
  std::sort(sortedLatencies.begin(), sortedLatencies.end());

  // 计算平均值
  double sum = 0.0;
  for (uint64_t latency : sortedLatencies) {
    sum += latency;
  }
  metrics.avgLatency = sum / sortedLatencies.size() / 1000.0;  // 转换为微秒

  // 计算百分位数
  size_t size        = sortedLatencies.size();
  metrics.p50Latency = sortedLatencies[size * 0.5] / 1000.0;
  metrics.p90Latency = sortedLatencies[size * 0.9] / 1000.0;
  metrics.p95Latency = sortedLatencies[size * 0.95] / 1000.0;
  metrics.p99Latency = sortedLatencies[size * 0.99] / 1000.0;
  metrics.maxLatency = sortedLatencies.back() / 1000.0;
}

std::string getSystemInfo() {
  std::string info;

  char hostname[1024];
  gethostname(hostname, 1024);
  info += std::string("主机名: ") + hostname + "; ";

  std::ifstream cpuinfo("/proc/cpuinfo");
  std::string line;
  int cpuCount = 0;
  std::string cpuModel;

  while (std::getline(cpuinfo, line)) {
    if (line.find("model name") != std::string::npos) {
      cpuCount++;
      if (cpuModel.empty()) {
        cpuModel = line.substr(line.find(":") + 2);
      }
    }
  }

  info += "CPU: " + cpuModel + " x" + std::to_string(cpuCount) + "; ";

  std::ifstream meminfo("/proc/meminfo");
  std::string totalMem;

  while (std::getline(meminfo, line)) {
    if (line.find("MemTotal") != std::string::npos) {
      totalMem = line.substr(line.find(":") + 1);
      break;
    }
  }

  info += "内存: " + totalMem + "; ";

  struct statvfs diskStat;
  if (statvfs("/", &diskStat) == 0) {
    unsigned long long totalSpace = diskStat.f_frsize * diskStat.f_blocks;
    unsigned long long freeSpace  = diskStat.f_frsize * diskStat.f_bfree;
    info += "磁盘: 总计=" + std::to_string(totalSpace / (1024 * 1024 * 1024)) +
            "GB, 可用=" + std::to_string(freeSpace / (1024 * 1024 * 1024)) +
            "GB";
  }

  return info;
}

// 从OptimizedGLog日志器获取内部状态
// 注意：这需要日志器提供对应的接口或输出统计信息
bool getLoggerInternalStats(PerformanceMetrics& metrics,
    mm::LoggerManager& logManager, const BenchmarkConfig& config) {
  // 检查是否是OptimizedGLog
  if (config.loggerType != "OptimizedGLog") {
    return false;
  }

  try {
    // 实现日志器统计获取
    // 方式1：解析stderr日志获取统计信息
    // 从stderr日志文件中搜索 "OptimizedGlogLogger stats" 行
    std::ifstream logFile("benchmark_log.txt");
    if (logFile.is_open()) {
      std::string line;
      while (std::getline(logFile, line)) {
        if (line.find("OptimizedGlogLogger stats") != std::string::npos) {
          // 示例行: "OptimizedGlogLogger stats - Enqueued: 15723, Processed:
          // 15723, Dropped: 622990, Overflow: 81336" 解析这行获取数据
          size_t pos;

          pos = line.find("Enqueued: ");
          if (pos != std::string::npos) {
            metrics.enqueuedCount = std::stoull(line.substr(pos + 10));
          }

          pos = line.find("Processed: ");
          if (pos != std::string::npos) {
            metrics.processedCount = std::stoull(line.substr(pos + 11));
          }

          pos = line.find("Dropped: ");
          if (pos != std::string::npos) {
            metrics.droppedCount = std::stoull(line.substr(pos + 9));
          }

          pos = line.find("Overflow: ");
          if (pos != std::string::npos) {
            metrics.overflowCount = std::stoull(line.substr(pos + 10));
          }

          break;
        }
      }
      logFile.close();
    }

    // 方式2：如果没有日志文件，设置估计值
    if (metrics.enqueuedCount == 0) {
      metrics.enqueuedCount  = g_totalLogsGenerated.load();
      metrics.processedCount = g_totalLogsGenerated.load();
      metrics.droppedCount   = 0;
      metrics.overflowCount  = 0;
    }

    // 计算队列利用率
    if (config.queueCapacity > 0) {
      // 估计队列利用率
      metrics.queueUtilization =
          std::min(1.0, metrics.logsPerSecond / (config.queueCapacity * 10.0));
    }

    return true;
  } catch (const std::exception& e) {
    std::cerr << "获取日志器内部统计信息失败: " << e.what() << std::endl;
    return false;
  }
}

// 将结果写入CSV
void writeResultsToCSV(const BenchmarkConfig& config,
    const PerformanceMetrics& metrics, const std::string& systemInfo) {
  // 首先打印输出文件路径，方便调试
  std::cerr << "Writing results to CSV file: " << config.outputFile << std::endl;

  // 确保输出目录存在
  std::filesystem::path outputPath(config.outputFile);
  std::filesystem::create_directories(outputPath.parent_path());

  std::ofstream outputFile;
  if (config.appendOutput) {
    outputFile.open(config.outputFile, std::ios_base::app);

    // 检查文件是否为空，如果是空文件则需要添加头
    bool needHeader = false;
    if (outputFile.is_open()) {
      // 获取文件大小
      outputFile.seekp(0, std::ios::end);
      if (outputFile.tellp() == 0) {
        needHeader = true;
      }
      // 回到文件末尾准备追加
      outputFile.seekp(0, std::ios::end);
    }

    // 如果需要头，写入头
    if (needHeader) {
      outputFile << "Timestamp,TestID,TestName,LoggerType,ConsoleOutput,"
                    "FileOutput,LogLevel,"
                 << "NumThreads,LogsPerThread,MessageSize,LogRate,"
                 << "DebugPct,InfoPct,WarnPct,ErrorPct,"
                 << "BatchSize,QueueCapacity,NumWorkers,PoolSize,"
                 << "LogsPerSecond,BytesPerSecond,"
                 << "AvgLatencyUs,P50LatencyUs,P90LatencyUs,P95LatencyUs,"
                    "P99LatencyUs,MaxLatencyUs,"
                 << "CPUPercent,MemoryMB,DiskWritesBps,DiskWritesMBs,"
                 << "EnqueuedCount,ProcessedCount,DroppedCount,OverflowCount,"
                    "QueueUtilization,"
                 << "SystemInfo" << std::endl;
    }
  } else {
    outputFile.open(config.outputFile);

    // 如果文件是新创建的，写入CSV头
    if (outputFile.tellp() == 0) {
      outputFile << "Timestamp,TestID,TestName,LoggerType,ConsoleOutput,"
                    "FileOutput,LogLevel,"
                 << "NumThreads,LogsPerThread,MessageSize,LogRate,"
                 << "DebugPct,InfoPct,WarnPct,ErrorPct,"
                 << "BatchSize,QueueCapacity,NumWorkers,PoolSize,"
                 << "LogsPerSecond,BytesPerSecond,"
                 << "AvgLatencyUs,P50LatencyUs,P90LatencyUs,P95LatencyUs,"
                    "P99LatencyUs,MaxLatencyUs,"
                 << "CPUPercent,MemoryMB,DiskWritesBps,DiskWritesMBs,"
                 << "EnqueuedCount,ProcessedCount,DroppedCount,OverflowCount,"
                    "QueueUtilization,"
                 << "SystemInfo" << std::endl;
    }
  }

  if (!outputFile.is_open()) {
    std::cerr << "错误: 无法打开输出文件: " << config.outputFile << std::endl;
    return;
  }

  // 获取当前时间戳
  char timestamp[100];
  std::time_t now = std::time(nullptr);
  std::strftime(
      timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

  // 写入结果行
  outputFile << timestamp << "," << config.testId << ","
             << "\"" << config.testName << "\"," << config.loggerType << ","
             << (config.enableConsoleOutput ? "true" : "false") << ","
             << (config.enableFileOutput ? "true" : "false") << ","
             << config.logLevel << "," << config.numThreads << ","
             << config.logsPerThread << "," << config.logMessageSize << ","
             << config.logRatePerSecond << "," << config.debugLogPercentage
             << "," << config.infoLogPercentage << ","
             << config.warnLogPercentage << "," << config.errorLogPercentage
             << "," << config.batchSize << "," << config.queueCapacity << ","
             << config.numWorkers << "," << config.poolSize << "," << std::fixed
             << std::setprecision(2) << metrics.logsPerSecond << ","
             << std::fixed << std::setprecision(2) << metrics.bytesPerSecond
             << "," << std::fixed << std::setprecision(2) << metrics.avgLatency
             << "," << std::fixed << std::setprecision(2) << metrics.p50Latency
             << "," << std::fixed << std::setprecision(2) << metrics.p90Latency
             << "," << std::fixed << std::setprecision(2) << metrics.p95Latency
             << "," << std::fixed << std::setprecision(2) << metrics.p99Latency
             << "," << std::fixed << std::setprecision(2) << metrics.maxLatency
             << "," << std::fixed << std::setprecision(2)
             << metrics.cpuUsagePercent << "," << std::fixed
             << std::setprecision(2) << metrics.memoryUsageMB << ","
             << metrics.diskWritesBytesPerSec << "," << std::fixed
             << std::setprecision(2) << metrics.diskWritesMBPerSec << ","
             << metrics.enqueuedCount << "," << metrics.processedCount << ","
             << metrics.droppedCount << "," << metrics.overflowCount << ","
             << std::fixed << std::setprecision(4) << metrics.queueUtilization
             << ","
             << "\"" << systemInfo << "\"" << std::endl;

  outputFile.close();

  // 验证文件是否正确写入
  std::ifstream verifyFile(config.outputFile);
  if (verifyFile.is_open()) {
    verifyFile.seekg(0, std::ios::end);
    auto fileSize = verifyFile.tellg();
    verifyFile.close();

    std::cerr << "已成功写入CSV文件: " << config.outputFile
              << ", 文件大小: " << fileSize << " 字节" << std::endl;
  } else {
    std::cerr << "警告: 无法验证输出文件是否正确写入" << std::endl;
  }

  if (config.verboseOutput) {
    std::cout << "结果已写入 " << config.outputFile << std::endl;
  }
}

// Parse command line arguments
bool parseArgs(int argc, char* argv[], BenchmarkConfig& config) {
  // 从环境变量读取配置（由Python脚本设置）
  const char* envVars[] = {"TEST_ID", "TEST_NAME", "LOGGER_TYPE",
      "ENABLE_CONSOLE", "ENABLE_FILE", "LOG_FILE_PATH", "LOG_LEVEL",
      "NUM_THREADS", "LOGS_PER_THREAD", "LOG_MSG_SIZE", "LOG_RATE", "DEBUG_PCT",
      "INFO_PCT", "WARN_PCT", "ERROR_PCT", "BATCH_SIZE", "QUEUE_CAPACITY",
      "NUM_WORKERS", "POOL_SIZE", "WARMUP_SECONDS", "TEST_DURATION",
      "COOLDOWN_SECONDS", "OUTPUT_FILE", "APPEND_OUTPUT", "VERBOSE_OUTPUT",
      "MEASURE_LATENCY", "USE_RATE_LIMIT"};

  std::map<std::string, std::string> envValues;
  std::cerr << "Reading environment variables:" << std::endl;
  for (const char* var : envVars) {
    const char* value = getenv(var);
    if (value) {
      envValues[var] = value;
      std::cerr << "  " << var << " = " << value << std::endl;
    }
  }

  // 检查OUTPUT_FILE环境变量（这是最关键的一个）
  const char* outputFileEnv = getenv("OUTPUT_FILE");
  if (outputFileEnv) {
    std::cerr << "Found OUTPUT_FILE environment variable: " << outputFileEnv << std::endl;
    // 确保使用此值而不是默认值
    config.outputFile = outputFileEnv;
  } else {
    std::cerr << "WARNING: OUTPUT_FILE environment variable not set, using default: "
              << config.outputFile << std::endl;
  }

  // 设置默认值或使用环境变量值
  config.testId =
      envValues.count("TEST_ID") ? envValues["TEST_ID"] : "benchmark";
  config.testName   = envValues.count("TEST_NAME") ? envValues["TEST_NAME"]
                                                   : "MMLogger性能测试";
  config.loggerType = envValues.count("LOGGER_TYPE") ? envValues["LOGGER_TYPE"]
                                                     : "OptimizedGLog";
  config.enableConsoleOutput = envValues.count("ENABLE_CONSOLE")
                                   ? (envValues["ENABLE_CONSOLE"] == "true")
                                   : true;
  config.enableFileOutput    = envValues.count("ENABLE_FILE")
                                   ? (envValues["ENABLE_FILE"] == "true")
                                   : false;
  config.logFilePath =
      envValues.count("LOG_FILE_PATH") ? envValues["LOG_FILE_PATH"] : "./logs";
  config.logLevel =
      envValues.count("LOG_LEVEL") ? envValues["LOG_LEVEL"] : "info";

  config.numThreads =
      envValues.count("NUM_THREADS") ? std::stoi(envValues["NUM_THREADS"]) : 4;
  config.logsPerThread  = envValues.count("LOGS_PER_THREAD")
                              ? std::stoi(envValues["LOGS_PER_THREAD"])
                              : 100000;
  config.logMessageSize = envValues.count("LOG_MSG_SIZE")
                              ? std::stoi(envValues["LOG_MSG_SIZE"])
                              : 128;
  config.logRatePerSecond =
      envValues.count("LOG_RATE") ? std::stoi(envValues["LOG_RATE"]) : 0;

  config.debugLogPercentage =
      envValues.count("DEBUG_PCT") ? std::stod(envValues["DEBUG_PCT"]) : 10.0;
  config.infoLogPercentage =
      envValues.count("INFO_PCT") ? std::stod(envValues["INFO_PCT"]) : 60.0;
  config.warnLogPercentage =
      envValues.count("WARN_PCT") ? std::stod(envValues["WARN_PCT"]) : 20.0;
  config.errorLogPercentage =
      envValues.count("ERROR_PCT") ? std::stod(envValues["ERROR_PCT"]) : 10.0;

  config.batchSize =
      envValues.count("BATCH_SIZE") ? std::stoi(envValues["BATCH_SIZE"]) : 200;
  config.queueCapacity = envValues.count("QUEUE_CAPACITY")
                             ? std::stoi(envValues["QUEUE_CAPACITY"])
                             : 10000;
  config.numWorkers =
      envValues.count("NUM_WORKERS") ? std::stoi(envValues["NUM_WORKERS"]) : 4;
  config.poolSize =
      envValues.count("POOL_SIZE") ? std::stoi(envValues["POOL_SIZE"]) : 20000;

  config.warmupSeconds       = envValues.count("WARMUP_SECONDS")
                                   ? std::stoi(envValues["WARMUP_SECONDS"])
                                   : 2;
  config.testDurationSeconds = envValues.count("TEST_DURATION")
                                   ? std::stoi(envValues["TEST_DURATION"])
                                   : 10;
  config.cooldownSeconds     = envValues.count("COOLDOWN_SECONDS")
                                   ? std::stoi(envValues["COOLDOWN_SECONDS"])
                                   : 2;

  // 只有未设置OUTPUT_FILE环境变量时才使用默认值或配置中的值
  if (!outputFileEnv) {
    config.outputFile = envValues.count("OUTPUT_FILE") ? envValues["OUTPUT_FILE"]
                                                       : "benchmark_results.csv";
  }

  config.appendOutput   = envValues.count("APPEND_OUTPUT")
                              ? (envValues["APPEND_OUTPUT"] == "true")
                              : true;
  config.verboseOutput  = envValues.count("VERBOSE_OUTPUT")
                              ? (envValues["VERBOSE_OUTPUT"] == "true")
                              : true;
  config.measureLatency = envValues.count("MEASURE_LATENCY")
                              ? (envValues["MEASURE_LATENCY"] == "true")
                              : true;
  config.useRateLimit   = envValues.count("USE_RATE_LIMIT")
                              ? (envValues["USE_RATE_LIMIT"] == "true")
                              : false;

  // 输出最终配置，方便调试
  std::cerr << "最终配置:" << std::endl;
  std::cerr << "  测试ID: " << config.testId << std::endl;
  std::cerr << "  输出文件: " << config.outputFile << std::endl;
  std::cerr << "  日志器类型: " << config.loggerType << std::endl;

  return true;
}

int main(int argc, char* argv[]) {
  BenchmarkConfig config;
  if (!parseArgs(argc, argv, config)) {
    return 1;
  }

  if (config.verboseOutput) {
    std::cout << "开始MMLogger性能测试，配置如下:" << std::endl;
    std::cout << "  测试ID: " << config.testId << std::endl;
    std::cout << "  测试名称: " << config.testName << std::endl;
    std::cout << "  日志类型: " << config.loggerType << std::endl;
    std::cout << "  线程数: " << config.numThreads << std::endl;
    std::cout << "  每线程日志数: " << config.logsPerThread << std::endl;
    std::cout << "  消息大小: " << config.logMessageSize << " 字节"
              << std::endl;
    std::cout << "  测试持续时间: " << config.testDurationSeconds << " 秒"
              << std::endl;
  }

  // 重定向stdout和stderr到文件，用于捕获日志器的统计输出
  std::streambuf* coutbuf = std::cout.rdbuf();
  std::streambuf* cerrbuf = std::cerr.rdbuf();
  std::ofstream logFile("benchmark_log.txt");
  std::cout.rdbuf(logFile.rdbuf());
  std::cerr.rdbuf(logFile.rdbuf());

  // 设置日志器命令行参数
  std::vector<std::string> loggerArgs = {
      "--sinktype=" + config.loggerType, "--toTerm=" + config.logLevel};

  loggerArgs.push_back(
      "--console=" +
      std::string(config.enableConsoleOutput ? "true" : "false"));

  if (config.enableFileOutput) {
    loggerArgs.push_back("--file=true");
    loggerArgs.push_back("--filepath=" + config.logFilePath);
    loggerArgs.push_back("--toFile=" + config.logLevel);

    // 需要时创建日志目录
    std::filesystem::create_directories(config.logFilePath);
  }

  // 添加OptimizedGLog特定参数（如果需要）
  if (config.loggerType == "OptimizedGLog") {
    loggerArgs.push_back("--batchSize=" + std::to_string(config.batchSize));
    loggerArgs.push_back(
        "--queueCapacity=" + std::to_string(config.queueCapacity));
    loggerArgs.push_back("--numWorkers=" + std::to_string(config.numWorkers));
    loggerArgs.push_back("--poolSize=" + std::to_string(config.poolSize));
  }

  std::vector<char*> cArgs;
  cArgs.push_back(argv[0]);
  std::vector<std::string> stringArgs(loggerArgs);

  for (auto& arg : stringArgs) {
    cArgs.push_back(const_cast<char*>(arg.c_str()));
  }

  // 恢复标准输出和错误流，用于后续输出
  std::cout.rdbuf(coutbuf);

  // 初始化日志器
  mm::LoggerManager& logManager = mm::LoggerManager::instance();
  int result                    = logManager.setup(cArgs.size(), cArgs.data());
  if (result != 0) {
    std::cerr << "日志初始化失败" << std::endl;
    return 1;
  }

  logManager.setupLogger();
  logManager.Start();

  // 性能指标
  PerformanceMetrics metrics = {};

  // 获取系统信息
  std::string systemInfo = getSystemInfo();

  MM_INFO("开始性能测试: %s", config.testName.c_str());

  // 预热阶段
  if (config.warmupSeconds > 0) {
    if (config.verboseOutput) {
      std::cout << "预热阶段 (" << config.warmupSeconds << " 秒)..."
                << std::endl;
    }
    MM_INFO("预热阶段 (%d 秒)...", config.warmupSeconds);

    g_runBenchmark.store(true);
    std::vector<std::thread> warmupThreads;
    int warmupLogsPerThread = std::max(1000, config.logsPerThread / 10);

    // 修改配置以用于预热
    BenchmarkConfig warmupConfig = config;
    warmupConfig.logsPerThread   = warmupLogsPerThread;
    warmupConfig.measureLatency  = false;

    auto startWarmTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < config.numThreads; ++i) {
      warmupThreads.emplace_back(logGenerationThread, i, warmupConfig);
    }

    // 预热期间睡眠
    std::this_thread::sleep_for(std::chrono::seconds(config.warmupSeconds));

    // 停止预热
    g_runBenchmark.store(false);
    for (auto& thread : warmupThreads) {
      thread.join();
    }

    auto endWarmTime = std::chrono::high_resolution_clock::now();
    auto seconds =
        std::chrono::duration<double>(endWarmTime - startWarmTime).count();
    std::cout << "热身实际耗时:" << seconds << "s" << std::endl;

    // 清除计数器
    g_totalLogsGenerated.store(0);
    {
      std::lock_guard<std::mutex> lock(g_latencyMutex);
      g_latencies.clear();
    }
  }

  // 主测试阶段
  if (config.verboseOutput) {
    std::cout << "主测试阶段 (" << config.testDurationSeconds << " 秒)..."
              << std::endl;
  }
  MM_INFO("主测试阶段 (%d 秒)...", config.testDurationSeconds);

  // 设置性能监控
  std::string monitoredPath = config.enableFileOutput ? config.logFilePath : "";
  PerformanceMonitor monitor("/proc/self/io", monitoredPath);
  monitor.start();

  // 启动测试线程
  g_runBenchmark.store(true);
  std::vector<std::thread> benchmarkThreads;

  auto startTime = std::chrono::steady_clock::now();

  for (int i = 0; i < config.numThreads; ++i) {
    benchmarkThreads.emplace_back(logGenerationThread, i, config);
  }

  // 测试期间睡眠
  std::this_thread::sleep_for(std::chrono::seconds(config.testDurationSeconds));

  // 停止测试
  g_runBenchmark.store(false);
  auto endTime = std::chrono::steady_clock::now();

  for (auto& thread : benchmarkThreads) {
    thread.join();
  }

  // 停止性能监控
  monitor.stop();

  auto elapsedSeconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
          .count() /
      1000.0;
  std::cout << "主测试实际耗时:" << elapsedSeconds << "s" << std::endl;

  // 计算指标
  metrics.logsPerSecond =
      static_cast<double>(g_totalLogsGenerated) / elapsedSeconds;
  metrics.bytesPerSecond        = metrics.logsPerSecond * config.logMessageSize;
  metrics.cpuUsagePercent       = monitor.getCpuUsage();
  metrics.memoryUsageMB         = monitor.getMemoryUsage();
  metrics.diskWritesBytesPerSec = monitor.getDiskWriteRate();
  metrics.diskWritesMBPerSec =
      metrics.diskWritesBytesPerSec / (1024.0 * 1024.0);

  // 获取延迟百分位数
  if (config.measureLatency) {
    std::lock_guard<std::mutex> lock(g_latencyMutex);
    calculateLatencyPercentiles(g_latencies, metrics);
  }

  // 记录结果
  MM_INFO("测试完成: %s", config.testName.c_str());
  MM_INFO("吞吐量: %.2f 日志/秒", metrics.logsPerSecond);
  MM_INFO("平均延迟: %.3f 微秒", metrics.avgLatency);

  // 冷却阶段
  if (config.cooldownSeconds > 0) {
    if (config.verboseOutput) {
      std::cout << "冷却阶段 (" << config.cooldownSeconds << " 秒)..."
                << std::endl;
    }
    MM_INFO("冷却阶段 (%d 秒)...", config.cooldownSeconds);
    std::this_thread::sleep_for(std::chrono::seconds(config.cooldownSeconds));
  }

  // 获取日志器内部状态
  getLoggerInternalStats(metrics, logManager, config);

  // 清理
  logManager.teardown();

  // 恢复流
  std::cerr.rdbuf(cerrbuf);
  logFile.close();

  // 将结果写入CSV
  writeResultsToCSV(config, metrics, systemInfo);

  if (config.verboseOutput) {
    std::cout << "\n=============== 性能测试结果 ===============" << std::endl;
    std::cout << "测试名称: " << config.testName << std::endl;
    std::cout << "日志器类型: " << config.loggerType << std::endl;
    std::cout << "线程数: " << config.numThreads << std::endl;
    std::cout << "\n--- 吞吐量指标 ---" << std::endl;
    std::cout << "日志吞吐量: " << std::fixed << std::setprecision(2)
              << metrics.logsPerSecond << " 日志/秒" << std::endl;
    std::cout << "数据吞吐量: " << std::fixed << std::setprecision(2)
              << metrics.bytesPerSecond / (1024.0 * 1024.0) << " MB/秒"
              << std::endl;

    std::cout << "\n--- 延迟指标 (微秒) ---" << std::endl;
    std::cout << "平均延迟: " << std::fixed << std::setprecision(2)
              << metrics.avgLatency << std::endl;
    std::cout << "P50延迟: " << std::fixed << std::setprecision(2)
              << metrics.p50Latency << std::endl;
    std::cout << "P90延迟: " << std::fixed << std::setprecision(2)
              << metrics.p90Latency << std::endl;
    std::cout << "P95延迟: " << std::fixed << std::setprecision(2)
              << metrics.p95Latency << std::endl;
    std::cout << "P99延迟: " << std::fixed << std::setprecision(2)
              << metrics.p99Latency << std::endl;
    std::cout << "最大延迟: " << std::fixed << std::setprecision(2)
              << metrics.maxLatency << std::endl;

    std::cout << "\n--- 资源使用 ---" << std::endl;
    std::cout << "CPU使用率: " << std::fixed << std::setprecision(2)
              << metrics.cpuUsagePercent << "%" << std::endl;
    std::cout << "内存使用: " << std::fixed << std::setprecision(2)
              << metrics.memoryUsageMB << " MB" << std::endl;
    std::cout << "磁盘写入: " << std::fixed << std::setprecision(2)
              << metrics.diskWritesMBPerSec << " MB/秒" << std::endl;

    if (config.loggerType == "OptimizedGLog") {
      std::cout << "\n--- OptimizedGLog状态 ---" << std::endl;
      std::cout << "入队消息数: " << metrics.enqueuedCount << std::endl;
      std::cout << "处理消息数: " << metrics.processedCount << std::endl;
      std::cout << "丢弃消息数: " << metrics.droppedCount << std::endl;
      std::cout << "溢出次数: " << metrics.overflowCount << std::endl;
      std::cout << "队列利用率: " << std::fixed << std::setprecision(2)
                << (metrics.queueUtilization * 100.0) << "%" << std::endl;
    }

    std::cout << "\n--- 配置参数 ---" << std::endl;
    std::cout << "日志消息大小: " << config.logMessageSize << " 字节"
              << std::endl;
    std::cout << "日志速率限制: "
              << (config.logRatePerSecond > 0
                         ? std::to_string(config.logRatePerSecond) + " 日志/秒"
                         : "无限制")
              << std::endl;

    if (config.loggerType == "OptimizedGLog") {
      std::cout << "批处理大小: " << config.batchSize << std::endl;
      std::cout << "队列容量: " << config.queueCapacity << std::endl;
      std::cout << "工作线程数: " << config.numWorkers << std::endl;
      std::cout << "内存池大小: " << config.poolSize << std::endl;
    }

    std::cout << "\n结果已写入: " << config.outputFile << std::endl;
    std::cout << "===============================================\n"
              << std::endl;
  }

  return 0;
}
