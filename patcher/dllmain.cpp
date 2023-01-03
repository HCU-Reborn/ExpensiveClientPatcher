#include <Windows.h>
#include <stdio.h>
#include "pch.h"
#include <iostream>
#include "minhook/minhook.h"
#include <winsock.h>
#pragma comment(lib, "ws2_32.lib")
//with love by garuda aka eternal darkness :)

typedef int(WINAPI* connectFn)(SOCKET socket, const sockaddr* name, int namelen);
connectFn original_connect;

typedef HRESULT(WINAPI* URLOpenBlockingStreamFn)(
    LPUNKNOWN            pCaller,
    LPCSTR               szURL,
    LPSTREAM* ppStream,
    _Reserved_ DWORD                dwReserved,
    LPBINDSTATUSCALLBACK lpfnCB);
URLOpenBlockingStreamFn original_open;

HRESULT WINAPI hooked_open(
    LPUNKNOWN            pCaller,
    LPCSTR               szURL,
    LPSTREAM* ppStream,
    _Reserved_ DWORD                dwReserved,
    LPBINDSTATUSCALLBACK lpfnCB) {
    szURL = "http://127.0.0.1/";
    return original_open(pCaller, szURL, ppStream, dwReserved, lpfnCB);
}

int WINAPI hooked_connect(SOCKET socket, const sockaddr* name, int namelen)
{
    //спиздил у миронова ээээ
    if (!name) return original_connect(socket, name, namelen);

    sockaddr_in sin;
    memcpy(&sin, name, sizeof(sin));

    char addr[128];
    sprintf_s(addr, "%s:%d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));


    if (!strcmp(addr, "81.163.28.102:2817"))
    {
        sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        sin.sin_port = ntohs(2817);
        printf("\n[HCU] Successfully redirected request to localhost!\n");
        return original_connect(socket, (sockaddr*)&sin, namelen);
    }

    return original_connect(socket, name, namelen);
}

const char* Hook()
{
    if (MH_Initialize() != MH_OK)
        return "[HCU] MH_Initialize failed";

    if (MH_CreateHookApi(L"Ws2_32.dll", "connect", hooked_connect, (LPVOID*)&original_connect) != MH_OK)
        return "[HCU] connect hook failed";

    if (MH_CreateHookApi(L"Urlmon.dll", "URLOpenBlockingStreamA", hooked_open, (LPVOID*)&original_open) != MH_OK)
        return "[HCU] blockingstream hook failed";

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
        return "[HCU] MH_EnableHook failed";

    return nullptr;
}

bool DataCompare(BYTE* data, BYTE* pattern, const char* mask)
{
    for (; *mask; ++mask, ++data, ++pattern)
    {
        if (*mask == 'x' && *data != *pattern)
            return false;
    }
    return (*mask) == NULL;
}

BYTE* FindPattern(BYTE* base, DWORD size, BYTE* pattern, const char* mask)
{
    for (DWORD i = 0; i < size; i++)
    {
        if (DataCompare((BYTE*)(base + i), pattern, mask))
            return (BYTE*)(base + i);
    }
    return NULL;
}

DWORD WINAPI LOL(LPVOID lpParam)
{
    ShellExecuteA(NULL, "open", "https://discord.gg/Ghq2zDhYtg", NULL, NULL, SW_RESTORE);
    printf("\n[OWNED] Cracked by HCU & SUBI & SCRAIT & DREAM1X");
    LoadLibraryA("Urlmon.dll"); //на всякий случай чо

    const auto hooks = Hook();

    if (hooks)
    {
        printf("\n[HCU] Hooking failed [%s]\n", hooks);
    }

    Sleep(8000);//слипаем чтоб не ебануло нахуй

    //это пиздец
    DWORD old_protect = 0;
    auto ntdll = GetModuleHandleA("ntdll.dll");
    BYTE callcode = ((BYTE*)GetProcAddress(ntdll, "NtQuerySection"))[4] - 1;
    BYTE restore[] = { 0x4C, 0x8B, 0xD1, 0xB8, callcode };

    auto nt_vp = (BYTE*)GetProcAddress(ntdll, "NtProtectVirtualMemory");
    VirtualProtect(nt_vp, sizeof(restore), PAGE_EXECUTE_READWRITE, &old_protect);
    memcpy(nt_vp, restore, sizeof(restore));
    VirtualProtect(nt_vp, sizeof(restore), old_protect, &old_protect);

    BYTE* pattern1 = (BYTE*)"\x48\x8d\x0d\x00\x00\x00\x00\x52\xe8\x00\x00\x00\x00\xb9"; //lea
    BYTE* base = (BYTE*)GetModuleHandleA(NULL);
    DWORD size = 0x1000000;
    BYTE* result1 = FindPattern(base, size, pattern1, "xxx????xx????x");

    BYTE* pattern2 = (BYTE*)"\x76\x40\x4c\x8d\x4d";//jne
    BYTE* result2 = FindPattern(base, size, pattern2, "xxxxx");


    // патчим
    VirtualProtect(result1, 5, PAGE_EXECUTE_READWRITE, &old_protect);
    HANDLE hProcess = GetCurrentProcess();
    BYTE patch[5] = { 0xE9, 0x00, 0x00, 0x00, 0x00 }; // JMP
    *(DWORD*)(patch + 1) = (DWORD)(result2 - result1 - 5);
    SIZE_T written;
    if (WriteProcessMemory(hProcess, result1, patch, sizeof(patch), &written)) {
        //sosi
        printf("\n[HCU] Successfully patched serj's op protection\n[HCU] Please, enter any key: ");
    }
    else {
        //рот ебал
        printf("\n[HCU] Something went completely wrong!");
    }

    VirtualProtect(result1, 5, old_protect, &old_protect);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        CreateThread(NULL, 0, &LOL, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

