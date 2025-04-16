#!/usr/bin/env python3
"""
处理性能测试输出的工具模块
"""

import re
import os
import sys
from datetime import datetime

class OutputProcessor:
    """处理并合并性能测试的各种输出"""

    def __init__(self, log_path=None):
        self.log_path = log_path
        self.glog_metrics = {}
        self.benchmark_metrics = {}

    def parse_glog_output(self, content):
        """解析GLog格式的输出，提取关键性能指标"""
        # 提取时间戳格式的日志行
        timestamp_pattern = r'I\d{8} \d{2}:\d{2}:\d{2}\.\d+ \d+ :[^]]+\]\s+[^ ]+'
        metrics = {}

        # 查找与性能相关的指标
        throughput_match = re.search(r'吞吐量: ([\d.]+) 日志/秒', content)
        if throughput_match:
            metrics['glog_throughput'] = float(throughput_match.group(1))

        latency_match = re.search(r'平均延迟: ([\d.]+) 微秒', content)
        if latency_match:
            metrics['glog_avg_latency'] = float(latency_match.group(1))

        # 特别关注OptimizedGlogLogger stats
        glog_stats_match = re.search(r'OptimizedGlogLogger stats - Enqueued: (\d+), Processed: (\d+), Dropped: (\d+), Overflow: (\d+)', content)
        if glog_stats_match:
            metrics['glog_enqueued'] = int(glog_stats_match.group(1))
            metrics['glog_processed'] = int(glog_stats_match.group(2))
            metrics['glog_dropped'] = int(glog_stats_match.group(3))
            metrics['glog_overflow'] = int(glog_stats_match.group(4))

        # 提取其他可能的GLog指标...

        self.glog_metrics = metrics
        return metrics

    def parse_benchmark_output(self, content):
        """解析基准测试工具自身的输出"""
        metrics = {}

        # 查找性能测试结果块
        results_block_match = re.search(r'=============== 性能测试结果 ===============(.*?)===============================================', content, re.DOTALL)
        if results_block_match:
            results_block = results_block_match.group(1)

            # 从性能测试结果块中提取所有指标
            name_match = re.search(r'测试名称: (.*?)$', results_block, re.MULTILINE)
            if name_match:
                metrics['test_name'] = name_match.group(1).strip()

            logger_type_match = re.search(r'日志器类型: (.*?)$', results_block, re.MULTILINE)
            if logger_type_match:
                metrics['logger_type'] = logger_type_match.group(1).strip()

            threads_match = re.search(r'线程数: (\d+)', results_block)
            if threads_match:
                metrics['num_threads'] = int(threads_match.group(1))

            # 吞吐量指标
            throughput_match = re.search(r'日志吞吐量: ([\d.]+) 日志/秒', results_block)
            if throughput_match:
                metrics['throughput'] = float(throughput_match.group(1))

            data_throughput_match = re.search(r'数据吞吐量: ([\d.]+) MB/秒', results_block)
            if data_throughput_match:
                metrics['data_throughput_mb'] = float(data_throughput_match.group(1))

            # 延迟指标
            avg_latency_match = re.search(r'平均延迟: ([\d.]+)', results_block)
            if avg_latency_match:
                metrics['avg_latency'] = float(avg_latency_match.group(1))

            p50_match = re.search(r'P50延迟: ([\d.]+)', results_block)
            if p50_match:
                metrics['p50_latency'] = float(p50_match.group(1))

            p90_match = re.search(r'P90延迟: ([\d.]+)', results_block)
            if p90_match:
                metrics['p90_latency'] = float(p90_match.group(1))

            p95_match = re.search(r'P95延迟: ([\d.]+)', results_block)
            if p95_match:
                metrics['p95_latency'] = float(p95_match.group(1))

            p99_match = re.search(r'P99延迟: ([\d.]+)', results_block)
            if p99_match:
                metrics['p99_latency'] = float(p99_match.group(1))

            max_latency_match = re.search(r'最大延迟: ([\d.]+)', results_block)
            if max_latency_match:
                metrics['max_latency'] = float(max_latency_match.group(1))

            # 资源使用
            cpu_match = re.search(r'CPU使用率: ([\d.]+)%', results_block)
            if cpu_match:
                metrics['cpu_percent'] = float(cpu_match.group(1))

            memory_match = re.search(r'内存使用: ([\d.]+) MB', results_block)
            if memory_match:
                metrics['memory_mb'] = float(memory_match.group(1))

            disk_match = re.search(r'磁盘写入: ([\d.]+) MB/秒', results_block)
            if disk_match:
                metrics['disk_write_mb'] = float(disk_match.group(1))

            # OptimizedGLog状态
            if '--- OptimizedGLog状态 ---' in results_block:
                enqueued_match = re.search(r'入队消息数: (\d+)', results_block)
                if enqueued_match:
                    metrics['enqueued_count'] = int(enqueued_match.group(1))

                processed_match = re.search(r'处理消息数: (\d+)', results_block)
                if processed_match:
                    metrics['processed_count'] = int(processed_match.group(1))

                dropped_match = re.search(r'丢弃消息数: (\d+)', results_block)
                if dropped_match:
                    metrics['dropped_count'] = int(dropped_match.group(1))

                overflow_match = re.search(r'溢出次数: (\d+)', results_block)
                if overflow_match:
                    metrics['overflow_count'] = int(overflow_match.group(1))

                queue_util_match = re.search(r'队列利用率: ([\d.]+)%', results_block)
                if queue_util_match:
                    metrics['queue_utilization'] = float(queue_util_match.group(1))

            # 配置参数
            msg_size_match = re.search(r'日志消息大小: (\d+) 字节', results_block)
            if msg_size_match:
                metrics['msg_size'] = int(msg_size_match.group(1))

            batch_size_match = re.search(r'批处理大小: (\d+)', results_block)
            if batch_size_match:
                metrics['batch_size'] = int(batch_size_match.group(1))

            queue_capacity_match = re.search(r'队列容量: (\d+)', results_block)
            if queue_capacity_match:
                metrics['queue_capacity'] = int(queue_capacity_match.group(1))

            num_workers_match = re.search(r'工作线程数: (\d+)', results_block)
            if num_workers_match:
                metrics['num_workers'] = int(num_workers_match.group(1))

            pool_size_match = re.search(r'内存池大小: (\d+)', results_block)
            if pool_size_match:
                metrics['pool_size'] = int(pool_size_match.group(1))

        # 如果没有找到格式化的结果块，尝试提取单独的指标
        else:
            # 提取常见性能指标
            throughput_match = re.search(r'日志吞吐量: ([\d.]+) 日志/秒', content)
            if throughput_match:
                metrics['throughput'] = float(throughput_match.group(1))

            latency_match = re.search(r'平均延迟: ([\d.]+)', content)
            if latency_match:
                metrics['avg_latency'] = float(latency_match.group(1))

            p99_match = re.search(r'P99延迟: ([\d.]+)', content)
            if p99_match:
                metrics['p99_latency'] = float(p99_match.group(1))

            max_latency_match = re.search(r'最大延迟: ([\d.]+)', content)
            if max_latency_match:
                metrics['max_latency'] = float(max_latency_match.group(1))

            cpu_match = re.search(r'CPU使用率: ([\d.]+)%', content)
            if cpu_match:
                metrics['cpu_percent'] = float(cpu_match.group(1))

            memory_match = re.search(r'内存使用: ([\d.]+) MB', content)
            if memory_match:
                metrics['memory_mb'] = float(memory_match.group(1))

            disk_match = re.search(r'磁盘写入: ([\d.]+) MB/秒', content)
            if disk_match:
                metrics['disk_write_mb'] = float(disk_match.group(1))

        # 检查GLog输出部分中的OptimizedGlogLogger stats行
        glog_section_match = re.search(r'===== GLog输出 =====\n(.*)', content, re.DOTALL)
        if glog_section_match:
            glog_section = glog_section_match.group(1)
            glog_stats_match = re.search(r'OptimizedGlogLogger stats - Enqueued: (\d+), Processed: (\d+), Dropped: (\d+), Overflow: (\d+)', glog_section)
            if glog_stats_match:
                metrics['glog_enqueued'] = int(glog_stats_match.group(1))
                metrics['glog_processed'] = int(glog_stats_match.group(2))
                metrics['glog_dropped'] = int(glog_stats_match.group(3))
                metrics['glog_overflow'] = int(glog_stats_match.group(4))

        self.benchmark_metrics = metrics
        return metrics

    def process_test_output(self, log_file):
        """处理单个测试的输出文件，提取和合并指标"""
        if not os.path.exists(log_file):
            print(f"找不到日志文件: {log_file}")
            return {}

        with open(log_file, 'r') as f:
            content = f.read()

        # 解析两种格式的输出
        glog_metrics = self.parse_glog_output(content)
        benchmark_metrics = self.parse_benchmark_output(content)

        # 合并指标
        combined_metrics = {**benchmark_metrics, **glog_metrics}
        return combined_metrics

    def generate_unified_report(self, metrics, output_file=None):
        """生成统一的性能报告"""
        if not metrics:
            return "无性能数据可用"

        report = []
        report.append("\n================ 统一性能测试结果 ================")

        # 添加时间戳
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        report.append(f"生成时间: {now}")

        # 测试信息部分
        if 'test_name' in metrics:
            report.append(f"\n测试名称: {metrics['test_name']}")
        if 'logger_type' in metrics:
            report.append(f"日志器类型: {metrics['logger_type']}")
        if 'num_threads' in metrics:
            report.append(f"线程数: {metrics['num_threads']}")

        # 吞吐量部分
        report.append("\n--- 吞吐量指标 ---")
        if 'throughput' in metrics:
            report.append(f"日志吞吐量: {metrics['throughput']:.2f} 日志/秒")
        elif 'glog_throughput' in metrics:
            report.append(f"日志吞吐量: {metrics['glog_throughput']:.2f} 日志/秒")

        if 'data_throughput_mb' in metrics:
            report.append(f"数据吞吐量: {metrics['data_throughput_mb']:.2f} MB/秒")

        # 延迟部分
        report.append("\n--- 延迟指标 (微秒) ---")
        if 'avg_latency' in metrics:
            report.append(f"平均延迟: {metrics['avg_latency']:.2f}")
        elif 'glog_avg_latency' in metrics:
            report.append(f"平均延迟: {metrics['glog_avg_latency']:.2f}")

        if 'p50_latency' in metrics:
            report.append(f"P50延迟: {metrics['p50_latency']:.2f}")
        if 'p90_latency' in metrics:
            report.append(f"P90延迟: {metrics['p90_latency']:.2f}")
        if 'p95_latency' in metrics:
            report.append(f"P95延迟: {metrics['p95_latency']:.2f}")
        if 'p99_latency' in metrics:
            report.append(f"P99延迟: {metrics['p99_latency']:.2f}")
        if 'max_latency' in metrics:
            report.append(f"最大延迟: {metrics['max_latency']:.2f}")

        # 资源使用部分
        report.append("\n--- 资源使用 ---")
        if 'cpu_percent' in metrics:
            report.append(f"CPU使用率: {metrics['cpu_percent']:.2f}%")
        if 'memory_mb' in metrics:
            report.append(f"内存使用: {metrics['memory_mb']:.2f} MB")
        if 'disk_write_mb' in metrics:
            report.append(f"磁盘写入: {metrics['disk_write_mb']:.2f} MB/秒")

        # OptimizedGLog状态部分 - 优先使用从性能测试结果块中获取的数据
        has_glog_stats = any(k in metrics for k in ['enqueued_count', 'processed_count', 'dropped_count', 'overflow_count'])
        has_glog_output_stats = any(k in metrics for k in ['glog_enqueued', 'glog_processed', 'glog_dropped', 'glog_overflow'])

        if has_glog_stats:
            report.append("\n--- OptimizedGLog状态 ---")
            if 'enqueued_count' in metrics:
                report.append(f"入队消息数: {metrics['enqueued_count']}")
            if 'processed_count' in metrics:
                report.append(f"处理消息数: {metrics['processed_count']}")
            if 'dropped_count' in metrics:
                report.append(f"丢弃消息数: {metrics['dropped_count']}")
            if 'overflow_count' in metrics:
                report.append(f"溢出次数: {metrics['overflow_count']}")
            if 'queue_utilization' in metrics:
                report.append(f"队列利用率: {metrics['queue_utilization']:.2f}%")
        elif has_glog_output_stats:
            report.append("\n--- OptimizedGLog状态 (GLog输出) ---")
            if 'glog_enqueued' in metrics:
                report.append(f"入队消息数: {metrics['glog_enqueued']}")
            if 'glog_processed' in metrics:
                report.append(f"处理消息数: {metrics['glog_processed']}")
            if 'glog_dropped' in metrics:
                report.append(f"丢弃消息数: {metrics['glog_dropped']}")
            if 'glog_overflow' in metrics:
                report.append(f"溢出次数: {metrics['glog_overflow']}")

        # 配置参数部分
        report.append("\n--- 配置参数 ---")
        if 'msg_size' in metrics:
            report.append(f"日志消息大小: {metrics['msg_size']} 字节")
        if 'batch_size' in metrics:
            report.append(f"批处理大小: {metrics['batch_size']}")
        if 'queue_capacity' in metrics:
            report.append(f"队列容量: {metrics['queue_capacity']}")
        if 'num_workers' in metrics:
            report.append(f"工作线程数: {metrics['num_workers']}")
        if 'pool_size' in metrics:
            report.append(f"内存池大小: {metrics['pool_size']}")

        report.append("\n=================================================")

        result = "\n".join(report)

        # 写入文件（如果指定）
        if output_file:
            with open(output_file, 'w') as f:
                f.write(result)

        return result

    def process_and_generate(self, log_file, output_file=None):
        """处理日志文件并生成统一报告"""
        metrics = self.process_test_output(log_file)
        return self.generate_unified_report(metrics, output_file)

# 测试代码
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("使用方法: python output_processor.py <日志文件> [输出文件]")
        sys.exit(1)

    log_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None

    processor = OutputProcessor()
    report = processor.process_and_generate(log_file, output_file)
    print(report)
