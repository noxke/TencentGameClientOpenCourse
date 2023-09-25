// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <iostream>
#include <Windows.h>

uint8_t shellcode[0x50] = {
 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
 0x8B, 0x01, 0x3D, 0xB4, 0xED, 0xCE, 0xF3, 0x75, 0x07, 0xB8, 0xD5, 0xFD, 0xC8, 0xB7, 0x89, 0x01,
 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x89, 0x54, 0x24, 0x10, 0x4C, 0x89, 0x44, 0x24, 0x18, 0x4C,
 0x89, 0x4C, 0x24, 0x20, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xc3,
 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
/*

nop
nop
mov eax, [rcx]  // 错误或正确两个字符长4个字节
cmp eax, 0xF3CEEDB4 // 比较是不是错误
jnz $+0x8  // 不是错误跳转
mov eax, 0xB7C8FDD5
mov [rcx], eax  // 将正确写入内存
mov [rsp+0x8], rcx  // 向printf函数传参
mov [rsp+0x10], rdx
mov [rsp+0x18], r8
mov [rsp+0x20], r9
mov rax, printf+0x10    // 返回到原printf函数
push rax
ret
nop
nop
*/

uint8_t hookcode[20] = {0x90, 0x90, 0x90, 0x90, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xc3, 0x90, 0x90, 0x90, 0x90};
/*
nop
nop
nop
nop
mov rax, shellcode_addr // 跳转到shellcode代码
push rax
ret
*/

void set_hook()
{
    // hook printf函数
    DWORD64 pProc;
    DWORD64 pProcOffset = 0x1020;
    HMODULE hModule = GetModuleHandle(nullptr);
    DWORD_PTR baseAddress = reinterpret_cast<DWORD_PTR>(hModule);
    pProc = (DWORD64)baseAddress + pProcOffset;

    printf("\nhook proc addr : 0x%llx\n", pProc);
    // 修改shellcode中的返回地址 printf+0x10
    *((DWORD64 *)((uint8_t *)shellcode + 0x50 - 0x1A)) = (pProc + 0x10);
    // 写入shellcode
    LPVOID shellcodeBuf = VirtualAlloc(NULL, 0x50, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(shellcodeBuf, shellcode, 0x50);
    DWORD64 dwShellcodeBuf = (DWORD64)shellcodeBuf;
    printf("\nshell code addr : 0x%llx\n", dwShellcodeBuf);
    // 修改函数内存保护权限
    VirtualProtect((LPVOID)pProc, 20, PAGE_EXECUTE_READWRITE, NULL);
    // 修改hookcode中的shellcode地址
    *((DWORD64 *)((uint8_t *)hookcode + 6)) = dwShellcodeBuf;
    // 获取进程句柄
    HANDLE hProcess = GetCurrentProcess();
    // hook printf函数
    WriteProcessMemory(hProcess, (LPVOID)pProc, hookcode, 20, NULL);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // printf("injected!");
        set_hook();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

