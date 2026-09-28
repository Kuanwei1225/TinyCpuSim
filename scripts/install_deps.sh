#!/bin/bash
# TinyCpuSim Prerequisites & Dependencies Installer
# Automatically installs C++ compilers, CMake, ARM cross-toolchain, and Python packages.
# NOTE: gem5 is OPTIONAL and not required for normal simulation, tests, or ubench verifications.

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Resolve default installation path for Python, avoiding env/PATH where possible
find_default_python() {
    for candidate in \
        "/usr/bin/python3" \
        "/usr/local/bin/python3" \
        "/opt/homebrew/bin/python3" \
        "/usr/bin/python"; do
        if [ -x "${candidate}" ]; then
            echo "${candidate}"
            return 0
        fi
    done
    if command -v python3 >/dev/null 2>&1; then
        command -v python3
        return 0
    fi
    echo "python3"
}

PYTHON_BIN=$(find_default_python)

echo "============================================================"
echo "       TinyCpuSim - Dependencies & Environment Setup        "
echo "============================================================"

detect_os() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ -f /etc/debian_version ]]; then
        echo "debian"
    elif [[ -f /etc/fedora-release ]] || [[ -f /etc/redhat-release ]]; then
        echo "fedora"
    elif [[ -f /etc/arch-release ]]; then
        echo "arch"
    else
        echo "unknown"
    fi
}

OS=$(detect_os)
echo "[INFO] Detected Operating System: ${OS}"

case "${OS}" in
    debian)
        echo "[STEP 1/2] Installing System Packages via APT..."
        if [ "$EUID" -ne 0 ]; then
            SUDO="sudo"
        else
            SUDO=""
        fi
        
        ${SUDO} apt-get update -y
        ${SUDO} apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            gcc-arm-none-eabi \
            binutils-arm-none-eabi \
            python3 \
            python3-pip \
            python3-dev \
            pkg-config
        ;;
    macos)
        echo "[STEP 1/2] Installing Packages via Homebrew..."
        if ! command -v brew &>/dev/null; then
            echo "[ERROR] Homebrew not found. Please install Homebrew from https://brew.sh/"
            exit 1
        fi
        brew install cmake ninja arm-none-eabi-gcc python3
        ;;
    fedora)
        echo "[STEP 1/2] Installing System Packages via DNF..."
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            ninja-build \
            arm-none-eabi-gcc-cs \
            arm-none-eabi-binutils-cs \
            python3 \
            python3-pip \
            python3-devel
        ;;
    arch)
        echo "[STEP 1/2] Installing System Packages via Pacman..."
        sudo pacman -Syu --noconfirm \
            base-devel \
            cmake \
            ninja \
            arm-none-eabi-gcc \
            arm-none-eabi-binutils \
            python \
            python-pip
        ;;
    *)
        echo "[WARN] Unknown OS. Please manually ensure cmake, g++, arm-none-eabi-gcc, and python3 are installed."
        ;;
esac

echo ""
echo "[STEP 2/2] Installing Python Microarchitecture & Assembler Packages..."
echo " Using Python: ${PYTHON_BIN}"
"${PYTHON_BIN}" -m pip install --upgrade pip 2>/dev/null || true
"${PYTHON_BIN}" -m pip install --break-system-packages pyelftools keystone-engine capstone pydot 2>/dev/null || \
"${PYTHON_BIN}" -m pip install --user pyelftools keystone-engine capstone pydot 2>/dev/null || \
"${PYTHON_BIN}" -m pip install pyelftools keystone-engine capstone pydot || true

echo ""
echo "============================================================"
echo " [SUCCESS] All TinyCpuSim core dependencies installed!"
echo " Next steps:"
echo "   1. Build simulator:  ./run.sh build"
echo "   2. Run test suite:   ./run.sh test"
echo "   3. Run ubenchmarks:  ./run.sh ubench all"
echo "============================================================"
