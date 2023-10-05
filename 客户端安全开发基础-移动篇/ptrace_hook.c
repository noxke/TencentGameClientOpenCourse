#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>

#if defined(__arm__)
    const char *arch = "arm";
#elif defined(__i386__)
    const char *arch = "x86";
#endif

const char *hack_str = "Hacked by noxke";

#if defined(__arm__)
void hook_arm(pid_t pid, unsigned long lib_base)
{
    int status;
    struct user_regs regs;
    unsigned int ori_code;
    unsigned int bp_code = 0x0000BEFF;  // 设置FF号断点
    // 需要hook的函数偏移
    unsigned long proc_offset = 0x8ca6;
    // 需要hook的函数地址
    unsigned long proc_addr = lib_base + proc_offset;
    // 附加到目标进程
    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    waitpid(pid, &status, 0);
    // 先保存断点前的指令，再将其设置为断点代码
    ori_code = ptrace(PTRACE_PEEKDATA, pid, (void *)proc_addr, NULL);
    ptrace(PTRACE_POKEDATA, pid, (void *)proc_addr, (void *)bp_code);
    printf("set breakpoint at 0x%lx\n", proc_addr);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    // 捕获断点
    while (1)
    {
        waitpid(pid, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP)
        {
            // 获取寄存器信息
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            unsigned long pc = regs.uregs[15];
            printf("break at 0x%lx\n", pc);
            if (pc == proc_addr)
            {
                // 获取R0寄存器
                unsigned long r0 = regs.uregs[0];
                unsigned long str_addr = ptrace(PTRACE_PEEKDATA, pid, (void *)(r0 + 8), NULL);
                char ori_str[0x20];
                char new_str[0x20];
                strcpy(new_str, hack_str);
                for (int i = 0; i < 0x20; i += 4)
                {
                    *(unsigned long *)(ori_str + i) = ptrace(PTRACE_PEEKDATA, pid, (void *)(str_addr + i), NULL);
                    ptrace(PTRACE_POKEDATA, pid, (void *)(str_addr + i), (void *)(*(unsigned long *)(new_str + i)));
                }
                printf("str at 0x%lx\n", str_addr);
                printf("ori str: %s\n", ori_str);
                printf("new str: %s\n", new_str);
                // 恢复原来的指令
                ptrace(PTRACE_POKEDATA, pid, (void *)proc_addr, (void *)ori_code);
                break;
            }
        }
        ptrace(PTRACE_CONT, pid, NULL, NULL);
    }
    // 分离目标进程
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
}
#endif
#if defined(__i386__)
void hook_x86(pid_t pid, unsigned long lib_base)
{
    int status;
    struct user_regs_struct regs;
    unsigned int ori_code;
    unsigned int bp_code = 0x000000CC;  // 设置int3断点
    // 需要hook的函数偏移
    unsigned long proc_offset = 0x8dc0;
    // 需要hook的函数地址
    unsigned long proc_addr = lib_base + proc_offset;
    // 附加到目标进程
    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    waitpid(pid, &status, 0);
    // 先保存断点前的指令，再将其设置为断点代码
    ori_code = ptrace(PTRACE_PEEKDATA, pid, (void *)proc_addr, NULL);
    ptrace(PTRACE_POKEDATA, pid, (void *)proc_addr, (void *)bp_code);
    printf("set breakpoint at 0x%lx\n", proc_addr);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    // 捕获断点
    while (1)
    {
        waitpid(pid, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP)
        {
            // 获取寄存器信息
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            unsigned long eip = regs.eip;
            printf("break at 0x%lx\n", eip - 1);
            if (eip == proc_addr + 1)
            {
                // x86与arm不同，执行断点后EIP指向吓一条指令，需要将EIP恢复
                regs.eip = regs.eip - 1;
                ptrace(PTRACE_SETREGS, pid, NULL, &regs);
                // 获取edx寄存器
                unsigned long edx = regs.edx;
                unsigned long str_addr = ptrace(PTRACE_PEEKDATA, pid, (void *)(edx + 8), NULL);
                char ori_str[0x20];
                char new_str[0x20];
                strcpy(new_str, hack_str);
                for (int i = 0; i < 0x20; i += 4)
                {
                    *(unsigned long *)(ori_str + i) = ptrace(PTRACE_PEEKDATA, pid, (void *)(str_addr + i), NULL);
                    ptrace(PTRACE_POKEDATA, pid, (void *)(str_addr + i), (void *)(*(unsigned long *)(new_str + i)));
                }
                printf("str at 0x%lx\n", str_addr);
                printf("ori str: %s\n", ori_str);
                printf("new str: %s\n", new_str);
                // 恢复原来的指令
                ptrace(PTRACE_POKEDATA, pid, (void *)proc_addr, (void *)ori_code);
                break;
            }
        }
        ptrace(PTRACE_CONT, pid, NULL, NULL);
    }
    // 分离目标进程
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
}
#endif

int main()
{
    // 目标进程pid
    pid_t demo_pid = 0;
    // 目标进程中libnative-llib.so加载的基地址
    unsigned long lib_base = 0;
    FILE *fp = NULL;
    char cli_output[1024] = {'\0'};
    // 获取进程pid和libnative-lib.so加载的基地址
    while (1)
    {
        fp = popen("pidof com.example.x86demo && cat /proc/`pidof com.example.x86demo`/maps | grep libnative-lib.so", "r");
        fread(cli_output, 1, 1024, fp);
        pclose(fp);
        if (strlen(cli_output) != 0)
        {
            sscanf(cli_output, "%d%lx", &demo_pid, &lib_base);
            if (lib_base == 0)
            {
                continue;
            }
            printf("demo_pid: %d\n", demo_pid);
            printf("libnative-lib.so image base: 0x%lx\n", lib_base);
            break;
        }
    }
    #if defined(__arm__)
        hook_arm(demo_pid, lib_base);
    #elif defined(__i386__)
        hook_x86(demo_pid, lib_base);
    #endif
    return 0;
}