#!/bin/bash
# Build script for gFractor audio plugin (macOS/Linux)

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="build"
PARALLEL_JOBS=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}gFractor Plugin Build Script${NC}"
echo -e "${GREEN}========================================${NC}"

# Check if JUCE submodule exists
if [ ! -f "JUCE/CMakeLists.txt" ]; then
    echo -e "${YELLOW}JUCE submodule not found. Initializing submodules...${NC}"
    git submodule update --init --recursive
fi

# Parse command line arguments
CLEAN=false
RUN_TESTS=false
INSTALL=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        --install)
            INSTALL=true
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --clean      Clean build directory before building"
            echo "  --test       Run tests after building"
            echo "  --install    Install plugin to system directories"
            echo "  --debug      Build in Debug mode (default: Release)"
            echo "  --release    Build in Release mode"
            echo "  --help       Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Configure
echo -e "${GREEN}Configuring CMake (${BUILD_TYPE})...${NC}"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo -e "${GREEN}Building with ${PARALLEL_JOBS} parallel jobs...${NC}"
cmake --build "$BUILD_DIR" \
    --config "$BUILD_TYPE" \
    --parallel "$PARALLEL_JOBS"

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo -e "${GREEN}Running tests...${NC}"
    ctest --test-dir "$BUILD_DIR" \
        --build-config "$BUILD_TYPE" \
        --output-on-failure
fi

# Install if requested
if [ "$INSTALL" = true ]; then
    echo -e "${GREEN}Installing plugin...${NC}"
    cmake --install "$BUILD_DIR" --config "$BUILD_TYPE"
fi

# Summary
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "Build type: ${BUILD_TYPE}"
echo -e "Build directory: ${BUILD_DIR}"
echo ""

# Show plugin locations
if [ -d "$BUILD_DIR" ]; then
    echo -e "${GREEN}Plugin artifacts:${NC}"
    find "$BUILD_DIR" -name "*.vst3" -o -name "*.component" | while read -r file; do
        echo "  - $file"
    done
fi

echo ""
echo -e "${YELLOW}Tip: Use --test to run tests, --clean to rebuild from scratch${NC}"
