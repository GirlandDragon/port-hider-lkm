export DDK_ROOT=/opt/ddk

# Choose your target kernel version (uncomment one set):
export KDIR=$DDK_ROOT/kdir/android12-5.10
export CLANG_PATH=$DDK_ROOT/clang/clang-r416183b/bin

# export KDIR=$DDK_ROOT/kdir/android13-5.15
# export CLANG_PATH=$DDK_ROOT/clang/clang-r450784e/bin

# export KDIR=$DDK_ROOT/kdir/android14-6.1
# export CLANG_PATH=$DDK_ROOT/clang/clang-r487747c/bin

# export KDIR=$DDK_ROOT/kdir/android16-6.12
# export CLANG_PATH=$DDK_ROOT/clang/clang-r522817f/bin

# Or use NDK Clang directly:
# export KDIR=/path/to/kernel-headers
# export CLANG_PATH=$HOME/Android/Sdk/ndk/*/toolchains/llvm/prebuilt/linux-x86_64/bin

export PATH=$CLANG_PATH:$PATH
export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64
export LLVM=1
export LLVM_IAS=1

echo "=== Port Hider LKM - Build Environment ==="
echo "KDIR:     $KDIR"
echo "ARCH:     $ARCH"
echo "CLANG:    $(which clang 2>/dev/null || echo 'not in PATH')"
echo ""
echo "Run 'make' to build"
