# SPDX-License-Identifier: Apache-2.0
#
# Custom toolchain variant: zephyr SDK compiler (gcc) with LLVM lld linker.
# This wraps the standard "zephyr" toolchain but overrides the linker to lld.

include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/generic.cmake)

set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)

# Override: use lld instead of ld
set(LINKER lld)

message(STATUS "Found toolchain: zephyr-lld (zephyr SDK ${SDK_VERSION} + lld) (${ZEPHYR_SDK_INSTALL_DIR})")
