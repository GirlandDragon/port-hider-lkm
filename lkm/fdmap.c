#include "port_hider.h"

static unsigned int fd_key(pid_t tgid, unsigned int fd)
{
    return hash_32((u32)tgid ^ (u32)fd, FD_HASH_BITS);
}

void fd_del(pid_t tgid, unsigned int fd)
{
    struct fd_entry *e;
    struct hlist_node *tmp;
    spin_lock(&fd_lock);
    hlist_for_each_entry_safe(e, tmp, &fd_htable[fd_key(tgid, fd)], node) {
        if (e->tgid == tgid && e->fd == fd) {
            hlist_del(&e->node);
            kfree(e);
            break;
        }
    }
    spin_unlock(&fd_lock);
}

void fd_add(pid_t tgid, unsigned int fd, u16 port)
{
    struct fd_entry *e, *old;
    struct hlist_node *tmp;

    e = kmalloc(sizeof(*e), GFP_ATOMIC);
    if (!e) return;
    e->tgid = tgid;
    e->fd   = fd;
    e->orig_port = port;

    spin_lock(&fd_lock);
    hlist_for_each_entry_safe(old, tmp, &fd_htable[fd_key(tgid, fd)], node) {
        if (old->tgid == tgid && old->fd == fd) {
            hlist_del(&old->node);
            kfree(old);
            break;
        }
    }
    hlist_add_head(&e->node, &fd_htable[fd_key(tgid, fd)]);
    spin_unlock(&fd_lock);
}

bool fd_get(pid_t tgid, unsigned int fd, u16 *port)
{
    struct fd_entry *e;
    bool found = false;
    spin_lock(&fd_lock);
    hlist_for_each_entry(e, &fd_htable[fd_key(tgid, fd)], node) {
        if (e->tgid == tgid && e->fd == fd) {
            *port = e->orig_port;
            found = true;
            break;
        }
    }
    spin_unlock(&fd_lock);
    return found;
}

void fdmap_cleanup(void)
{
    int i;
    struct fd_entry *e;
    struct hlist_node *tmp;
    for (i = 0; i < FD_HASH_SIZE; i++)
        hlist_for_each_entry_safe(e, tmp, &fd_htable[i], node) {
            hlist_del(&e->node);
            kfree(e);
        }
}
