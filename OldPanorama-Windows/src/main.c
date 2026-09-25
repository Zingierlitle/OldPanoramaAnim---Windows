#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <string.h>
#include "target.h"

static HMODULE self_module;

static void log_status(const char *text) {
    wchar_t path[32768];
    DWORD n = GetModuleFileNameW(self_module, path, 32768);
    if (!n || n > 32740) return;
    while (n && path[n-1] != L'\\') --n;
    const wchar_t name[] = L"OldPanorama.log";
    memcpy(path+n, name, sizeof(name));
    HANDLE f = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(f, text, (DWORD)strlen(text), &written, NULL);
    CloseHandle(f);
}

static int correct_executable(void) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(NULL, path, 32768)) return 0;
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE |
                          FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    unsigned char digest[32], *buffer = NULL;
    int ok = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) goto done;
    if (BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0) < 0) goto done;
    buffer = HeapAlloc(GetProcessHeap(), 0, 1024*1024);
    if (!buffer) goto done;
    for (;;) {
        DWORD n;
        if (!ReadFile(f, buffer, 1024*1024, &n, NULL)) goto done;
        if (!n) break;
        if (BCryptHashData(hash, buffer, n, 0) < 0) goto done;
    }
    if (BCryptFinishHash(hash, digest, sizeof(digest), 0) < 0) goto done;
    ok = memcmp(digest, expected_sha256, sizeof(digest)) == 0;
done:
    if (buffer) HeapFree(GetProcessHeap(), 0, buffer);
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(f);
    return ok;
}

// The target branch is aligned: replace the complete two-byte instruction
// atomically, never a partially written branch or a global Options getter.
static int patch_window(unsigned char *window) {
    if (memcmp(window, expected_window, sizeof(expected_window)) != 0) return 0;
    unsigned char *branch = window + BRANCH_OFFSET;
    if ((ULONG_PTR)branch & 1) return 0;
    DWORD old_protection, unused;
    if (!VirtualProtect(branch, 2, PAGE_EXECUTE_READWRITE, &old_protection)) return 0;
    SHORT previous = InterlockedCompareExchange16((volatile SHORT *)branch,
                                                  (SHORT)0x9090, (SHORT)0x1e74);
    BOOL flushed = FlushInstructionCache(GetCurrentProcess(), branch, 2);
    BOOL restored = VirtualProtect(branch, 2, old_protection, &unused);
    if (previous != (SHORT)0x1e74) return 0;
    return flushed && restored ? 1 : 2;
}

static DWORD WINAPI initialize(void *unused) {
    (void)unused;
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->FileHeader.TimeDateStamp != TARGET_TIMESTAMP ||
        nt->OptionalHeader.SizeOfImage != TARGET_IMAGE_SIZE || !correct_executable()) {
        log_status("NOT APPLIED: unsupported executable. Requires the verified Windows x64 1.26.0.02 build.\r\n");
        return 0;
    }
    int result = patch_window(base + TARGET_RVA);
    log_status(result == 1 ?
        "APPLIED: panorama Screen Animations gate bypassed at RVA 0x206302c. Executable on disk unchanged. In-game behavior must be checked.\r\n" :
        result == 2 ? "APPLIED WITH WARNING: instruction-cache flush or protection restore failed. Restart Minecraft.\r\n" :
        "NOT APPLIED: original instruction guard failed or memory protection could not be changed.\r\n");
    return 0;
}

#ifndef SELFTEST
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        self_module = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(NULL, 0, initialize, NULL, 0, NULL);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
#else
#include <stdio.h>
int main(void) {
    unsigned char *p = VirtualAlloc(NULL, 4096, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!p) return 1;
    memcpy(p, expected_window, sizeof(expected_window));
    p[0] ^= 1;
    if (patch_window(p) != 0 || p[BRANCH_OFFSET] != 0x74) return 2;
    memcpy(p, expected_window, sizeof(expected_window));
    DWORD old;
    if (!VirtualProtect(p, 4096, PAGE_EXECUTE_READ, &old)) return 3;
    if (patch_window(p) != 1) return 4;
    for (unsigned i = 0; i < sizeof(expected_window); ++i) {
        unsigned char want = (i == BRANCH_OFFSET || i == BRANCH_OFFSET+1) ? 0x90 : expected_window[i];
        if (p[i] != want) return 5;
    }
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(p, &mbi, sizeof(mbi));
    if (mbi.Protect != PAGE_EXECUTE_READ || patch_window(p) != 0) return 6;
    if (correct_executable()) return 7;
    VirtualFree(p, 0, MEM_RELEASE);
    puts("PASS: changed-byte rejection, two-byte-only patch, memory-protection restoration, repeated-patch rejection, wrong-executable rejection.");
    return 0;
}
#endif
