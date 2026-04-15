#!/bin/bash

set -e  # Exit on error
echo "🚀 Building Todo App (Release Mode)"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Clean previous build
if [ -d "build" ]; then
    echo -e "${YELLOW}🧹 Cleaning previous build...${NC}"
    rm -rf build
fi

# Create build directory
echo -e "${GREEN}📁 Creating build directory...${NC}"
mkdir -p build

# Move to build directory
cd build

# Configure with CMake (Release mode with Ninja)
echo -e "${GREEN}⚙️  Configuring CMake...${NC}"
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O2 -march=native -DNDEBUG" \
    ..

# Build with Ninja
echo -e "${GREEN}🔨 Building project...${NC}"
ninja

# Check if build succeeded
if [ -f "todo_app" ]; then
    echo -e "${GREEN}✅ Build successful!${NC}"
    echo -e "${GREEN}📦 Binary location: $(pwd)/todo_app${NC}"
    echo ""
    echo -e "${YELLOW}To run the app:${NC}"
    echo "  ./todo_app"
    echo ""
    echo -e "${YELLOW}Or from project root:${NC}"
    echo "  ./build/todo_app"
else
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi