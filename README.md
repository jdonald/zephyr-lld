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

### Build with lld Linker

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

3. **Apply the Zephyr source patch** (see [Upstream Fixes Required](#upstream-fixes-required)):

   ```bash
   cd $ZEPHYR_BASE
   git apply /path/to/zephyr-lld/patches/0001-soc-renesas-ra4m1-lld-compat-rom_start.ld.patch
   ```

4. **Build** with the custom toolchain variant:

   ```bash
   west build -b arduino_uno_r4@minima -d build-lld \
     -- -DZEPHYR_TOOLCHAIN_VARIANT=zephyr-lld -DTOOLCHAIN_ROOT=$(pwd)
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

## Workarounds Applied for lld Compatibility

Three categories of issues prevent a stock Zephyr build from linking with
`ld.lld`. This project includes workarounds for all three:

### 1. Duplicate `__retarget_lock_*` Symbols

**Problem:** Zephyr's picolibc lock wrappers (in `lib/libc/picolibc/locks.c`,
pulled in via `--whole-archive`) and the SDK's bundled picolibc both define
strong `__retarget_lock_*` symbols. GNU ld silently uses the first definition
encountered; lld is strict and errors on duplicates.

**Workaround:** The custom `cmake/linker/lld/linker_flags.cmake` adds
`-Wl,--allow-multiple-definition` to the lld link flags.

**Upstream fix:** Zephyr should declare its picolibc lock wrappers as
`__attribute__((weak))`, or the SDK should build picolibc without the
conflicting strong definitions.

### 2. Linker Script Section Placement (`rom_start.ld`)

**Problem:** The Renesas RA4M1 SoC linker include file
(`soc/renesas/ra/ra4m1/rom_start.ld`) uses:

```ld
. = DT_REG_ADDR(DT_NODELABEL(option_setting_ofs0));
```

which preprocesses to `. = 1024;`. Inside an output section, GNU ld treats
this as a section-relative offset (VMA = section_start + 1024). LLVM lld
treats it as an absolute VMA (0x400), which is before the section start at
0x4000, causing a "unable to move location counter backward" error.

**Workaround:** Requires patching the Zephyr source file to use
`__rom_start_address + DT_REG_ADDR(...)` under `#ifdef __LLD_LINKER_CMD__`.
Zephyr already defines `__LLD_LINKER_CMD__` when using lld, and the same
pattern is used in `arch/common/rom_start_offset.ld`.

**Upstream fix:** This is a bug in the Renesas RA4M1 SoC's `rom_start.ld`.
The same issue likely affects other Renesas RA SoCs. The fix is minimal
(conditional `#ifdef`) and follows existing Zephyr conventions.

### 3. Missing POSIX Syscall Stubs

**Problem:** lld resolves archive symbols more aggressively than GNU ld. It
pulls in reentrant wrappers (`_sbrk_r`, `_read_r`, etc.) from the SDK's
bundled `libc.a` that reference POSIX syscall stubs (`_sbrk`, `_read`,
`_write`, `_close`, `_lseek`, `_fstat`, `_isatty`). GNU ld does not pull
these in because Zephyr's picolibc layer satisfies the needed symbols earlier
in archive resolution order.

**Workaround:** `src/syscall_stubs.c` provides weak stub implementations
that return appropriate error values. These are harmlessly ignored in ld.bfd
builds.

**Upstream fix:** This is a consequence of lld's different (arguably more
correct) archive resolution. Zephyr could provide these stubs in its picolibc
integration layer, or the link order could be adjusted so `-lc` is not needed
when picolibc is the configured libc.

## Upstream Fixes Required

To build with lld without workarounds, the following upstream changes are needed:

| Component | Issue | Severity |
|-----------|-------|----------|
| **Zephyr SDK** | `cmake/zephyr/generic.cmake` hardcodes `set(LINKER ld)` — should allow override | Medium |
| **Zephyr** | `soc/renesas/ra/ra4m1/rom_start.ld` uses bare `. = constant` inside output section — incompatible with lld | **High** |
| **Zephyr** | `lib/libc/picolibc/locks.c` defines strong `__retarget_lock_*` that collide with SDK picolibc | Medium |
| **Zephyr** | Missing weak POSIX syscall stubs for lld's archive resolution behavior | Low |

## Project Structure

```
.
├── CMakeLists.txt                          # Zephyr application build file
├── prj.conf                                # Kconfig: enables GPIO driver
├── src/
│   ├── main.c                              # LED blink pattern
│   └── syscall_stubs.c                     # Weak POSIX stubs for lld compat
├── cmake/
│   ├── toolchain/zephyr-lld/
│   │   ├── generic.cmake                   # Custom variant: SDK + lld override
│   │   └── target.cmake                    # Delegates to SDK target config
│   ├── compiler/gcc/                       # Forwarders to ${ZEPHYR_BASE}
│   ├── linker/lld/                         # Forwarders + lld-specific flags
│   └── bintools/gnu/                       # Forwarders to ${ZEPHYR_BASE}
├── patches/                                # Zephyr source patches (see below)
└── README.md
```

## License

Apache-2.0 (matching the Zephyr project license).
