// Linux stub for <bcrypt.h>
#pragma once
// Windows crypto API not available on Linux
#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG 0x00000002
inline NTSTATUS BCryptGenRandom(void*, unsigned char* buf, unsigned long size, unsigned long)
{
    // Use /dev/urandom on Linux
    FILE* f = fopen("/dev/urandom", "rb");
    if (f) { fread(buf, 1, size, f); fclose(f); return 0; }
    return -1;
}
