#include "port_hider.h"

/* inode_permission → access/open; vfs_getattr → stat
 * ARM64: 5.12+ x1=inode, <5.12 x0=inode
 * MinPath: 0=off, 1-4=threshold before activating */

static int MinPath;
module_param(MinPath, int, 0444);

static bool rp_ip_ok;
static bool rp_ga_ok;
static bool vfs_retry_init;
static struct delayed_work vfs_retry;

#define IPERM_DATA_LEN sizeof(struct inode *)

static const char * const blocked_paths[] = {
    "/dev/scene",
    "/dev/memcg/scene_idle",
    "/dev/memcg/scene_active",
    "/dev/cpuset/scene-daemon",
};

static bool match_inode(struct inode *inode)
{
    int i;
    if (!inode) return false;
    for (i = 0; i < nr_blocked; i++) {
        if (blocked[i].valid &&
            blocked[i].dev == inode->i_sb->s_dev &&
            blocked[i].ino == inode->i_ino)
            return true;
    }
    return false;
}

static bool already_blocked(dev_t dev, unsigned long ino)
{
    int i;
    for (i = 0; i < nr_blocked; i++) {
        if (blocked[i].valid && blocked[i].dev == dev && blocked[i].ino == ino)
            return true;
    }
    return false;
}

static void resolve_blocked(void)
{
    struct path p;
    dev_t dev;
    unsigned long ino;
    int i;

    for (i = 0; i < ARRAY_SIZE(blocked_paths) && nr_blocked < MAX_BLOCKED_PATHS; i++) {
        if (kern_path(blocked_paths[i], 0, &p) != 0)
            continue;
        dev = d_inode(p.dentry)->i_sb->s_dev;
        ino = d_inode(p.dentry)->i_ino;
        if (!already_blocked(dev, ino)) {
            blocked[nr_blocked].dev = dev;
            blocked[nr_blocked].ino = ino;
            blocked[nr_blocked].valid = true;
            nr_blocked++;
            pr_info("vfs: blocked %s\n", blocked_paths[i]);
        }
        path_put(&p);
    }
}

static int register_hooks(void);

static void vfs_retry_fn(struct work_struct *work)
{
    resolve_blocked();
    if (nr_blocked >= MinPath) {
        register_hooks();
        pr_info("vfs: %d paths >= MinPath=%d, active\n", nr_blocked, MinPath);
    } else {
        pr_info("vfs: %d/%d paths, retry 5s\n", nr_blocked, MinPath);
        schedule_delayed_work(&vfs_retry, msecs_to_jiffies(5000));
    }
}

/* inode_permission hook (access/open/exec) */

static int h_iperm_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,12,0)
    *(struct inode **)ri->data = (struct inode *)regs->regs[1];
#else
    *(struct inode **)ri->data = (struct inode *)regs->regs[0];
#endif
    return 0;
}

static int h_iperm_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    if (is_allowed_task())
        return 0;
    if (match_inode(*(struct inode **)ri->data))
        regs->regs[0] = -ENOENT;
    return 0;
}

static struct kretprobe rp_ip = {
    .entry_handler = h_iperm_entry,
    .handler       = h_iperm_ret,
    .data_size     = IPERM_DATA_LEN,
    .maxactive     = 256,
    .kp.symbol_name = "inode_permission",
};

/* vfs_getattr hook (stat family) */

#define GETATTR_DATA_LEN sizeof(struct path *)

static int h_ga_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    *(struct path **)ri->data = (struct path *)regs->regs[0];
    return 0;
}

static int h_ga_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct path *p = *(struct path **)ri->data;

    if ((int)regs->regs[0] != 0 || !p || !p->dentry)
        return 0;
    if (is_allowed_task())
        return 0;
    if (match_inode(d_inode(p->dentry)))
        regs->regs[0] = -ENOENT;
    return 0;
}

static struct kretprobe rp_ga = {
    .entry_handler = h_ga_entry,
    .handler       = h_ga_ret,
    .data_size     = GETATTR_DATA_LEN,
    .maxactive     = 256,
    .kp.symbol_name = "vfs_getattr",
};

static int register_hooks(void)
{
    int ok = 0;
    if (register_kretprobe(&rp_ip) == 0) { rp_ip_ok = true; ok++; }
    if (register_kretprobe(&rp_ga) == 0) { rp_ga_ok = true; ok++; }
    return ok;
}

/* setup / cleanup */

int setup_vfs(void)
{
    if (MinPath == 0) {
        pr_info("vfs: MinPath=0, disabled\n");
        return 0;
    }
    if (MinPath < 0 || MinPath > ARRAY_SIZE(blocked_paths)) {
        pr_warn("vfs: MinPath=%d out of range, disabled\n", MinPath);
        return 0;
    }

    resolve_blocked();

    if (nr_blocked >= MinPath) {
        register_hooks();
        pr_info("vfs: %d paths >= MinPath=%d, active\n", nr_blocked, MinPath);
        return 0;
    }

    pr_info("vfs: %d/%d paths, retry 5s\n", nr_blocked, MinPath);
    vfs_retry_init = true;
    INIT_DELAYED_WORK(&vfs_retry, vfs_retry_fn);
    schedule_delayed_work(&vfs_retry, msecs_to_jiffies(5000));
    return 0;
}

void cleanup_vfs(void)
{
    if (vfs_retry_init) { cancel_delayed_work_sync(&vfs_retry); vfs_retry_init = false; }
    if (rp_ga_ok) { unregister_kretprobe(&rp_ga); rp_ga_ok = false; }
    if (rp_ip_ok) { unregister_kretprobe(&rp_ip); rp_ip_ok = false; }
    nr_blocked = 0;
}
