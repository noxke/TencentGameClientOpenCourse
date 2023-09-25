#include <stdio.h>
#include <Windows.h>
#include <processthreadsapi.h>
#include <WinUser.h>
#include <TlHelp32.h>
#include <Psapi.h>

#define MAX_LEN 0x100

DWORD getDwPidByName(LPCWSTR procName)
{
    // 使用tlhelp32获取进程PID
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, (DWORD)0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    LPWSTR fileNameBuf = (LPWSTR)malloc(sizeof(WCHAR) * MAX_LEN);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    Process32First(hSnapshot, &pe32);
    do
    {
        DWORD dwPID = pe32.th32ProcessID;
        // printf("dwPID: %d\n", dwPID);
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID);
        if (hProcess == NULL)
        {
            // printf("can't open process %d\n", dwPID);
            continue;
        }
        GetProcessImageFileName(hProcess, fileNameBuf, (DWORD)MAX_LEN);
        // 提取进程名
        WCHAR *pFileName = wcsrchr(fileNameBuf, L'\\');
        pFileName++;
        // wprintf(pFileName);
        // putchar('\n');
        if (!wcscmp(procName, pFileName))
        {
            free((void *)fileNameBuf);
            return dwPID;
        }
    } while (Process32Next(hSnapshot, &pe32));
    free((void *)fileNameBuf);
    return 0;
}


int main(int argc, const char *argv[])
{
    DWORD dwPID;    // 进程pid
    HANDLE hProcess;    // 进程句柄
    HANDLE hThread; // 创建的远程线程句柄
    HMODULE hMod;   // kernel32.dll句柄
    LPVOID pLoadLibrary;  // LoadLibrary函数地址
    LPVOID remoteMemAddr;   // 分配的远程进程内存
    WCHAR procName[MAX_LEN];    // 注入程序名
    char dllPath[MAX_LEN]; // dll位置
    if (argc != 3)
    {
        printf("usage:\n%s processName dllPath", argv[0]);
        return 0;
    }
    // 拷贝ddl路径
    strcpy_s(dllPath, argv[2]);
    // 将进程名转换为宽字符
    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        (LPCCH)argv[1],
                        strlen(argv[1]) + 1,
                        procName,
                        (int)((strlen(argv[1]) + 1) * sizeof(WCHAR)));
    // 获取进程PID
    dwPID = getDwPidByName(procName);
    if (dwPID == 0)
    {
        printf("can't get PID of %s\n", argv[1]);
        return 0;
    }
    printf("%s PID is %d\n", argv[1], dwPID);

    // 获取进程句柄
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
    if (hProcess == NULL)
    {
        printf("open process failed\n");
        return 0;
    }
    // 获取LoadLibrary函数地址
    hMod = GetModuleHandleA((LPCSTR)"kernel32.dll");
    pLoadLibrary = (LPVOID)GetProcAddress(hMod, (LPCSTR)"LoadLibraryA");
    // 分配内存
    remoteMemAddr = VirtualAllocEx(hProcess, NULL, (SIZE_T)MAX_LEN, MEM_COMMIT, PAGE_READWRITE);
    if (remoteMemAddr == NULL)
    {
        printf("alloc memory failed\n");
        return 0;
    }
    printf("alloc memory at 0x%llx\n", remoteMemAddr);
    // 将dll文件位置写入进程内存
    if (!WriteProcessMemory(hProcess, remoteMemAddr, LPVOID(dllPath), (SIZE_T)MAX_LEN, NULL))
    {
        printf("write memory failed\n");
        return 0;
    }
    // 创建远程线程
    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, remoteMemAddr, 0, NULL);
    if (hThread == NULL)
    {
        printf("create remote thread failed\n");
        return 0;
    }
    printf("inject succeed\n");
    return 0;
}
