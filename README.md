# zephyr-lld

A minimal [Zephyr RTOS](https://zephyrproject.org/) project for the
**Arduino Uno R4 Minima** that blinks the built-in LED in the
"Shave and a Haircut, Two Bits" (5-and-2 knock) rhythm on each reset.

> **Note:** The original intent was to target the Arduino Uno R3
> (ATmega328P), but Zephyr does not support the AVR architecture. The
> Arduino Uno R4 Minima (Renesas RA4M1, ARM Cortex-M4) is the closest
> officially-supported Arduino Uno board.

## What It Does

On power-up or reset the LED on pin D13 plays the classic seven-knock
pattern, then stays off:

```
  "Shave  and-a  hair - cut"    (pause)    "two    bits"
   *      **     *     *                    *      *
```

## Prerequisites

| Tool | Linux | macOS |
|------|-------|-------|
| Python 3.10+ | `apt install python3 python3-pip python3-venv` | Ships with Xcode CLT or `brew install python` |
| CMake 3.20+ | `apt install cmake` | `brew install cmake` |
| Ninja | `apt install ninja-build` | `brew install ninja` |
| Device Tree Compiler | `apt install device-tree-compiler` | `brew install dtc` |
| West (Zephyr meta-tool) | `pip3 install west` | `pip3 install west` |
| Zephyr SDK 0.17.0 | See below | See below |
| LLVM/lld (optional) | `apt install lld` | `brew install llvm` |

### Install the Zephyr SDK

```bash
# Linux (x86_64)
cd /opt
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz
tar xf zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz
cd zephyr-sdk-0.17.0
./setup.sh -t arm-zephyr-eabi -c

# macOS (AArch64 / Apple Silicon)
cd /opt
curl -LO https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_macos-aarch64_minimal.tar.xz
tar xf zephyr-sdk-0.17.0_macos-aarch64_minimal.tar.xz
cd zephyr-sdk-0.17.0
./setup.sh -t arm-zephyr-eabi -c

# macOS (x86_64 / Intel)
cd /opt
curl -LO https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_macos-x86_64_minimal.tar.xz
tar xf zephyr-sdk-0.17.0_macos-x86_64_minimal.tar.xz
cd zephyr-sdk-0.17.0
./setup.sh -t arm-zephyr-eabi -c
```

Export the SDK location (add to your shell profile):

```bash
export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.17.0
```

### Initialize the Zephyr Workspace

```bash
west init -m https://github.com/zephyrproject-rtos/zephyr --mr main ~/zephyrproject
cd ~/zephyrproject
west update --narrow -o=--depth=1 hal_renesas cmsis cmsis_6
pip3 install -r zephyr/scripts/requirements.txt
```

Export the Zephyr base (add to your shell profile):

```bash
export ZEPHYR_BASE=~/zephyrproject/zephyr
```

## Building

### Default Build (ld.bfd)

```bash
west build -b arduino_uno_r4@minima
```

or, with an explicit build directory:

```bash
west build -b arduino_uno_r4@minima -d build-bfd
```

### Build with lld Linker (Experimental)

This project includes a custom `zephyr-lld` toolchain variant that uses the
GCC cross-compiler from the Zephyr SDK with LLVM's `ld.lld` as the linker.
To use it:

1. **Install lld** (see Prerequisites table above).

2. **Create symlinks** so the GCC cross-compiler driver can find `ld.lld`:

   ```bash
   ln -sf "$(which ld.lld)" "${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-ld.lld"
   ln -sf "$(which ld.lld)" "${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/arm-zephyr-eabi/bin/ld.lld"
   ```

   On macOS with Homebrew LLVM you may need to use the full path:
   ```bash
   ln -sf /opt/homebrew/opt/llvm/bin/ld.lld "${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-ld.lld"
   ln -sf /opt/homebrew/opt/llvm/bin/ld.lld "${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/arm-zephyr-eabi/bin/ld.lld"
   ```

3. **Build** with the custom toolchain variant:

   ```bash
   export ZEPHYR_TOOLCHAIN_VARIANT=zephyr-lld
   export TOOLCHAIN_ROOT=$(pwd)   # must point to this repository root
   west build -b arduino_uno_r4@minima -d build-lld
   ```

### Verifying the Linker Knob

After CMake configuration completes, inspect the generated `build.ninja` to
confirm the correct linker is used in **all** link steps (there are two: the
pre-link for `zephyr_pre0.elf` and the final link for `zephyr.elf`):

```bash
# For ld.bfd (default):
grep -o '\-fuse-ld=[a-z]*' build-bfd/build.ninja | sort | uniq -c
#   2 -fuse-ld=bfd

# For lld:
grep -o '\-fuse-ld=[a-z]*' build-lld/build.ninja | sort | uniq -c
#   2 -fuse-ld=lld
```

## Flashing

With the Arduino Uno R4 Minima connected via USB:

```bash
west flash
```

This uses [pyOCD](https://pyocd.io/) as the flash runner. You may need to
install a udev rule on Linux for USB access (see
[Zephyr's docs](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)).

After flashing, press the reset button to see the "Shave and a Haircut"
pattern on the built-in LED.

## How the Linker Variant Works

The Zephyr SDK's default toolchain (`zephyr`) hardcodes `set(LINKER ld)` in
its CMake configuration, which cannot be overridden via `-D` from the command
line. To work around this, this project provides a custom toolchain variant
called `zephyr-lld` under `cmake/toolchain/zephyr-lld/`. It:

1. Includes the standard Zephyr SDK's `generic.cmake` and `target.cmake`.
2. Overrides `LINKER` from `ld` to `lld`.
3. Provides forwarding `*.cmake` files under `cmake/compiler/gcc/`,
   `cmake/linker/lld/`, and `cmake/bintools/gnu/` that delegate to the
   corresponding files in `${ZEPHYR_BASE}`.

When `TOOLCHAIN_ROOT` points to this repository and
`ZEPHYR_TOOLCHAIN_VARIANT` is set to `zephyr-lld`, Zephyr's build system
picks up the overridden linker configuration.

## Known lld Linker Issues

As of Zephyr 4.x and LLVM lld 18.x, linking a Zephyr firmware image for the
Arduino Uno R4 Minima with `ld.lld` **does not fully succeed**. The
CMake configuration and compilation complete, and `build.ninja` correctly
contains `-fuse-ld=lld` in all link steps. However, the actual link step
fails with the following categories of errors:

### 1. Duplicate Symbol Errors

```
ld.lld: error: duplicate symbol: __retarget_lock_acquire
```

Zephyr's picolibc lock wrappers and the SDK's bundled picolibc both define
`__retarget_lock_*` symbols. GNU ld resolves these via archive ordering and
weak symbol semantics, but lld is stricter. This can be worked around with
`-Wl,--allow-multiple-definition`, but exposes the next issue.

### 2. Linker Script Incompatibilities

```
ld.lld: error: unable to place section rom_start at file offset [...]; check your linker script for overflows
ld.lld: error: section text virtual address range overlaps with rom_start
```

Zephyr's linker scripts (and the Renesas RA SoC-specific linker scripts) use
GNU ld extensions and section placement conventions that lld interprets
differently, resulting in address space layout errors.

### Next Steps for lld Investigation

- Upstream Zephyr issue [#41776](https://github.com/zephyrproject-rtos/zephyr/issues/41776)
  tracks `-fuse-ld=lld` support on `qemu_x86`; similar work would be needed
  for ARM targets.
- The linker script incompatibilities may require lld-specific linker script
  variants (Zephyr already has `linker-tool-lld.h` for preprocessor macros).
- The duplicate symbol issue could potentially be resolved by adjusting
  Zephyr's picolibc integration or using `--allow-multiple-definition` as a
  linker flag.
- Testing on simpler boards (e.g., `qemu_cortex_m3`) where the linker script
  is less complex may yield better results.

## Project Structure

```
.
├── CMakeLists.txt                          # Zephyr application build file
├── prj.conf                                # Kconfig: enables GPIO driver
├── src/
│   └── main.c                              # LED blink pattern
├── cmake/
│   ├── toolchain/zephyr-lld/
│   │   ├── generic.cmake                   # Custom variant: SDK + lld override
│   │   └── target.cmake                    # Delegates to SDK target config
│   ├── compiler/gcc/                       # Forwarders to ${ZEPHYR_BASE}
│   ├── linker/lld/                         # Forwarders to ${ZEPHYR_BASE}
│   └── bintools/gnu/                       # Forwarders to ${ZEPHYR_BASE}
└── README.md
```

## License

Apache-2.0 (matching the Zephyr project license).
