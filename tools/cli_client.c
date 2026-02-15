#include "tools/cli_client.h"
#include "platform/sdk/vo_sdk.h"
#include "vss_registry.h"
#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(vo_cli, LOG_LEVEL_INF);

#define CLI_STACK_SIZE 2048
#define CLI_PRIORITY 5

K_THREAD_STACK_DEFINE(cli_stack, CLI_STACK_SIZE);
static struct k_thread cli_thread_data;

#define STR_POOL_SIZE 8
#define STR_BUF_LEN 128
static char g_str_pool[STR_POOL_SIZE][STR_BUF_LEN];
static int g_str_idx = 0;

static const char *alloc_cli_string(const char *src)
{
    char *buf = g_str_pool[g_str_idx];
    g_str_idx = (g_str_idx + 1) % STR_POOL_SIZE;
    strncpy(buf, src, STR_BUF_LEN - 1);
    buf[STR_BUF_LEN - 1] = '\0';
    return buf;
}

static void print_help(void)
{
    printk("Commands:\n");
    printk("  set <path> <value>\n");
    printk("  get <path>\n");
    printk("Examples:\n");
    printk("  set Vehicle.Cabin.Precondition.Request START\n");
    printk("  set Vehicle.Body.Door.FrontLeft.LockTarget LOCKED\n");
    printk("  set Vehicle.Chassis.Longitudinal.AccelTarget 0.8\n");
    printk("  set Vehicle.Chassis.Lateral.SteeringAngleTarget 2.5\n");
}

static int parse_bool(const char *s, bool *out)
{
    if (!s || !out) {
        return -1;
    }
    if (strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0 || strcasecmp(s, "on") == 0) {
        *out = true;
        return 0;
    }
    if (strcasecmp(s, "false") == 0 || strcmp(s, "0") == 0 || strcasecmp(s, "off") == 0) {
        *out = false;
        return 0;
    }
    return -1;
}

static int handle_set(const char *path, const char *value_str)
{
    int handle = vss_find_handle_by_path(path);
    vss_value_t value;

    if (handle < 0) {
        printk("Unknown path: %s\n", path);
        return -1;
    }

    memset(&value, 0, sizeof(value));
    value.type = vss_registry[handle].type;

    switch (value.type) {
    case VSS_TYPE_BOOL: {
        bool b;
        if (parse_bool(value_str, &b) != 0) {
            printk("Invalid bool: %s\n", value_str);
            return -1;
        }
        value.scalar.b = b;
        break;
    }
    case VSS_TYPE_INT32:
        value.scalar.i32 = (int32_t)strtol(value_str, NULL, 0);
        break;
    case VSS_TYPE_UINT32:
        value.scalar.u32 = (uint32_t)strtoul(value_str, NULL, 0);
        break;
    case VSS_TYPE_FLOAT:
        value.scalar.f = strtof(value_str, NULL);
        break;
    case VSS_TYPE_DOUBLE:
        value.scalar.d = strtod(value_str, NULL);
        break;
    case VSS_TYPE_ENUM:
    case VSS_TYPE_STRUCT:
    case VSS_TYPE_STRING:
        value.str = alloc_cli_string(value_str);
        break;
    default:
        printk("Unsupported type for set\n");
        return -1;
    }

    if (vo_set(handle, &value) != 0) {
        printk("Set failed for %s\n", path);
        return -1;
    }

    printk("Set ok: %s\n", path);
    return 0;
}

static void print_value(const vss_value_t *value)
{
    switch (value->type) {
    case VSS_TYPE_BOOL:
        printk("%s", value->scalar.b ? "true" : "false");
        break;
    case VSS_TYPE_INT32:
        printk("%d", value->scalar.i32);
        break;
    case VSS_TYPE_UINT32:
        printk("%u", value->scalar.u32);
        break;
    case VSS_TYPE_FLOAT:
        printk("%f", (double)value->scalar.f);
        break;
    case VSS_TYPE_DOUBLE:
        printk("%f", value->scalar.d);
        break;
    case VSS_TYPE_ENUM:
    case VSS_TYPE_STRUCT:
    case VSS_TYPE_STRING:
        printk("%s", value->str ? value->str : "<null>");
        break;
    default:
        printk("<unknown>");
        break;
    }
}

static int handle_get(const char *path)
{
    int handle = vss_find_handle_by_path(path);
    vss_value_t value;

    if (handle < 0) {
        printk("Unknown path: %s\n", path);
        return -1;
    }

    if (vo_get(handle, &value) != 0) {
        printk("No value for %s\n", path);
        return -1;
    }

    printk("%s = ", path);
    print_value(&value);
    printk("\n");
    return 0;
}

static void cli_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    console_getline_init();
    print_help();

    while (1) {
        printk("> ");
        char *line = console_getline();
        if (!line) {
            continue;
        }

        while (isspace((unsigned char)*line)) {
            line++;
        }

        if (*line == '\0') {
            continue;
        }

        if (strncmp(line, "help", 4) == 0) {
            print_help();
            continue;
        }

        if (strncmp(line, "set ", 4) == 0) {
            char *p = line + 4;
            while (isspace((unsigned char)*p)) {
                p++;
            }
            char *path = p;
            char *space = strchr(p, ' ');
            if (!space) {
                printk("Usage: set <path> <value>\n");
                continue;
            }
            *space = '\0';
            char *value_str = space + 1;
            while (isspace((unsigned char)*value_str)) {
                value_str++;
            }
            if (*value_str == '\0') {
                printk("Usage: set <path> <value>\n");
                continue;
            }
            handle_set(path, value_str);
            continue;
        }

        if (strncmp(line, "get ", 4) == 0) {
            char *p = line + 4;
            while (isspace((unsigned char)*p)) {
                p++;
            }
            if (*p == '\0') {
                printk("Usage: get <path>\n");
                continue;
            }
            handle_get(p);
            continue;
        }

        printk("Unknown command. Type 'help'.\n");
    }
}

void cli_client_init(void)
{
    k_thread_create(&cli_thread_data, cli_stack, CLI_STACK_SIZE,
                    cli_thread, NULL, NULL, NULL,
                    CLI_PRIORITY, 0, K_NO_WAIT);
    LOG_INF("CLI client ready (interactive)");
}
