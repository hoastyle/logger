#!/usr/bin/env python3
"""
MMLogger性能评估测试运行器

此脚本根据YAML配置文件执行性能测试，并收集结果。
"""

import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from utils.color_fix import ColorFormatter

import argparse
import os
import sys
import yaml
import subprocess
import time
import datetime
import csv
import logging
import platform
import shutil
import re
from utils.output_processor import OutputProcessor
from pathlib import Path

# 设置日志记录
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('benchmark_runner.log')
    ]
)
logger = logging.getLogger('benchmark_runner')

# 添加去除ANSI颜色代码的函数
def strip_ansi_colors(text):
    """移除字符串中的ANSI颜色代码"""
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)

def parse_args():
    """解析命令行参数"""
    parser = argparse.ArgumentParser(description='MMLogger性能评估测试运行器')
    parser.add_argument('--config', type=str, default='benchmarks/test_config.yaml',
                        help='配置文件路径')
    parser.add_argument('--test-suite', type=str, default=None,
                        help='要运行的测试套件名称 (默认: 运行所有)')
    parser.add_argument('--test-id', type=str, default=None,
                        help='要运行的单个测试ID (默认: 运行所有)')
    parser.add_argument('--repeat', type=int, default=1,
                        help='每个测试重复次数')
    parser.add_argument('--output-dir', type=str, default='results',
                        help='输出结果目录')
    parser.add_argument('--binary', type=str, default=None,
                        help='C++基准测试可执行文件路径 (默认: 自动检测)')
    parser.add_argument('--skip-compile', action='store_true',
                        help='跳过编译基准测试程序')
    parser.add_argument('--verbose', action='store_true',
                        help='启用详细输出')
    return parser.parse_args()

def find_benchmark_binary():
    """自动查找基准测试可执行文件"""
    # 首先检查常见位置
    possible_paths = [
        'build/benchmarks/benchmark',
        'build/benchmarks/mmlogger_benchmark',
        'build/bin/mmlogger_benchmark',
        'benchmarks/mmlogger_benchmark',
    ]

    # 添加平台特定扩展名
    if platform.system() == 'Windows':
        possible_paths.extend([p + '.exe' for p in possible_paths])

    for path in possible_paths:
        if os.path.isfile(path) and os.access(path, os.X_OK):
            logger.info(f"找到基准测试可执行文件: {path}")
            return path

    # 如果找不到，执行更广泛的搜索
    logger.info("在build目录中搜索基准测试可执行文件...")
    for root, dirs, files in os.walk("build"):
        for file in files:
            if file.startswith("mmlogger_benchmark") or file == "benchmark":
                if platform.system() == 'Windows' and not file.endswith('.exe'):
                    continue
                path = os.path.join(root, file)
                if os.access(path, os.X_OK):
                    logger.info(f"找到基准测试可执行文件: {path}")
                    return path

    logger.error("找不到基准测试可执行文件！请确保已构建基准测试程序。")
    return None

def load_config(config_path):
    """加载YAML配置文件"""
    try:
        with open(config_path, 'r') as f:
            return yaml.safe_load(f)
    except Exception as e:
        logger.error(f"无法加载配置文件 {config_path}: {e}")
        sys.exit(1)

def compile_benchmark(skip_compile=False):
    """编译基准测试程序"""
    if skip_compile:
        logger.info("跳过编译阶段")
        return True

    logger.info("编译基准测试程序...")

    # 检查是否存在构建脚本
    if os.path.exists('./build_benchmark.sh') and platform.system() != 'Windows':
        try:
            subprocess.run(['bash', './build_benchmark.sh'], check=True)
            logger.info("使用 build_benchmark.sh 编译成功")
            return True
        except subprocess.CalledProcessError as e:
            logger.error(f"使用 build_benchmark.sh 编译失败: {e}")
            # 回退到标准 CMake 构建

    # 使用标准 CMake 构建
    try:
        build_dir = Path('./build')
        if not build_dir.exists():
            build_dir.mkdir()

        # 切换到构建目录并执行cmake
        os.chdir(build_dir)
        cmake_cmd = ['cmake', '..', '-DMM_BUILD_BENCHMARKS=ON']

        # 如果是Windows，添加生成器
        if platform.system() == 'Windows':
            generator = 'Visual Studio 16 2019'  # 可以根据需要更改
            cmake_cmd.extend(['-G', generator])

        subprocess.run(cmake_cmd, check=True)

        # 构建
        if platform.system() == 'Windows':
            subprocess.run(['cmake', '--build', '.', '--config', 'Release'], check=True)
        else:
            cpu_count = os.cpu_count() or 4
            subprocess.run(['make', f'-j{cpu_count}'], check=True)

        os.chdir('..')

        logger.info("编译成功")
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"编译失败: {e}")
        os.chdir('..')  # 确保返回到原始目录
        return False

def run_test(binary, test_config, output_file, verbose=False):
    """运行单个测试"""
    # 准备环境变量
    env = os.environ.copy()

    # 设置所有测试参数为环境变量
    for key, value in test_config.items():
        # 跳过描述
        if key == 'description':
            continue

        env_key = key.upper()  # 转换为大写
        env[env_key] = str(value)

    # 确保输出目录存在
    output_dir = os.path.dirname(output_file)
    os.makedirs(output_dir, exist_ok=True)

    # 设置日志目录
    log_path = test_config.get('log_file_path', './logs')
    os.makedirs(log_path, exist_ok=True)

    # 设置输出文件
    env['OUTPUT_FILE'] = output_file

    # 启动测试进程
    logger.info(f"运行测试 '{test_config['name']}' (ID: {test_config['id']})...")

    try:
        # 对于所有情况，我们都显示输出到控制台，并也捕获到日志文件
        test_log_file = f"{output_dir}/{test_config['id']}.log"

        # 重定向stderr到临时文件，以便捕获OptimizedGlogLogger stats
        benchmark_log_filename = "benchmark_log.txt"
        with open(benchmark_log_filename, 'w') as stderr_file:
            # 运行基准测试，同时显示输出到终端并记录到日志文件
            process = subprocess.Popen(
                [binary],
                env=env,
                stdout=subprocess.PIPE,
                stderr=stderr_file,
                text=True,
                bufsize=1
            )

            # 实时收集和显示stdout，同时写入日志文件（去除颜色代码）
            with open(test_log_file, 'w') as log_file:
                for line in process.stdout:
                    print(line, end='')  # 显示到控制台（保留颜色）
                    log_file.write(strip_ansi_colors(line))  # 写入日志文件（去除颜色代码）

            process.stdout.close()
            return_code = process.wait()

            if return_code != 0:
                logger.error(f"测试进程返回错误代码: {return_code}")
                return False

            # 将stderr内容附加到测试日志文件
            if os.path.exists(benchmark_log_filename):
                with open(benchmark_log_filename, 'r') as stderr_source, open(test_log_file, 'a') as log_dest:
                    log_dest.write("\n===== GLog输出 =====\n")
                    glog_content = stderr_source.read()
                    log_dest.write(strip_ansi_colors(glog_content))  # 写入日志文件（去除颜色代码）

        logger.info(f"测试 '{test_config['id']}' 成功完成")
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"测试 '{test_config['id']}' 失败: {e}")
        return False
    except Exception as e:
        logger.error(f"运行测试时发生错误: {e}")
        return False

def summarize_results(output_file):
    """汇总测试结果，生成简单的统计信息"""
    try:
        with open(output_file, 'r') as f:
            reader = csv.DictReader(f)
            rows = list(reader)

        if not rows:
            logger.warning(f"结果文件 {output_file} 中没有数据")
            return

        # 按测试ID分组
        results_by_id = {}
        for row in rows:
            test_id = row['TestID']
            if test_id not in results_by_id:
                results_by_id[test_id] = []
            results_by_id[test_id].append(row)

        # 计算每个测试的摘要统计
        summary_file = f"{os.path.splitext(output_file)[0]}_summary.csv"
        with open(summary_file, 'w', newline='') as f:
            # 创建摘要列
            fieldnames = ['TestID', 'TestName', 'LoggerType', 'Runs',
                          'AvgLogsPerSecond', 'MaxLogsPerSecond', 'MinLogsPerSecond',
                          'AvgLatencyUs', 'P99LatencyUs', 'MaxLatencyUs',
                          'AvgCPUPercent', 'AvgMemoryMB', 'DiskWritesMBs']
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()

            # 计算每个测试的统计数据
            for test_id, results in results_by_id.items():
                if not results:
                    continue

                num_runs = len(results)
                test_name = results[0]['TestName']
                logger_type = results[0]['LoggerType']

                # 计算统计信息
                logs_per_second = [float(r.get('LogsPerSecond', 0)) for r in results]
                latency = [float(r.get('AvgLatencyUs', 0)) for r in results]
                p99_latency = [float(r.get('P99LatencyUs', 0)) for r in results]
                max_latency = [float(r.get('MaxLatencyUs', 0)) for r in results]
                cpu_percent = [float(r.get('CPUPercent', 0)) for r in results]
                memory_mb = [float(r.get('MemoryMB', 0)) for r in results]
                disk_writes = [float(r.get('DiskWritesMBs', 0)) for r in results]

                # 防止除零错误
                avg_logs = sum(logs_per_second) / num_runs if logs_per_second else 0
                avg_latency = sum(latency) / num_runs if latency else 0
                avg_p99 = sum(p99_latency) / num_runs if p99_latency else 0
                avg_cpu = sum(cpu_percent) / num_runs if cpu_percent else 0
                avg_memory = sum(memory_mb) / num_runs if memory_mb else 0
                avg_disk = sum(disk_writes) / num_runs if disk_writes else 0

                summary = {
                    'TestID': test_id,
                    'TestName': test_name,
                    'LoggerType': logger_type,
                    'Runs': num_runs,
                    'AvgLogsPerSecond': avg_logs,
                    'MaxLogsPerSecond': max(logs_per_second) if logs_per_second else 0,
                    'MinLogsPerSecond': min(logs_per_second) if logs_per_second else 0,
                    'AvgLatencyUs': avg_latency,
                    'P99LatencyUs': avg_p99,
                    'MaxLatencyUs': max(max_latency) if max_latency else 0,
                    'AvgCPUPercent': avg_cpu,
                    'AvgMemoryMB': avg_memory,
                    'DiskWritesMBs': avg_disk
                }

                writer.writerow(summary)

        logger.info(f"摘要统计已写入 {summary_file}")

        # 创建CSV副本为Excel友好格式（添加BOM）
        excel_summary_file = f"{os.path.splitext(output_file)[0]}_summary_excel.csv"
        with open(summary_file, 'r', newline='') as src, \
             open(excel_summary_file, 'w', newline='', encoding='utf-8-sig') as dst:
            dst.write(src.read())

        logger.info(f"Excel友好的摘要已写入 {excel_summary_file}")

    except Exception as e:
        logger.error(f"无法生成测试摘要: {e}")

def generate_unified_reports(test_results):
    """为每个测试生成统一报告"""
    try:
        print("生成统一性能报告...")
        for result in test_results:
            log_file = result["output_file"].replace(".csv", ".log")
            output_file = result["output_file"].replace(".csv", "_unified_report.txt")
            if os.path.exists(log_file):
                processor = OutputProcessor()
                processor.process_and_generate(log_file, output_file)
                logger.info(f"已生成统一报告: {output_file}")
    except Exception as e:
        logger.error(f"生成统一报告时出错: {e}")

def main():
    """主函数"""
    args = parse_args()

    # 加载配置
    config = load_config(args.config)

    # 编译基准测试
    if not args.skip_compile and not compile_benchmark():
        sys.exit(1)

    # 查找基准测试可执行文件
    benchmark_binary = args.binary if args.binary else find_benchmark_binary()
    if not benchmark_binary:
        logger.error("找不到基准测试可执行文件，无法继续执行")
        sys.exit(1)

    # 确保基准测试可执行文件存在且可执行
    if not os.path.isfile(benchmark_binary) or not os.access(benchmark_binary, os.X_OK):
        logger.error(f"指定的基准测试可执行文件 {benchmark_binary} 不存在或不可执行")
        sys.exit(1)

    # 创建输出目录
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    output_base_dir = Path(args.output_dir) / timestamp
    output_base_dir.mkdir(parents=True, exist_ok=True)

    # 拷贝配置文件到输出目录以便参考
    config_copy_path = output_base_dir / "test_config_used.yaml"
    with open(args.config, 'r') as src, open(config_copy_path, 'w') as dst:
        dst.write(src.read())

    # 确定要运行的测试套件和测试
    test_suites_to_run = []
    if args.test_suite:
        # 只运行指定的测试套件
        for suite in config['test_suites']:
            if suite['name'] == args.test_suite:
                test_suites_to_run.append(suite)
                break
        if not test_suites_to_run:
            logger.error(f"找不到测试套件: {args.test_suite}")
            sys.exit(1)
    else:
        # 运行所有测试套件
        test_suites_to_run = config['test_suites']

    # 运行所有测试
    test_results = []
    for suite in test_suites_to_run:
        suite_name = suite['name']
        suite_dir = output_base_dir / suite_name.replace(' ', '_')
        suite_dir.mkdir(exist_ok=True)

        logger.info(f"运行测试套件: {suite_name}")
        logger.info(f"描述: {suite.get('description', '无描述')}")

        for test in suite['tests']:
            # 如果指定了特定测试ID，跳过所有不匹配的测试
            if args.test_id and test['id'] != args.test_id:
                continue

            # 所有测试配置
            global_config = config.get('global', {})
            full_config = {}

            # 先应用全局配置
            for key, value in global_config.items():
                full_config[key] = value

            # 再应用测试特定配置（覆盖全局配置）
            for key, value in test.items():
                full_config[key] = value

            # 运行此测试（可重复）
            for run in range(args.repeat):
                run_suffix = f"_run_{run+1}" if args.repeat > 1 else ""
                output_file = suite_dir / f"{test['id']}{run_suffix}.csv"

                # 执行测试
                success = run_test(benchmark_binary, full_config, str(output_file), args.verbose)
                if success:
                    test_results.append({
                        'suite': suite_name,
                        'test_id': test['id'],
                        'run': run + 1,
                        'output_file': str(output_file)
                    })

    # 所有测试完成后，生成汇总报告
    if test_results:
        # 为每个测试套件生成摘要
        logger.info("生成测试结果摘要...")

        # 收集所有结果到一个合并的CSV
        merged_output = output_base_dir / "all_results.csv"
        first_file = True

        with open(merged_output, 'w', newline='') as merged_file:
            for result in test_results:
                try:
                    with open(result['output_file'], 'r', newline='') as f:
                        # 读取CSV
                        if first_file:
                            # 包括标题行
                            merged_file.write(f.read())
                            first_file = False
                        else:
                            # 跳过标题行
                            next(f)  # 跳过标题
                            merged_file.write(f.read())
                except Exception as e:
                    logger.error(f"处理文件 {result['output_file']} 时出错: {e}")

        # 生成汇总统计
        summarize_results(str(merged_output))

        # 生成统一报告
        generate_unified_reports(test_results)

        logger.info(f"所有测试完成。结果已保存到 {output_base_dir}")
    else:
        logger.warning("没有成功完成的测试")

if __name__ == "__main__":
    main()
