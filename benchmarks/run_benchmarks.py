#!/usr/bin/env python3
"""
MMLogger Benchmark Runner - Improved Version

This script runs performance benchmarks for the MMLogger library based on YAML configurations.
It ensures proper parameter mapping between the YAML config and C++ implementation.
"""

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
from pathlib import Path

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('benchmark_runner.log')
    ]
)
logger = logging.getLogger('benchmark_runner')

# Colors for terminal output
class Colors:
    GREEN = '\033[0;32m' if sys.stdout.isatty() else ''
    YELLOW = '\033[1;33m' if sys.stdout.isatty() else ''
    RED = '\033[0;31m' if sys.stdout.isatty() else ''
    BLUE = '\033[0;34m' if sys.stdout.isatty() else ''
    NC = '\033[0m' if sys.stdout.isatty() else ''  # No color

def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(description='MMLogger Performance Benchmark Runner')
    parser.add_argument('--config', type=str, default='benchmarks/test_config.yaml',
                        help='Configuration file path')
    parser.add_argument('--test-suite', type=str, default=None,
                        help='Name of the test suite to run (default: all)')
    parser.add_argument('--test-id', type=str, default=None,
                        help='ID of a specific test to run (default: all)')
    parser.add_argument('--repeat', type=int, default=1,
                        help='Number of times to repeat each test')
    parser.add_argument('--output-dir', type=str, default='results',
                        help='Directory for output results')
    parser.add_argument('--binary', type=str, default=None,
                        help='Path to the C++ benchmark executable (default: auto-detect)')
    parser.add_argument('--skip-compile', action='store_true',
                        help='Skip compiling the benchmark program')
    parser.add_argument('--verbose', action='store_true',
                        help='Enable verbose output')
    return parser.parse_args()

def find_benchmark_binary():
    """Find the benchmark executable"""
    possible_paths = [
        'build/benchmarks/benchmark',
        'build/benchmarks/mmlogger_benchmark',
        'build/bin/mmlogger_benchmark',
        'benchmarks/mmlogger_benchmark',
        './benchmark'
    ]

    # Add platform-specific extensions
    if platform.system() == 'Windows':
        possible_paths.extend([p + '.exe' for p in possible_paths])

    # Check each path
    for path in possible_paths:
        if os.path.isfile(path) and os.access(path, os.X_OK):
            logger.info(f"Found benchmark executable: {path}")
            return path

    # Broader search
    logger.info("Searching for benchmark executable in build directory...")
    for root, dirs, files in os.walk("build"):
        for file in files:
            if file.startswith("mmlogger_benchmark") or file == "benchmark":
                if platform.system() == 'Windows' and not file.endswith('.exe'):
                    continue
                path = os.path.join(root, file)
                if os.access(path, os.X_OK):
                    logger.info(f"Found benchmark executable: {path}")
                    return path

    logger.error("Benchmark executable not found! Make sure it's built.")
    return None

def load_yaml_config(config_path):
    """Load YAML configuration file"""
    try:
        with open(config_path, 'r') as f:
            return yaml.safe_load(f)
    except Exception as e:
        logger.error(f"Failed to load config file {config_path}: {e}")
        sys.exit(1)

def compile_benchmark(skip_compile=False):
    """Compile the benchmark program"""
    if skip_compile:
        logger.info("Skipping compilation phase")
        return True

    logger.info("Compiling benchmark program...")

    # Try using build_benchmark.sh if available
    if os.path.exists('./build_benchmark.sh') and platform.system() != 'Windows':
        try:
            subprocess.run(['bash', './build_benchmark.sh'], check=True)
            logger.info("Compilation successful using build_benchmark.sh")
            return True
        except subprocess.CalledProcessError as e:
            logger.error(f"Compilation failed with build_benchmark.sh: {e}")
            # Fall back to standard CMake build

    # Standard CMake build
    try:
        build_dir = Path('./build')
        if not build_dir.exists():
            build_dir.mkdir()

        # Configure with CMake
        os.chdir(build_dir)
        cmake_cmd = ['cmake', '..', '-DMM_BUILD_BENCHMARKS=ON']

        # Add generator on Windows
        if platform.system() == 'Windows':
            cmake_cmd.extend(['-G', 'Visual Studio 16 2019'])

        subprocess.run(cmake_cmd, check=True)

        # Build
        if platform.system() == 'Windows':
            subprocess.run(['cmake', '--build', '.', '--config', 'Release'], check=True)
        else:
            cpu_count = os.cpu_count() or 4
            subprocess.run(['make', f'-j{cpu_count}'], check=True)

        os.chdir('..')
        logger.info("Compilation successful")
        return True

    except subprocess.CalledProcessError as e:
        logger.error(f"Compilation failed: {e}")
        os.chdir('..')  # Always return to original directory
        return False

def prepare_environment_vars(test_config):
    """Convert YAML test configuration to environment variables"""
    env = os.environ.copy()

    # Map YAML keys to environment variable names
    # This mapping ensures consistency between YAML and C++
    key_mapping = {
        'id': 'TEST_ID',
        'name': 'TEST_NAME',
        'logger_type': 'LOGGER_TYPE',
        'enable_console': 'ENABLE_CONSOLE',
        'enable_file': 'ENABLE_FILE',
        'log_file_path': 'LOG_FILE_PATH',
        'log_level': 'LOG_LEVEL',
        'num_threads': 'NUM_THREADS',
        'logs_per_thread': 'LOGS_PER_THREAD',
        'log_msg_size': 'LOG_MSG_SIZE',
        'log_rate': 'LOG_RATE',
        'debug_pct': 'DEBUG_PCT',
        'info_pct': 'INFO_PCT',
        'warn_pct': 'WARN_PCT',
        'error_pct': 'ERROR_PCT',
        'batch_size': 'BATCH_SIZE',
        'queue_capacity': 'QUEUE_CAPACITY',
        'num_workers': 'NUM_WORKERS',
        'pool_size': 'POOL_SIZE',
        'warmup_seconds': 'WARMUP_SECONDS',
        'test_duration': 'TEST_DURATION',
        'cooldown_seconds': 'COOLDOWN_SECONDS',
        'measure_latency': 'MEASURE_LATENCY',
        'use_rate_limit': 'USE_RATE_LIMIT'
    }

    # Apply each configuration parameter
    for key, value in test_config.items():
        # Skip 'description' field
        if key == 'description':
            continue

        # Map the key to environment variable name
        env_key = key_mapping.get(key, key.upper())

        # Convert value to appropriate string format
        if isinstance(value, bool):
            # Boolean values need to be "true" or "false" strings
            env_value = "true" if value else "false"
        else:
            # Convert everything else to string
            env_value = str(value)

        # Set environment variable
        env[env_key] = env_value

    return env

def run_test(binary, test_config, output_file, verbose=False):
    """Run a single benchmark test"""
    # Prepare environment variables
    env = prepare_environment_vars(test_config)

    # Set output file
    env['OUTPUT_FILE'] = output_file

    # Ensure output directory exists
    output_dir = os.path.dirname(output_file)
    os.makedirs(output_dir, exist_ok=True)

    # Ensure log directory exists
    log_path = test_config.get('log_file_path', './logs')
    os.makedirs(log_path, exist_ok=True)

    # Print test information
    logger.info(f"Running test '{test_config['name']}' (ID: {test_config['id']})...")
    if verbose:
        print(f"{Colors.GREEN}Running test:{Colors.NC} {test_config['name']}")
        print(f"  Logger Type: {test_config['logger_type']}")
        print(f"  Threads: {test_config.get('num_threads', 4)}")
        print(f"  Duration: {test_config.get('test_duration', 10)} seconds")

    try:
        # Capture all output for logging
        test_log_file = f"{output_dir}/{test_config['id']}.log"

        # Benchmark process with stderr captured to a separate file
        benchmark_log_filename = f"{output_dir}/{test_config['id']}_stderr.log"
        with open(benchmark_log_filename, 'w') as stderr_file:
            # Run the benchmark process
            process = subprocess.Popen(
                [binary],
                env=env,
                stdout=subprocess.PIPE,
                stderr=stderr_file,
                text=True,
                bufsize=1
            )

            # Collect and display stdout, write to log file
            with open(test_log_file, 'w') as log_file:
                for line in process.stdout:
                    if verbose:
                        print(line, end='')  # Show to console with colors
                    # Write to log file (strip ANSI color codes)
                    log_file.write(re.sub(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])', '', line))

            process.stdout.close()
            return_code = process.wait()

            if return_code != 0:
                logger.error(f"Test process returned error code: {return_code}")
                return False

            # Append stderr content to test log
            with open(benchmark_log_filename, 'r') as stderr_source, \
                 open(test_log_file, 'a') as log_dest:
                log_dest.write("\n===== GLog Output =====\n")
                stderr_content = stderr_source.read()
                log_dest.write(re.sub(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])', '', stderr_content))

        logger.info(f"Test '{test_config['id']}' completed successfully")
        return True

    except Exception as e:
        logger.error(f"Error running test: {e}")
        return False

def summarize_results(output_file):
    """Generate summary statistics from test results"""
    try:
        # Check if file exists
        if not os.path.exists(output_file):
            logger.warning(f"Results file {output_file} not found")
            return

        # Read results
        with open(output_file, 'r') as f:
            reader = csv.DictReader(f)
            rows = list(reader)

        if not rows:
            logger.warning(f"No data in results file {output_file}")
            return

        # Group by test ID
        results_by_id = {}
        for row in rows:
            test_id = row['TestID']
            if test_id not in results_by_id:
                results_by_id[test_id] = []
            results_by_id[test_id].append(row)

        # Create summary file
        summary_file = f"{os.path.splitext(output_file)[0]}_summary.csv"
        with open(summary_file, 'w', newline='') as f:
            # Define summary columns
            fieldnames = [
                'TestID', 'TestName', 'LoggerType', 'Runs',
                'AvgLogsPerSecond', 'MaxLogsPerSecond', 'MinLogsPerSecond',
                'AvgLatencyUs', 'P99LatencyUs', 'MaxLatencyUs',
                'AvgCPUPercent', 'AvgMemoryMB', 'DiskWritesMBs'
            ]
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()

            # Calculate statistics for each test
            for test_id, results in results_by_id.items():
                if not results:
                    continue

                # Basic info
                num_runs = len(results)
                test_name = results[0]['TestName']
                logger_type = results[0]['LoggerType']

                # Calculate metrics
                logs_per_second = [float(r.get('LogsPerSecond', 0)) for r in results]
                latency = [float(r.get('AvgLatencyUs', 0)) for r in results]
                p99_latency = [float(r.get('P99LatencyUs', 0)) for r in results]
                max_latency = [float(r.get('MaxLatencyUs', 0)) for r in results]
                cpu_percent = [float(r.get('CPUPercent', 0)) for r in results]
                memory_mb = [float(r.get('MemoryMB', 0)) for r in results]
                disk_writes = [float(r.get('DiskWritesMBs', 0)) for r in results]

                # Avoid division by zero
                avg_logs = sum(logs_per_second) / num_runs if logs_per_second and num_runs > 0 else 0
                avg_latency = sum(latency) / num_runs if latency and num_runs > 0 else 0
                avg_p99 = sum(p99_latency) / num_runs if p99_latency and num_runs > 0 else 0
                avg_cpu = sum(cpu_percent) / num_runs if cpu_percent and num_runs > 0 else 0
                avg_memory = sum(memory_mb) / num_runs if memory_mb and num_runs > 0 else 0
                avg_disk = sum(disk_writes) / num_runs if disk_writes and num_runs > 0 else 0

                # Create summary row
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

        logger.info(f"Summary statistics written to {summary_file}")

        # Create Excel-friendly version with BOM for UTF-8
        excel_summary_file = f"{os.path.splitext(output_file)[0]}_summary_excel.csv"
        with open(summary_file, 'r', newline='') as src, \
             open(excel_summary_file, 'w', newline='', encoding='utf-8-sig') as dst:
            dst.write(src.read())

        logger.info(f"Excel-friendly summary written to {excel_summary_file}")

    except Exception as e:
        logger.error(f"Error generating summary: {e}")

def process_test_results(output_files):
    """Process the test results to generate unified reports"""
    try:
        logger.info("Processing test results...")

        # For each output file, process and generate unified report
        for output_file in output_files:
            log_file = output_file.replace(".csv", ".log")
            if not os.path.exists(log_file):
                continue

            # Extract key metrics from log file
            metrics = {}
            with open(log_file, 'r') as f:
                content = f.read()

                # Extract throughput
                throughput_match = re.search(r'日志吞吐量: ([\d.]+) 日志/秒', content)
                if throughput_match:
                    metrics['throughput'] = float(throughput_match.group(1))

                # Extract latency
                avg_latency_match = re.search(r'平均延迟: ([\d.]+)', content)
                if avg_latency_match:
                    metrics['avg_latency'] = float(avg_latency_match.group(1))

                # Extract CPU usage
                cpu_match = re.search(r'CPU使用率: ([\d.]+)%', content)
                if cpu_match:
                    metrics['cpu_percent'] = float(cpu_match.group(1))

                # Extract memory usage
                memory_match = re.search(r'内存使用: ([\d.]+) MB', content)
                if memory_match:
                    metrics['memory_mb'] = float(memory_match.group(1))

                # Extract queue stats if possible
                enqueued_match = re.search(r'入队消息数: (\d+)', content)
                if enqueued_match:
                    metrics['enqueued_count'] = int(enqueued_match.group(1))

                processed_match = re.search(r'处理消息数: (\d+)', content)
                if processed_match:
                    metrics['processed_count'] = int(processed_match.group(1))

                # Look for GLog output section
                glog_stats_match = re.search(
                    r'OptimizedGlogLogger stats - Enqueued: (\d+), Processed: (\d+), Dropped: (\d+), Overflow: (\d+)',
                    content
                )
                if glog_stats_match:
                    metrics['glog_enqueued'] = int(glog_stats_match.group(1))
                    metrics['glog_processed'] = int(glog_stats_match.group(2))
                    metrics['glog_dropped'] = int(glog_stats_match.group(3))
                    metrics['glog_overflow'] = int(glog_stats_match.group(4))

            # Write unified report
            report_file = output_file.replace(".csv", "_report.txt")
            with open(report_file, 'w') as f:
                f.write("======== MMLogger Performance Test Report ========\n\n")

                # Basic information
                report_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                f.write(f"Generated: {report_time}\n\n")

                # Performance metrics
                f.write("--- Performance Metrics ---\n")
                if 'throughput' in metrics:
                    f.write(f"Throughput: {metrics['throughput']:.2f} logs/sec\n")
                if 'avg_latency' in metrics:
                    f.write(f"Average Latency: {metrics['avg_latency']:.2f} μs\n")
                if 'cpu_percent' in metrics:
                    f.write(f"CPU Usage: {metrics['cpu_percent']:.2f}%\n")
                if 'memory_mb' in metrics:
                    f.write(f"Memory Usage: {metrics['memory_mb']:.2f} MB\n")

                # Queue statistics
                if any(k in metrics for k in ['enqueued_count', 'processed_count', 'glog_enqueued', 'glog_processed']):
                    f.write("\n--- Queue Statistics ---\n")

                    # Prefer direct metrics if available
                    if 'enqueued_count' in metrics:
                        f.write(f"Messages Enqueued: {metrics['enqueued_count']}\n")
                    elif 'glog_enqueued' in metrics:
                        f.write(f"Messages Enqueued: {metrics['glog_enqueued']}\n")

                    if 'processed_count' in metrics:
                        f.write(f"Messages Processed: {metrics['processed_count']}\n")
                    elif 'glog_processed' in metrics:
                        f.write(f"Messages Processed: {metrics['glog_processed']}\n")

                    if 'glog_dropped' in metrics:
                        f.write(f"Messages Dropped: {metrics['glog_dropped']}\n")

                    if 'glog_overflow' in metrics:
                        f.write(f"Memory Overflow Count: {metrics['glog_overflow']}\n")

                f.write("\n=============================================\n")
                f.write("Raw data available in CSV and log files\n")

            logger.info(f"Report generated: {report_file}")

    except Exception as e:
        logger.error(f"Error processing test results: {e}")

def main():
    """Main execution function"""
    args = parse_args()

    # Load configuration
    config = load_yaml_config(args.config)

    # Compile benchmark if needed
    if not args.skip_compile and not compile_benchmark():
        sys.exit(1)

    # Find benchmark executable
    benchmark_binary = args.binary if args.binary else find_benchmark_binary()
    if not benchmark_binary:
        logger.error("Benchmark executable not found. Cannot continue.")
        sys.exit(1)

    # Create output directory with timestamp
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    output_base_dir = Path(args.output_dir) / timestamp
    output_base_dir.mkdir(parents=True, exist_ok=True)

    # Copy configuration for reference
    config_copy = output_base_dir / "test_config_used.yaml"
    with open(args.config, 'r') as src, open(config_copy, 'w') as dst:
        dst.write(src.read())

    # Determine which test suites to run
    test_suites_to_run = []
    if args.test_suite:
        # Find specific test suite
        for suite in config['test_suites']:
            if suite['name'] == args.test_suite:
                test_suites_to_run.append(suite)
                break
        if not test_suites_to_run:
            logger.error(f"Test suite not found: {args.test_suite}")
            sys.exit(1)
    else:
        # Run all test suites
        test_suites_to_run = config['test_suites']

    # Run tests
    test_results = []
    for suite in test_suites_to_run:
        suite_name = suite['name']
        suite_dir = output_base_dir / suite_name.replace(' ', '_')
        suite_dir.mkdir(exist_ok=True)

        logger.info(f"Running test suite: {suite_name}")
        if args.verbose:
            print(f"\n{Colors.BLUE}Test Suite:{Colors.NC} {suite_name}")
            print(f"Description: {suite.get('description', 'No description')}")

        for test in suite['tests']:
            # Skip if not the requested test ID
            if args.test_id and test['id'] != args.test_id:
                continue

            # Apply global config first, then test-specific config
            full_config = {}
            global_config = config.get('global', {})

            # Start with global config
            for key, value in global_config.items():
                full_config[key] = value

            # Apply test-specific config (overrides global)
            for key, value in test.items():
                full_config[key] = value

            # Run test (possibly multiple times)
            for run in range(args.repeat):
                run_suffix = f"_run_{run+1}" if args.repeat > 1 else ""
                output_file = suite_dir / f"{test['id']}{run_suffix}.csv"

                # Execute test
                success = run_test(benchmark_binary, full_config, str(output_file), args.verbose)
                if success:
                    test_results.append({
                        'suite': suite_name,
                        'test_id': test['id'],
                        'run': run + 1,
                        'output_file': str(output_file)
                    })

    # Process results
    if test_results:
        try:
            # Combine all results into one CSV
            all_results_csv = output_base_dir / "all_results.csv"
            logger.info(f"Combining test results into {all_results_csv}")

            # Collect all data
            header = None
            all_rows = []

            for result in test_results:
                csv_file = result['output_file']
                logger.info(f"Processing result file: {csv_file}")

                if not os.path.exists(csv_file):
                    logger.error(f"Result file not found: {csv_file}")
                    continue

                # Check file size
                file_size = os.path.getsize(csv_file)
                if file_size == 0:
                    logger.error(f"Result file is empty: {csv_file}")
                    continue

                try:
                    with open(csv_file, 'r', newline='') as f:
                        # Try reading as CSV
                        reader = csv.reader(f)
                        rows = list(reader)

                        if not rows:
                            logger.error(f"No CSV rows in file: {csv_file}")
                            continue

                        if len(rows) < 2:
                            logger.warning(f"CSV file has only {len(rows)} row(s): {csv_file}")
                            # Print file content for debugging
                            with open(csv_file, 'r') as debug_f:
                                content = debug_f.read()
                                logger.debug(f"File content sample: {content[:500]}...")
                            # If it's just a header, continue to next file
                            if len(rows) == 1:
                                if header is None:
                                    header = rows[0]
                                    logger.info(f"Using header from file with no data rows: {csv_file}")
                                continue

                        # Set header from first file with data
                        if header is None:
                            header = rows[0]
                            logger.info(f"Using header: {header}")

                        # Add data rows (skip header)
                        data_rows = rows[1:]
                        logger.info(f"Found {len(data_rows)} data rows in {csv_file}")
                        all_rows.extend(data_rows)
                except Exception as e:
                    logger.error(f"Error reading CSV file {csv_file}: {e}")
                    import traceback
                    logger.error(traceback.format_exc())

            # Write combined data
            if header and all_rows:
                with open(all_results_csv, 'w', newline='') as f:
                    writer = csv.writer(f)
                    writer.writerow(header)
                    writer.writerows(all_rows)
                logger.info(f"Combined {len(all_rows)} rows into {all_results_csv}")
            else:
                logger.error(f"Failed to combine results: header={bool(header)}, rows={len(all_rows)}")

                # Create a minimal results file if we have header but no rows
                if header and not all_rows:
                    logger.warning("Creating empty results file with just header")
                    with open(all_results_csv, 'w', newline='') as f:
                        writer = csv.writer(f)
                        writer.writerow(header)

        except Exception as e:
            logger.error(f"Error combining results: {e}")
            import traceback
            logger.error(traceback.format_exc())

        # Generate summary statistics
        summarize_results(str(all_results_csv))

        # Process and create reports
        output_files = [r['output_file'] for r in test_results]
        process_test_results(output_files)

        logger.info(f"All tests completed. Results saved to {output_base_dir}")
        print(f"\n{Colors.GREEN}Benchmark completed successfully!{Colors.NC}")
        print(f"Results saved to: {output_base_dir}")
    else:
        logger.warning("No tests were completed successfully")
        print(f"\n{Colors.YELLOW}No tests were completed successfully.{Colors.NC}")

if __name__ == "__main__":
    main()
