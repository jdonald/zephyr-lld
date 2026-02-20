include(${ZEPHYR_BASE}/cmake/linker/lld/linker_flags.cmake)

# Workaround: Zephyr's picolibc wrappers (pulled in via --whole-archive) and
# the SDK's bundled picolibc both define strong __retarget_lock_* symbols.
# GNU ld silently uses the first definition; lld is strict and errors out.
# Until upstream Zephyr marks one set as weak, allow multiple definitions.
list(APPEND TOOLCHAIN_LD_FLAGS -Wl,--allow-multiple-definition)
