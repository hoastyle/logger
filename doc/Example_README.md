# MMLogger 示例程序

本目录包含多个示例程序，展示如何使用 MMLogger 库的各种功能。

## 示例程序列表

1. **基本示例** (`mmlogger_example`)
   - 展示基本日志功能
   - 演示如何初始化和配置日志系统
   - 示范不同日志级别的用法

2. **高级示例** (`mmlogger_advanced_example`)
   - 演示如何进行编程方式的配置
   - 展示如何设置更复杂的日志选项

3. **OptimizedGLog 示例** (`mmlogger_optimized_glog_example`)
   - 演示高性能日志记录
   - 展示批处理和异步日志功能
   - 测试高并发日志性能

4. **完整功能示例** (`mmlogger_complete_example`) - 新增!
   - 演示所有支持的命令行参数
   - 提供详细用法提示和帮助信息
   - 包含多种演示模式：基本日志、多线程日志、速率限制日志和压力测试

## 编译和运行

### 使用 CMake 构建

```bash
# 在主项目目录创建构建目录
mkdir -p build && cd build

# 配置并构建
cmake .. -DMM_BUILD_EXAMPLES=ON
make

# 运行示例
./examples/mmlogger_example
./examples/mmlogger_advanced_example
./examples/mmlogger_optimized_glog_example
./examples/mmlogger_complete_example
```

### 运行完整功能示例

完整功能示例支持多种命令行参数和演示模式：

```bash
# 显示帮助信息
./examples/mmlogger_complete_example --help

# 运行带控制台输出的基本演示
./examples/mmlogger_complete_example --console=true --demo-mode=basic

# 运行多线程演示
./examples/mmlogger_complete_example --console=true --demo-mode=threads

# 运行 OptimizedGLog 的压力测试
./examples/mmlogger_complete_example --console=true --sinktype=OptimizedGLog --demo-mode=stress-test

# 运行所有演示
./examples/mmlogger_complete_example --console=true --demo-mode=all
```

## 支持的命令行参数

以下是完整功能示例支持的命令行参数：

| 参数                | 描述                                | 值示例                          |
|---------------------|-------------------------------------|--------------------------------|
| `--appid`           | 设置应用标识符                     | `--appid=MyApp`                |
| `--sinktype`        | 设置日志后端类型                   | `--sinktype=OptimizedGLog`     |
| `--console`         | 启用/禁用控制台输出                | `--console=true`               |
| `--toTerm`          | 设置控制台日志级别                 | `--toTerm=info`                |
| `--file`            | 启用/禁用文件日志                  | `--file=true`                  |
| `--filepath`        | 设置日志文件路径                   | `--filepath=./logs`            |
| `--toFile`          | 设置文件日志级别                   | `--toFile=warn`                |
| `--debugSwitch`     | 启用/禁用调试日志                  | `--debugSwitch=true`           |
| `--demo-mode`       | 设置演示模式                       | `--demo-mode=threads`          |

### OptimizedGLog 特有参数

| 参数                | 描述                                | 值示例                          |
|---------------------|-------------------------------------|--------------------------------|
| `--batchSize`       | 批处理大小                         | `--batchSize=200`              |
| `--queueCapacity`   | 队列容量                           | `--queueCapacity=20000`        |
| `--numWorkers`      | 工作线程数                         | `--numWorkers=4`               |
| `--poolSize`        | 内存池大小                         | `--poolSize=30000`             |
