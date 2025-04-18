#!/bin/bash
# Script for running MMLogger benchmarks with different configurations

# Colors for terminal output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No color

# Default parameters
CONFIG_FILE="benchmarks/test_config_simple.yaml"
TEST_SUITE=""
TEST_ID=""
REPEAT=1
OUTPUT_DIR="results"
SKIP_COMPILE=false
VERBOSE=false

# Display usage information
function show_help {
    echo "Usage: $0 [options]"
    echo
    echo "Run performance benchmarks for MMLogger library"
    echo
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -c, --config=FILE       Use specified configuration file (default: benchmarks/test_config.yaml)"
    echo "  -s, --suite=NAME        Run only the specified test suite"
    echo "  -t, --test=ID           Run only the specified test ID"
    echo "  -r, --repeat=N          Repeat each test N times (default: 1)"
    echo "  -o, --output=DIR        Set output directory (default: results)"
    echo "  --skip-compile          Skip compilation step"
    echo "  -v, --verbose           Enable verbose output"
    echo
    echo "Examples:"
    echo "  $0 --suite=\"Basic Performance Comparison\""
    echo "  $0 --test=optimized_basic --repeat=3 --verbose"
    echo "  $0 --config=custom_config.yaml"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c=*|--config=*)
            CONFIG_FILE="${1#*=}"
            shift
            ;;
        -s=*|--suite=*)
            TEST_SUITE="${1#*=}"
            shift
            ;;
        -t=*|--test=*)
            TEST_ID="${1#*=}"
            shift
            ;;
        -r=*|--repeat=*)
            REPEAT="${1#*=}"
            shift
            ;;
        -o=*|--output=*)
            OUTPUT_DIR="${1#*=}"
            shift
            ;;
        --skip-compile)
            SKIP_COMPILE=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Check if configuration file exists
if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "${RED}Error: Configuration file '$CONFIG_FILE' not found${NC}"
    exit 1
fi

# Check if Python script exists
PYTHON_SCRIPT="benchmarks/run_benchmarks.py"
if [ ! -f "$PYTHON_SCRIPT" ]; then
    echo -e "${RED}Error: Python script '$PYTHON_SCRIPT' not found${NC}"
    exit 1
fi

# Prepare arguments for the Python script
ARGS="--config=$CONFIG_FILE --output-dir=$OUTPUT_DIR --repeat=$REPEAT"

if [ -n "$TEST_SUITE" ]; then
    ARGS="$ARGS --test-suite=\"$TEST_SUITE\""
fi

if [ -n "$TEST_ID" ]; then
    ARGS="$ARGS --test-id=$TEST_ID"
fi

if [ "$SKIP_COMPILE" = true ]; then
    ARGS="$ARGS --skip-compile"
fi

if [ "$VERBOSE" = true ]; then
    ARGS="$ARGS --verbose"
fi

# Display benchmark information
echo -e "${GREEN}Running MMLogger Benchmarks${NC}"
echo -e "${BLUE}Configuration:${NC} $CONFIG_FILE"
if [ -n "$TEST_SUITE" ]; then
    echo -e "${BLUE}Test Suite:${NC} $TEST_SUITE"
fi
if [ -n "$TEST_ID" ]; then
    echo -e "${BLUE}Test ID:${NC} $TEST_ID"
fi
echo -e "${BLUE}Repeat Count:${NC} $REPEAT"
echo -e "${BLUE}Output Directory:${NC} $OUTPUT_DIR"
echo

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Run the benchmark
echo -e "${GREEN}Starting benchmarks...${NC}"
echo

# Evaluate the command string to handle quotes properly
eval "python3 $PYTHON_SCRIPT $ARGS"

# Check exit status
if [ $? -eq 0 ]; then
    echo
    echo -e "${GREEN}Benchmark completed successfully!${NC}"
else
    echo
    echo -e "${RED}Benchmark failed!${NC}"
    exit 1
fi
