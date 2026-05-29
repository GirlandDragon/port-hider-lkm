#include "port_hider.h"

static bool kp_v4c_ok;
static bool kp_v6c_ok;

static int h_connect4_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)regs->regs[1];
    if (!addr || addr->sin_family != AF_INET)
        return 0;
    if (addr->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
        return 0;
    if (is_hidden_port(addr->sin_port) && !is_allowed_task())
        addr->sin_port = htons(1);
    return 0;
}

static int h_connect6_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct sockaddr_in6 *addr = (struct sockaddr_in6 *)regs->regs[1];
    if (!addr || addr->sin6_family != AF_INET6)
        return 0;
    if (!addr_is_loopback6(&addr->sin6_addr))
        return 0;
    if (is_hidden_port(addr->sin6_port) && !is_allowed_task())
        addr->sin6_port = htons(1);
    return 0;
}

static struct kprobe kp_connect4 = {
    .pre_handler = h_connect4_pre,
    .symbol_name = "tcp_v4_connect",
};

static struct kprobe kp_connect6 = {
    .pre_handler = h_connect6_pre,
    .symbol_name = "tcp_v6_connect",
};

int setup_connect(void)
{
    if (register_kprobe(&kp_connect4) == 0)
        kp_v4c_ok = true;
    if (register_kprobe(&kp_connect6) == 0)
        kp_v6c_ok = true;
    return 0;
}

void cleanup_connect(void)
{
    if (kp_v4c_ok) { unregister_kprobe(&kp_connect4); kp_v4c_ok = false; }
    if (kp_v6c_ok) { unregister_kprobe(&kp_connect6); kp_v6c_ok = false; }
}
