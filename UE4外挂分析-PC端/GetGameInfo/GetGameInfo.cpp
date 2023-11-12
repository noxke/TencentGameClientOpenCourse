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

LPVOID getModuleBase(DWORD dwPID)
{
    // 创建快照，获取进程模块信息
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPID);
    LPVOID baseAddress = NULL;
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(MODULEENTRY32);

        // 遍历进程模块
        if (Module32First(hSnapshot, &moduleEntry))
        {
            // 获取可执行文件模块的基地址
            baseAddress = (LPVOID)moduleEntry.modBaseAddr;
        }
        // 关闭快照句柄
        CloseHandle(hSnapshot);
    }
    return baseAddress;
}

int main()
{
    LPCWSTR procName = L"ShooterClient.exe";
    DWORD dwPID;
    HANDLE hProcess;
    LPVOID baseAddr;
    dwPID = getDwPidByName(procName);
    printf("PID: %d\n", dwPID);
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
    if (hProcess == NULL)
    {
        printf("open process failed\n");
        return 0;
    }
    baseAddr = getModuleBase(dwPID);
    printf("proc base: 0x%llx\n", baseAddr);

    LPVOID UWorld;
    LPVOID GName;
    LPVOID GameInstance;
    LPVOID ULocalPlayer;
    LPVOID LocalPlayer;
    LPVOID PlayerController;
    LPVOID PlayerActor;
    LPVOID PlayerPosition;
    LPVOID ULevel;
    DWORD ActorCount;
    LPVOID ActorArray;
    // 读取UWorld
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)baseAddr + 0x02F71060), (LPVOID)&UWorld, 8, NULL);
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)UWorld + 0x160), (LPVOID)&GameInstance, 8, NULL);
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)GameInstance + 0x38), (LPVOID)&ULocalPlayer, 8, NULL);
    ReadProcessMemory(hProcess, (LPVOID)ULocalPlayer, (LPVOID)&LocalPlayer, 8, NULL);
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)LocalPlayer + 0x30), (LPVOID)&PlayerController, 8, NULL);
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)PlayerController + 0x3B0), (LPVOID)&PlayerActor, 8, NULL);
    printf("\n");
    printf("UWorld: 0x%llx\n", UWorld);
    printf("GameInstance: 0x%llx\n", GameInstance);
    printf("ULocalPlayer: 0x%llx\n", ULocalPlayer);
    printf("LocalPlayer: 0x%llx\n", LocalPlayer);
    printf("PlayerController: 0x%llx\n", PlayerController);
    printf("PlayerActor: 0x%llx\n", PlayerActor);
    printf("\n");
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)UWorld + 0x30), (LPVOID)&ULevel, 8, NULL);
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)ULevel + 0xA0), (LPVOID)&ActorCount, 4, NULL);
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)ULevel + 0x98), (LPVOID)&ActorArray, 8, NULL);
    // 读取玩家坐标
    FLOAT posi[3];
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)PlayerActor + 0x3A0), (LPVOID)&PlayerPosition, 8, NULL);
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)PlayerPosition+0x1A0), (LPVOID)posi, 0xC, NULL);
    printf("\n");
    printf("posi: [%f, %f, %f]\n", posi[0], posi[1], posi[2]);
    // 读取玩家视角
    FLOAT persp_x, persp_y;
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)PlayerPosition + 0x154), (LPVOID)&persp_x, 0x4, NULL);
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)PlayerPosition + 0x174), (LPVOID)&persp_y, 0x4, NULL);
    printf("perspective: [%f, %f]\n", persp_x, persp_y);
    printf("\n");
    printf("ULevel: 0x%llx\n", ULevel);
    printf("ActorCount: %d\n", ActorCount);
    printf("ActorArray: 0x%llx\n", ActorArray);
    // 读取GName
    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)baseAddr + 0x2E6E0C0), (LPVOID)&GName, 8, NULL);
    printf("GName: 0x%llx\n", GName);
    printf("\n");
    // 遍历ActorArry
    for (DWORD i = 0; i < ActorCount; i++)
    {
        LPVOID AActor;
        DWORD id;
        LPVOID PNameArray;
        LPVOID PName;
        CHAR name[0x100];
        ReadProcessMemory(hProcess, (LPVOID)((BYTE*)ActorArray + i * 8), (LPVOID)&AActor, 8, NULL);
        if (ReadProcessMemory(hProcess, (LPVOID)((BYTE*)AActor + 0x18), (LPVOID)&id, 4, NULL))
        {
            ReadProcessMemory(hProcess, (LPVOID)((BYTE*)GName + (id / 0x4000) * 8), (LPVOID)&PNameArray, 8, NULL);
            ReadProcessMemory(hProcess, (LPVOID)((BYTE*)PNameArray + (id % 0x4000) * 8), (LPVOID)&PName, 8, NULL);
            if (ReadProcessMemory(hProcess, (LPVOID)((BYTE*)PName + 0xC), (LPVOID)name, 0x100, NULL))
            {
                // printf("%d: %s\n", i, name);
                if (!strcmp(name, "BotPawn_C"))
                {
                    LPVOID botPosition;
                    FLOAT botPosi[3];
                    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)AActor + 0x3A0), (LPVOID)&botPosition, 8, NULL);
                    ReadProcessMemory(hProcess, (LPVOID)((BYTE*)botPosition + 0x1A0), (LPVOID)botPosi, 0xC, NULL);
                    printf("bot: [%f, %f, %f] \n", botPosi[0], botPosi[1], botPosi[2]);
                }
            }
        }
    }
    return 0;
}
