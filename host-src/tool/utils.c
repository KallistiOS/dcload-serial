#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Detect MinGW/MSYS vs. MinGW-w64/MSYS2 */
#ifdef __MINGW32__
#include <_mingw.h>
# ifdef __MINGW64_VERSION_MAJOR
#  define __RT_MINGW_W64__
# else
#  define __RT_MINGW_ORG__
# endif
#endif /* __MINGW32__ */

#ifdef __RT_MINGW_ORG__

/*
 * Compatibility layer for original, legacy MinGW/MSYS environment.
 * This allow toolchains built on MinGW-w64/MSYS2 to be usable with MinGW/MSYS.
 * Mainly, this is for linking 'dc-tool' with 'libbfd'. Declaring these
 * functions will let us to build 'dc-tool' even if 'sh-elf' toolchain was built
 * on MinGW-w64/MSYS2.
 */

// Thanks to Dietrich Epp
// See: https://stackoverflow.com/a/40160038
int vasprintf(char **strp, const char *fmt, va_list ap) {    
    int len = _vscprintf(fmt, ap);
    if (len == -1) {
        return -1;
    }
    size_t size = (size_t)len + 1;
    char *str = malloc(size);
    if (!str) {
        return -1;
    }    
    int r = __mingw_vsnprintf(str, len + 1, fmt, ap);
    if (r == -1) {
        free(str);
        return -1;
    }
    *strp = str;
    return r;
}

// Thanks to Dietrich Epp
// See: https://stackoverflow.com/a/40160038
int __cdecl __MINGW_NOTHROW libintl_asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vasprintf(strp, fmt, ap);
    va_end(ap);
    return r;
}

// See: https://stackoverflow.com/a/60380005
int __cdecl __MINGW_NOTHROW __ms_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr) {
	return __mingw_vsnprintf(buffer, count, format, argptr);
}

// Thanks to Kenji Uno and god
// See: https://github.com/HiraokaHyperTools/libacrt_iob_func
// See: https://stackoverflow.com/a/30894349
FILE * __cdecl __MINGW_NOTHROW _imp____acrt_iob_func(int handle) {
    switch (handle) {
        case 0: return stdin;
        case 1: return stdout;
        case 2: return stderr;
    }
    return NULL;
}

#endif /* __RT_MINGW_ORG__ */
