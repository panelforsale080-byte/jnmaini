#ifndef UTILS
#define UTILS

#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>

typedef unsigned long DWORD;
static uintptr_t libBase = 0;

DWORD findLibrary(const char *library) {
    char buffer[1024] = {0};
    FILE *fp = fopen("/proc/self/maps", "rt");
    if (fp == NULL) return 0;

    DWORD address = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, library)) {
            address = (DWORD)strtoul(buffer, NULL, 16);
            break;
        }
    }
    fclose(fp);
    return address;
}

DWORD getAbsoluteAddress(const char *libraryName, DWORD relativeAddr) {
    libBase = findLibrary(libraryName);
    if (libBase == 0) return 0;
    return (DWORD)(libBase + relativeAddr);
}

#endif