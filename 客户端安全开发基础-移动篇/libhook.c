#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <errno.h>

void onload() __attribute__((constructor));

void (*__android_log_print)(int i, ...);
void *handle;
#define TAG "Crack"
#define LOGD(...) __android_log_print(3, TAG, __VA_ARGS__)  
void *(*__memcpy)(void *dst, void *src, size_t n);

int hookProc()
{
    LOGD("hook and return TRUE");
    return 1;
}

void sethook()
{
    // 首先获取libcrackme1.so加载的基地址
    void *lib_base = NULL;
    unsigned long proc_offset = 0x1134;
    void *proc_addr;
    FILE *fp;
    char line[0x100];
    fp = fopen("/proc/self/maps", "rt");
    fgets(line, 0x100, fp);
    while (strlen(line) != 0)
    {
        if (strstr(line, "libcrackme1.so") != NULL)
        {
            sscanf(line, "%lx", (unsigned long*)&lib_base);
            break;
        }
        fgets(line, 0x100, fp);
    }
    fclose(fp);
    if (lib_base == NULL)
    {
        LOGD("get libcrackme1.so base failed");
        return;
    }
    LOGD("libcrackme1.so at 0x%lx", (unsigned long)lib_base);
    proc_addr = (void *)((unsigned long)lib_base + proc_offset);
    LOGD("hook proc at 0x%lx", (unsigned long)proc_addr);
    LOGD("new proc at 0x%lx", (unsigned long)hookProc);
    // LDR R0, [PC+8]
    // MOV PC, R0
    // hookProc
    // 修改段保护
    if (mprotect(lib_base, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        LOGD("mprotect failed");
        perror("mprotect");
        LOGD("Error: %s\n", strerror(errno));
    }
    unsigned char jumpCode[0xc] = {0x00, 0x00, 0x9F, 0xE5, 0x00, 0xF0, 0xA0, 0xE1};
    *(unsigned long *)(jumpCode + 8) = (unsigned long)hookProc;
    __memcpy(proc_addr, (void *)jumpCode, 0xc);
}

void onload()
{
    // 设置LOGD
    handle = dlopen("/system/lib/liblog.so", RTLD_LAZY);
    __android_log_print = (void (*)(int, ...))dlsym(handle, "__android_log_print");
    dlclose(handle);
    // 获取memcpy
    handle = dlopen("/apex/com.android.runtime/lib/bionic/libc.so", RTLD_LAZY);
    __memcpy = (void *(*)(void *, void *, size_t))dlsym(handle, "memcpy");
    dlclose(handle);

    LOGD("injected by noxke");
    sethook();
    return;
}