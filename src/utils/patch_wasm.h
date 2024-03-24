/*
 * This file includes some patched functions that are not linked properly when building for WASM.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
uint32_t __imported_wasi_snapshot_preview1_fd_fdstat_get(uint32_t, uint32_t) {
    assert(false);
    return 0;
}

uint32_t __imported_wasi_snapshot_preview1_fd_close(uint32_t) {
    assert(false);
    return 0;
}

int32_t __imported_wasi_snapshot_preview1_fd_seek(int32_t, int64_t, int32_t, int32_t) {
    assert(false);
    return -1;
}

int32_t __imported_wasi_snapshot_preview1_fd_write(int32_t, int32_t, int32_t, int32_t) {
    assert(false);
    return -1;
}

int vfprintf(FILE*, const char *, va_list) {
    return -1;
}
}
