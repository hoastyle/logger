# 1. 推荐参数配置

1. batchSize = 1200
2. queueCapacity = 200000
3. poolSize = 250000
4. msgBufferSize = 4096

```cpp
./your_app --sinktype=OptimizedGLog --toTerm=info --batchSize=1200 --queueCapacity=200000 --numWorkers=4 --poolSize=250000 --msgBufferSize=4096
```

## 1.1. 软硬件环境
CPU: 24核心 (8大核 + 8小核)
内存: 64GB总内存，日常占用14GB，可用约50GB
日志线程限制: 只能分配4个工作线程
业务线程: 50-100个线程
日志量: 20-30分钟产生1.8GB日志

## 1.2. 具体说明
1. 批处理大小 (batchSize=1200)

大幅增加批处理大小，从标准的500提升到1200
原因:

工作线程数量有限，每个线程需要处理更多日志
减少上下文切换和锁竞争
大批量处理能够更高效地利用系统I/O
优先使用大核心处理日志批次

2. 队列容量 (queueCapacity=200000)
显著提高队列容量，确保足够的缓冲区
原因:

处理线程少，需要更大的队列应对峰值
内存充裕，可以分配更多资源给队列
提供约80-100秒的日志缓冲容量
避免因处理线程少导致的日志丢失

3. 内存池大小 (poolSize=250000)
利用充裕内存资源，分配大型对象池
原因:

64GB内存中有约50GB可用，内存不是瓶颈
更大的对象池减少"溢出"风险
预分配足够的对象减少运行时分配开销
采用保守估计：队列容量的1.25倍

4. 消息缓冲区大小 (msgBufferSize=4096)
增加默认消息缓冲大小，从2048字节增加到4096字节
原因:

内存充裕，可以使用更大的消息缓冲区
减少长消息可能导致的截断
提高包含大量信息的错误日志完整性

## 1.3. 资源性能预期与监控
1. 内存使用估算
队列内存: 约200MB (假设平均日志500字节 × 200,000条 × 2倍内存开销)
对象池内存: 约300MB (包含对象元数据和预分配缓冲区)
总计: 约500MB，仅占可用内存的1%，非常安全

2. 性能预期与监控
在推荐配置下，系统应能够轻松处理 50 - 100 个业务线程产生的每半小时 1.8GB 日志数据。
观察性能指标，应满足:

* 低丢弃率: Dropped值应该接近0
* 低溢出率: Overflow值应该接近0
* CPU使用高效: 4个工作线程应主要在8个大核上运行
* 内存占用合理: 日志系统内存占用约500MB-1GB

如果性能不达预期，首先检查:

* 日志写入磁盘性能（可能是瓶颈）
* 业务线程与日志线程的CPU竞争情况
* 通过增加batchSize进一步提高每个工作线程效率

# 2. 其他可能的优化策略
## 2.1. CPU 亲合度
根据大小核的情况，设置 CPU 亲合度

```cpp
// 在OptimizedGlogLogger::setup()中添加
void OptimizedGlogLogger::setupThreadAffinity() {
  // 仅在启动工作线程后调用此函数
  for (size_t i = 0; i < workers_.size(); ++i) {
    // 将日志工作线程绑定到大核上
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    // 假设0-7是大核心ID，根据实际系统调整
    CPU_SET(i % 8, &cpuset);
    pthread_setaffinity_np(workers_[i].native_handle(), sizeof(cpu_set_t), &cpuset);
  }
}
```

## 2.2. 系统参数优化
1. TODO: 日志 I/O 优化
```bash
# 增加系统页缓存用于文件I/O
echo 80 > /proc/sys/vm/dirty_ratio
echo 10 > /proc/sys/vm/dirty_background_ratio
echo 3000 > /proc/sys/vm/dirty_expire_centisecs
```

2. TODO: 采用其他文件系统
如 XFS，推荐挂载选项:
`mount -o noatime,nodiratime,logbufs=8,logbsize=256k /dev/sdX /log/path`
