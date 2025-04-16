# MMLogger性能评估框架

这是一个用于评估MMLogger库性能的综合测试框架，结合了C++、Python和YAML的优势，提供了灵活、可配置的性能测试环境。

## 功能特点

- **灵活的配置**: 通过YAML配置文件定义测试场景，无需修改源代码
- **全面的性能指标**: 测量吞吐量、延迟、CPU使用率、内存占用和磁盘I/O
- **多维度测试**: 可测试线程数量、日志大小、输出目标等因素的影响
- **自动结果收集**: 将测试结果以标准CSV格式输出，便于后续分析
- **测试重复与汇总**: 支持多次重复测试以获取稳定结果，并自动生成统计摘要

## 系统要求

- C++17兼容的编译器 (GCC 7+, Clang 5+)
- Python 3.6+
- CMake 3.14+
- Google glog库
- PyYAML

## 目录结构

```
mmlogger-benchmark/
├── benchmarks/               # 性能测试C++源代码
│   ├── CMakeLists.txt
│   └── mmlogger_benchmark.cpp
├── build/                    # 构建输出目录
├── include/                  # MMLogger头文件
├── results/                  # 测试结果输出目录
├── src/                      # MMLogger实现源码
├── CMakeLists.txt            # 主CMake文件
├── Makefile                  # 构建和运行测试的Makefile
├── README.md                 # 本文档
├── run_benchmarks.py         # Python测试运行器
└── test_config.yaml          # 测试配置文件
```

## 快速入门

### 安装依赖项

```bash
# 安装Python依赖
pip install pyyaml

# 安装系统依赖
sudo apt-get install build-essential cmake libgoogle-glog-dev
```

### 构建并运行测试

使用提供的Makefile可以轻松构建和运行测试:

```bash
# 构建并运行所有测试
make

# 运行特定测试套件
make benchmark-suite SUITE="吞吐量测试"

# 运行特定测试
make benchmark-test TEST_ID=throughput_optimized

# 重复运行测试
make benchmark-repeat REPEAT=3
```

### 查看结果

测试结果将保存在`results/`目录下，按时间戳分组:

```
results/
└── 20250416_123456/          # 测试执行时间戳
    ├── 吞吐量测试/            # 测试套件
    │   ├── throughput_glog.csv
    │   ├── throughput_optimized.csv
    │   └── throughput_stdout.csv
    ├── all_results.csv       # 所有测试结果合并
    ├── all_results_summary.csv # 测试结果统计摘要
    └── test_config_used.yaml   # 测试使用的配置副本
```

## 配置测试

所有测试都在`test_config.yaml`文件中定义。您可以修改此文件以添加或调整测试场景。配置文件的结构如下:

```yaml
global:
  # 全局设置，适用于所有测试
  output_file: "benchmark_results.csv"
  append_output: true
  verbose_output: true

test_suites:
  - name: "测试套件名称"
    description: "套件描述"
    tests:
      - id: "测试ID"
        name: "测试名称"
        # 测试参数...
```

### 基本参数

每个测试都支持以下基本参数:

| 参数 | 描述 | 默认值 |
|------|------|--------|
| `logger_type` | 日志器类型 (Stdout, GLog, OptimizedGLog) | OptimizedGLog |
| `enable_console` | 是否启用控制台输出 | true |
| `enable_file` | 是否启用文件输出 | false |
| `log_file_path` | 日志文件路径 | ./logs |
| `log_level` | 日志级别 | info |
| `num_threads` | 生成日志的线程数 | 4 |
| `logs_per_thread` | 每个线程生成的日志条数 | 100000 |
| `log_msg_size` | 日志消息大小(字节) | 128 |
| `log_rate` | 每秒日志生成速率(0表示尽可能快) | 0 |
| `test_duration` | 测试持续时间(秒) | 10 |

### 高级参数

对于OptimizedGLog，还支持以下高级参数:

| 参数 | 描述 | 默认值 |
|------|------|--------|
| `batch_size` | 批处理大小 | 200 |
| `queue_capacity` | 队列容量 | 10000 |
| `num_workers` | 工作线程数 | 4 |
| `pool_size` | 内存池大小 | 20000 |

## 自定义测试

您可以根据需要向配置文件添加新的测试套件和测试。例如，添加一个测试不同CPU核心数的测试套件:

```yaml
- name: "CPU核心测试"
  description: "测试不同CPU核心数的影响"
  tests:
    - id: "cpu_2cores"
      name: "2核CPU"
      # 测试参数...
      
    - id: "cpu_4cores"
      name: "4核CPU"
      # 测试参数...
```

## 输出格式

性能测试的结果以CSV格式输出，包含以下列:

- `Timestamp`: 测试执行时间戳
- `TestID`, `TestName`: 测试标识和名称
- 配置参数: `LoggerType`, `NumThreads`等
- 性能指标:
  - `LogsPerSecond`: 每秒日志吞吐量
  - `BytesPerSecond`: 每秒数据吞吐量
  - `AvgLatencyUs`: 平均延迟(微秒)
  - `P50LatencyUs`, `P90LatencyUs`, `P99LatencyUs`: 百分位延迟
  - `MaxLatencyUs`: 最大延迟
  - `CPUPercent`: CPU使用率
  - `MemoryMB`: 内存使用(MB)
  - `DiskWritesBps`: 磁盘写入速度(字节/秒)

## 扩展框架

如果您想要扩展测试框架:

1. **添加新的性能指标**: 修改`mmlogger_benchmark.cpp`中的`PerformanceMetrics`结构并更新测量代码
2. **支持新的日志库**: 需要在基准测试程序中添加支持
3. **增加更多测试维度**: 更新YAML配置格式和Python运行器

## 贡献

欢迎通过以下方式贡献:
- 提交bug报告和功能请求
- 提交改进建议和文档更新
- 提交代码改进和新功能

## 许可证

此项目基于同样的许可条款作为MMLogger库。
