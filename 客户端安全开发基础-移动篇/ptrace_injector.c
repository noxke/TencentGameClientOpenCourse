#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <link.h>
#include <dlfcn.h>

// 程序使用的libc路径
#define libc_path "/apex/com.android.runtime/lib/bionic/libc.so"
#define libdl_path "/apex/com.android.runtime/lib/bionic/libdl.so"

// ptrace附加到进程
int ptrace_attach(pid_t pid)
{
    int status;
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
    {
        return -1;
    }
    waitpid(pid, &status, 0);
    printf("attach to process pid: %d\n", pid);
    return 0;
}

// ptrace脱离进程
int ptrace_detach(pid_t pid)
{
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0)
    {
        return -1;
    }
    printf("detach from process pid: %d\n", pid);
    return 0;
}

// ptrace恢复进程运行
int ptrace_continue(pid_t pid)
{
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) < 0)
    {
        return -1;
    }
    // printf("process continue\n");
    return 0;
}

// ptrace读数据
int ptrace_read(pid_t pid, void *addr, size_t len, void *data_buf)
{
    unsigned int data;
    size_t read_len;
    for (read_len = 0; read_len < len; read_len += 4)
    {
        data = ptrace(PTRACE_PEEKDATA, pid, (void *)(addr + read_len), NULL);
        if (len - read_len >= 4)
        {
            *(unsigned int *)(data_buf + read_len) = data;
        }
        else
        {
            for (int i = 0; i < len - read_len; i++)
            {
                *(unsigned char *)(data_buf + read_len + i) = *(((unsigned char *)(&data)) + i);
            }
        }
    }
    printf("read 0x%x bytes, start-addr: 0x%lx\n", len, (unsigned long)addr);
    return 0;
}

// ptrace写数据
int ptrace_write(pid_t pid, void *addr, size_t len, void *data)
{
    unsigned int tmp_data;
    size_t write_len;
    for (write_len = 0; write_len < len; write_len += 4)
    {
        if (len - write_len >= 4)
        {
            tmp_data = *(unsigned int *)(data + write_len);
        }
        else
        {
            // 不足4个字节时需要先读取原数据
            tmp_data = ptrace(PTRACE_PEEKDATA, pid, (void *)(addr + write_len), NULL);
            for (int i = 0; i < len - write_len; i++)
            {
                *(((unsigned char *)(&tmp_data)) + i) = *(unsigned char *)(data + write_len + i);
            }
        }
        if (ptrace(PTRACE_POKEDATA, pid, (void *)(addr + write_len), (void *)(tmp_data)) < 0)
        {
            return -1;
        }
    }
    printf("write 0x%x bytes, start-addr: 0x%lx\n", len, (unsigned long)addr);
    return 0;
}

// 读取寄存器
int ptrace_getregs(pid_t pid, struct user_regs *regs)
{
    if (ptrace(PTRACE_GETREGS, pid, NULL, (void *)regs) < 0)
    {
        return -1;
    }
    // printf("get registers\n");
    return 0;
}

// 设置寄存器
int ptrace_setregs(pid_t pid, struct user_regs *regs)
{
    if (ptrace(PTRACE_SETREGS, pid, NULL, (void *)regs) < 0)
    {
        return -1;
    }
    // printf("set registers\n");
    return 0;
}

// ptrace调用进程函数
void *ptrace_call(pid_t pid, void *proc_addr, int param_num, void *params[])
{
    /*
    前4个参数依次储存到R0～R3，超过4个参数从右向左依次压入栈中
    返回值在R0中
    LR寄存器为返回地址
    手动传参后修改PC寄存器为调用函数的地址，修改LR寄存器为0,使函数返回时，触发异常，获取函数返回值
    */
    int status;
    struct user_regs saved_regs;
    struct user_regs regs;
    void *ret;
    ptrace_getregs(pid, &saved_regs);
    memcpy(&regs, &saved_regs, sizeof(struct user_regs));
    // 设置参数
    regs.uregs[13] -= 0x50;
    if (param_num >= 1)
    {
        // R0
        printf("proc 0x%lx param 0 value: 0x%lx\n", (unsigned long)proc_addr, (unsigned long)params[0]);
        regs.uregs[0] = (unsigned long)params[0];
    }
    if (param_num >= 2)
    {
        // R1
        printf("proc 0x%lx param 1 value: 0x%lx\n", (unsigned long)proc_addr, (unsigned long)params[1]);
        regs.uregs[1] = (unsigned long)params[1];
    }
    if (param_num >= 3)
    {
        // R2
        printf("proc 0x%lx param 2 value: 0x%lx\n", (unsigned long)proc_addr, (unsigned long)params[2]);
        regs.uregs[2] = (unsigned long)params[2];
    }
    if (param_num >= 4)
    {
        // R3
        printf("proc 0x%lx param 3 value: 0x%lx\n", (unsigned long)proc_addr, (unsigned long)params[3]);
        regs.uregs[3] = (unsigned long)params[3];
        for (int i = 0; i < param_num - 4; i++)
        {
            // 剩余的参数压入栈中
            void *p = params[param_num - 1 - i];
            printf("proc 0x%lx param %d value: 0x%lx\n", (unsigned long)proc_addr, (param_num - 1 - i), (unsigned long)p);
            // SP
            regs.uregs[13] -= 4;
            ptrace_write(pid, (void *)regs.uregs[13], 4, &p);
        }
    }
    // PC寄存器，低位决定处理器模式
    regs.uregs[15] = ((unsigned long)proc_addr & 0xFFFFFFFE);
    // PSR寄存器，第5位决定Thumb模式,根据函数地址最低位确定是否进入Thumb模式
    if ((unsigned long)proc_addr & 0x1)
    {
        // Thumb模式
        regs.uregs[16] = regs.uregs[16] | 0x20;
    }
    else
    {
        // arm模式
        regs.uregs[16] = regs.uregs[16] & 0xFFFFFFDF;
    }
    // LR寄存器
    regs.uregs[14] = 0;
    if (ptrace_setregs(pid, &regs) != 0 || ptrace_continue(pid) != 0)
    {
        printf("call proc 0x%lx failed\n", (unsigned long)proc_addr);
        return NULL;
    }
    // 等待函数执行完返回错误
    waitpid(pid, &status, WUNTRACED);
    // printf("0x%x\n", status);
    while (status != 0xb7f)
    {
        ptrace_continue(pid);
        waitpid(pid, &status, WUNTRACED);
    }
    ptrace_getregs(pid, &regs);
    // 返回值在R0中
    ret = (void *)regs.uregs[0];
    // 恢复寄存器
    ptrace_setregs(pid, &saved_regs);
    printf("proc 0x%lx return value: 0x%lx\n", (unsigned long)proc_addr, (unsigned long)ret);
    return ret;
}

// 获取lib文件加载基地址
void *get_module_base(pid_t pid, const char *lib_path)
{
    FILE *fp = NULL;
    char maps_path[1024] = {'\0'};
    char maps_line[1024] = {'\0'};
    void *module_base = NULL;
    if (pid != 0)
    {
        sprintf(maps_path, "/proc/%d/maps", pid);
    }
    else
    {
        strcpy(maps_path, "/proc/self/maps");
    }
    fp = fopen(maps_path, "rt");
    while (fgets(maps_line, 1024, fp) != NULL)
    {
        if (strstr(maps_line, lib_path) != NULL)
        {
            sscanf(maps_line, "%lx", (unsigned long *)(&module_base));
            break;
        }
    }
    fclose(fp);
    if (module_base == NULL)
    {
        printf("%s not found in process %d\n", lib_path, pid);
        return NULL;
    }
    printf("%s at 0x%lx\n", lib_path, (unsigned long)module_base);
    return module_base;
}

// 获取proc函数的内存地址
void *get_remote_proc_addr(pid_t pid, const char *lib_path, const char *proc_name)
{
    void *handle;
    void *local_module_base;
    void *local_proc_addr;
    unsigned long proc_offset;
    void *remote_module_base;
    void *remote_proc_addr;
    // 在本地加载lib文件并获取proc函数偏移
    handle = dlopen(lib_path, RTLD_LAZY);
    if (handle == NULL)
    {
        printf("open %s failed\n", lib_path);
        return NULL;
    }
    dlerror();
    local_module_base = get_module_base(0, lib_path);
    local_proc_addr = dlsym(handle, proc_name);
    proc_offset = local_proc_addr - local_module_base;
    dlclose(handle);
    printf("%s offset: 0x%lx\n", proc_name, proc_offset);
    remote_module_base = get_module_base(pid, lib_path);
    remote_proc_addr = remote_module_base + proc_offset;
    printf("%s address: 0x%lx\n", proc_name, (unsigned long)remote_proc_addr);
    return remote_proc_addr;
}

// 注入so到进程
int inject_lib(pid_t pid, const char *lib_path)
{
    int status;
    void *params[10];
    void *proc_malloc;
    void *proc_dlopen;
    void *proc_dlerror;
    void *mem_buf;
    proc_malloc = get_remote_proc_addr(pid, libc_path, "malloc");
    proc_dlopen = get_remote_proc_addr(pid, libdl_path, "dlopen");
    proc_dlerror = get_remote_proc_addr(pid, libdl_path, "dlerror");
    // 首先附加到进程
    ptrace_attach(pid);
    // 调用malloc分配内存写入lib文件路径
    params[0] = (void *)0x100;
    mem_buf = ptrace_call(pid, proc_malloc, 1, params);
    if (mem_buf == NULL)
    {
        printf("malloc memory failed\n");
        ptrace_continue(pid);
        ptrace_detach(pid);
        return -1;
    }
    printf("malloc memory at 0x%lx\n", (unsigned long)mem_buf);
    // 将lib路径写入进程内存
    ptrace_write(pid, mem_buf, strlen(lib_path) + 1, (void *)lib_path);
    // 调用dlopen将lib文件加载到进程
    params[0] = mem_buf;
    params[1] = (void *)RTLD_LAZY;
    if (ptrace_call(pid, proc_dlopen, 2, params) == NULL)
    {
        printf("dlopen load %s to process %d failed\n", lib_path, pid);
        // 调用dlerror查看错误原因
        void *err_addr = ptrace_call(pid, proc_dlerror, 0, params);
        unsigned char err[0x100];
        ptrace_read(pid, err_addr, 0x100, (void *)err);
        printf("%s\n", err);
        ptrace_continue(pid);
        ptrace_detach(pid);
        return -1;
    }
    printf("dlopen load %s to process %d succeed\n", lib_path, pid);
    ptrace_continue(pid);
    ptrace_detach(pid);
    return 0;
}

int main(int argc, const char *argv[])
{
    // 目标进程pid
    pid_t process_pid = 0;
    FILE *fp = NULL;
    char cli_cmd[1024] = {'\0'};
    char cli_output[1024] = {'\0'};
    if (argc != 3)
    {
        printf("usage:\n%s process_name lib_path\n", argv[0]);
        return 0;
    }
    // 获取进程pid
    sprintf(cli_cmd, "pidof %s", argv[1]);
    fp = popen(cli_cmd, "r");
    fread(cli_output, 1, 1024, fp);
    pclose(fp);
    if (strlen(cli_output) == 0)
    {
        printf("process not found: %s\n", argv[1]);
        return -1;
    }
    sscanf(cli_output, "%d", &process_pid);
    printf("pid of %s: %d\n", argv[1], process_pid);
    inject_lib(process_pid, argv[2]);
    return 0;
}