# MMLogger Benchmark System

This directory contains the benchmark tools for testing the performance of the MMLogger library. The system has been redesigned to ensure proper integration between the YAML configuration, Python runner script, and C++ benchmark implementation.

## Overview

The benchmark system consists of:

1. **Configuration Files**: YAML files that define test suites and test cases
2. **Python Runner Script**: Handles test execution, environment configuration, and result processing
3. **C++ Benchmark Program**: Performs the actual benchmarking with the specified parameters
4. **Bash Script Wrapper**: Provides an easy-to-use command-line interface

## Quick Start

To run all benchmarks with default settings:

```bash
./run_benchmark.sh
```

To run a specific test suite:

```bash
./run_benchmark.sh --suite="Basic Performance Comparison"
```

To run a specific test multiple times:

```bash
./run_benchmark.sh --test=optimized_basic --repeat=3 --verbose
```

## Configuration

Test configurations are defined in YAML format. The default configuration file is `benchmarks/test_config.yaml`. Each test suite contains multiple test cases with specific parameters.

Example of a test configuration:

```yaml
- id: "optimized_basic"
  name: "OptimizedGLog Logger (Basic)"
  logger_type: "OptimizedGLog"
  enable_console: true
  enable_file: true
  log_file_path: "./logs/optimized"
  log_level: "info"
  num_threads: 4
  logs_per_thread: 100000
  log_msg_size: 128
  log_rate: 0
  batch_size: 200
  queue_capacity: 10000
  num_workers: 4
  pool_size: 20000
  test_duration: 10
```

### Configuration Parameters

#### Basic Parameters:
- `id`: Unique identifier for the test
- `name`: Human-readable name for the test
- `logger_type`: Type of logger to test (Stdout, GLog, OptimizedGLog)
- `enable_console`: Enable console output (true/false)
- `enable_file`: Enable file output (true/false)
- `log_file_path`: Path for log files
- `log_level`: Log level to use (debug, info, warn, error, fatal)
- `num_threads`: Number of threads generating logs
- `logs_per_thread`: Number of logs per thread
- `log_msg_size`: Size of log messages in bytes
- `log_rate`: Rate limit for logs (0 = unlimited)
- `test_duration`: Duration of the test in seconds

#### OptimizedGLog Parameters:
- `batch_size`: Number of messages to batch process
- `queue_capacity`: Maximum size of the message queue
- `num_workers`: Number of worker threads
- `pool_size`: Size of the memory pool

## Running Benchmarks

### Command-Line Options

The `run_benchmark.sh` script provides the following options:

```
Usage: ./run_benchmark.sh [options]

Run performance benchmarks for MMLogger library

Options:
  -h, --help              Show this help message
  -c, --config=FILE       Use specified configuration file (default: benchmarks/test_config.yaml)
  -s, --suite=NAME        Run only the specified test suite
  -t, --test=ID           Run only the specified test ID
  -r, --repeat=N          Repeat each test N times (default: 1)
  -o, --output=DIR        Set output directory (default: results)
  --skip-compile          Skip compilation step
  -v, --verbose           Enable verbose output
```

### Direct Python Usage

You can also run the Python script directly:

```bash
python3 benchmarks/run_benchmarks.py --config=test_config.yaml --test-suite="Basic Performance Comparison" --verbose
```

## Results

Test results are stored in the output directory (default: `results/`) with a timestamp. For each test run, the system generates:

1. **Raw CSV Data**: Contains all performance metrics
2. **Log Files**: Contains detailed output from the benchmark
3. **Summary Statistics**: Aggregated metrics for multiple runs
4. **Performance Reports**: Human-readable reports highlighting key metrics

### Output Structure

```
results/
└── 20250418_123456/                  # Timestamp
    ├── test_config_used.yaml         # Copy of the configuration used
    ├── Basic_Performance_Comparison/ # Test suite folder
    │   ├── stdout_basic.csv          # Raw test results
    │   ├── stdout_basic.log          # Test log
    │   ├── stdout_basic_report.txt   # Performance report
    │   └── ...                       # Other tests in this suite
    ├── Threading_Scalability_Tests/  # Another test suite
    │   └── ...
    ├── all_results.csv               # Combined results from all tests
    └── all_results_summary.csv       # Statistical summary
```

## Customization

### Creating Custom Test Configurations

You can create your own test configurations by copying and modifying the `test_config.yaml` file:

```bash
cp benchmarks/test_config.yaml my_config.yaml
# Edit my_config.yaml with your preferred text editor
./run_benchmark.sh --config=my_config.yaml
```

### Adding New Test Suites

To add a new test suite, add a new entry to the YAML file:

```yaml
test_suites:
  - name: "My Custom Test Suite"
    description: "Description of my test suite"
    tests:
      - id: "my_test_1"
        name: "My First Test"
        logger_type: "OptimizedGLog"
        # ... other parameters ...
```

## Troubleshooting

### Common Issues

1. **Missing Test Results**: Ensure that the benchmark binary has been compiled successfully
2. **Parameter Mismatch**: Verify that parameter names in the YAML match the expected names in the C++ code
3. **Permissions Issues**: Make sure log directories are writable

### Debugging

For more detailed output, use the `--verbose` flag:

```bash
./run_benchmark.sh --verbose
```

Check the benchmark log file for detailed information:

```bash
cat benchmark_runner.log
```

## Advanced Usage

### Continuous Integration

For CI/CD pipelines, you can use:

```bash
./run_benchmark.sh --skip-compile --config=ci_config.yaml --output=ci_results
```

### Comparing Configuration Changes

To compare the impact of configuration changes:

1. Create two configuration files with different parameters
2. Run benchmarks with each configuration
3. Compare the results using the generated summary files
