#include "app.h"

typedef int (*cmd_fn)(const char *arg);
struct cmd { const char *name; cmd_fn fn; };

static size_t slen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }

static int cmd_ping(const char *arg) { (void)arg; return 0; }
static int cmd_echo(const char *arg) { return (int)slen(arg); }
static int cmd_version(const char *arg) { (void)arg; return (int)slen(APP_VERSION); }

static const struct cmd cmd_table[] = {
    { "ping", cmd_ping },
    { "echo", cmd_echo },
    { "version", cmd_version },
    { "status", NULL }, /* handler lands in the next commit */
};

static const char *volatile cmd_last;

size_t cmd_count(void) { return sizeof cmd_table / sizeof cmd_table[0]; }
const char *cmd_name(size_t i) { return cmd_table[i].name; }

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

int __attribute__((noinline)) cmd_dispatch(const char *name, const char *arg)
{
    for (size_t i = 0; i < cmd_count(); i++) {
        if (streq(cmd_table[i].name, name)) {
            int rc = cmd_table[i].fn(arg);
            cmd_last = cmd_table[i].name;
            return rc;
        }
    }
    return -1;
}
