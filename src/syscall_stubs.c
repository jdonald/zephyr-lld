/*
 * Minimal POSIX syscall stubs for lld compatibility.
 *
 * LLVM's lld resolves archive symbols more aggressively than GNU ld.
 * When linking with the Zephyr SDK's bundled libc.a (newlib/picolibc),
 * lld pulls in reentrant wrappers (_sbrk_r, _read_r, etc.) that
 * reference these POSIX syscall stubs.  GNU ld does not pull them in
 * because by the time it processes libc.a the symbols are already
 * satisfied by Zephyr's picolibc layer.
 *
 * These stubs are only reached if application code inadvertently calls
 * into newlib's file-I/O or malloc paths — which a bare-metal Zephyr
 * app should never do.  Each stub returns the appropriate error value.
 *
 * Declared __weak so they yield to any real implementation that may
 * appear in Zephyr or the SDK, and are harmlessly ignored in GNU ld
 * builds where the symbols are never referenced.
 */

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

__attribute__((weak)) int _close(int file)
{
	(void)file;
	errno = ENOSYS;
	return -1;
}

__attribute__((weak)) int _fstat(int file, struct stat *st)
{
	(void)file;
	(void)st;
	errno = ENOSYS;
	return -1;
}

__attribute__((weak)) int _isatty(int file)
{
	(void)file;
	errno = ENOSYS;
	return 0;
}

__attribute__((weak)) off_t _lseek(int file, off_t ptr, int dir)
{
	(void)file;
	(void)ptr;
	(void)dir;
	errno = ENOSYS;
	return (off_t)-1;
}

__attribute__((weak)) int _read(int file, void *buf, int nbytes)
{
	(void)file;
	(void)buf;
	(void)nbytes;
	errno = ENOSYS;
	return -1;
}

__attribute__((weak)) int _write(int file, const void *buf, int nbytes)
{
	(void)file;
	(void)buf;
	(void)nbytes;
	errno = ENOSYS;
	return -1;
}

__attribute__((weak)) void *_sbrk(ptrdiff_t incr)
{
	(void)incr;
	errno = ENOMEM;
	return (void *)-1;
}
