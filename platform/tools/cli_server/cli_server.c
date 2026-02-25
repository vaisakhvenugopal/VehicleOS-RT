#include "platform/tools/cli_server/cli_server.h"
#include "platform/sdk/vo_sdk.h"
#include "vss_registry.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

LOG_MODULE_REGISTER(vo_cli_server, LOG_LEVEL_INF);

#define CLI_SERVER_STACK_SIZE 4096
#define CLI_SERVER_PRIORITY 5
#define CLI_SERVER_PORT 5555
#define CLI_RX_LINE_MAX 256
#define CLI_TX_MAX 4096
#define CLI_UPDATE_Q_LEN 64

K_THREAD_STACK_DEFINE(cli_server_stack, CLI_SERVER_STACK_SIZE);
static struct k_thread cli_server_thread_data;

K_MSGQ_DEFINE(cli_update_q, sizeof(vss_handle_t), CLI_UPDATE_Q_LEN, 4);

static char g_watch_prefix[128];
static bool g_watch_active = false;
static uint32_t g_corr_seq = 0;

#define STR_POOL_SIZE 16
#define STR_BUF_LEN 256
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

static const char *type_str(vss_type_t t)
{
    switch (t) {
    case VSS_TYPE_BOOL: return "bool";
    case VSS_TYPE_INT32: return "int32";
    case VSS_TYPE_UINT32: return "uint32";
    case VSS_TYPE_FLOAT: return "float";
    case VSS_TYPE_DOUBLE: return "double";
    case VSS_TYPE_STRING: return "string";
    case VSS_TYPE_STRUCT: return "struct";
    case VSS_TYPE_ARRAY: return "array";
    case VSS_TYPE_ENUM: return "enum";
    default: return "unknown";
    }
}

static const char *class_str(vss_class_t c)
{
    switch (c) {
    case VSS_CLASS_MEASURED: return "measured";
    case VSS_CLASS_TARGET: return "target";
    case VSS_CLASS_STATE: return "state";
    case VSS_CLASS_CONFIG: return "config";
    case VSS_CLASS_PROCEDURE_REQUEST: return "procedure.request";
    case VSS_CLASS_PROCEDURE_STATE: return "procedure.state";
    case VSS_CLASS_PROCEDURE_RESPONSE: return "procedure.response";
    case VSS_CLASS_ACK: return "ack";
    default: return "unknown";
    }
}

static const char *slice_str(vss_slice_t s)
{
    switch (s) {
    case VSS_SLICE_SAFETY: return "safety";
    case VSS_SLICE_CONTROL: return "control";
    case VSS_SLICE_DATA: return "data";
    case VSS_SLICE_DIAG: return "diag";
    default: return "unknown";
    }
}

static const char *domain_str(vss_domain_t d)
{
    switch (d) {
    case VSS_DOMAIN_CABIN: return "cabin";
    case VSS_DOMAIN_BODY: return "body";
    case VSS_DOMAIN_POWERTRAIN: return "powertrain";
    case VSS_DOMAIN_CHASSIS: return "chassis";
    default: return "unknown";
    }
}

static void json_escape(const char *src, char *dst, size_t dst_len)
{
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 2 < dst_len; ++i) {
        unsigned char c = (unsigned char)src[i];
        if (c == '\\' || c == '"') {
            if (j + 2 >= dst_len) {
                break;
            }
            dst[j++] = '\\';
            dst[j++] = (char)c;
        } else if (c == '\n') {
            if (j + 2 >= dst_len) {
                break;
            }
            dst[j++] = '\\';
            dst[j++] = 'n';
        } else if (c == '\r') {
            if (j + 2 >= dst_len) {
                break;
            }
            dst[j++] = '\\';
            dst[j++] = 'r';
        } else if (c == '\t') {
            if (j + 2 >= dst_len) {
                break;
            }
            dst[j++] = '\\';
            dst[j++] = 't';
        } else if (isprint(c)) {
            dst[j++] = (char)c;
        }
    }
    dst[j] = '\0';
}

static int cli_send_line(int fd, const char *fmt, ...)
{
    char buf[CLI_TX_MAX];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) {
        return -1;
    }
    if ((size_t)n >= sizeof(buf)) {
        n = sizeof(buf) - 1;
        buf[n] = '\0';
    }
    buf[n++] = '\n';
    return zsock_send(fd, buf, n, 0);
}

static void normalize_path(char *path)
{
    char *start = path;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    if (start != path) {
        memmove(path, start, strlen(start) + 1);
    }
    if (strncmp(path, "vss.", 4) == 0) {
        memmove(path, path + 4, strlen(path + 4) + 1);
    }
    size_t len = strlen(path);
    while (len > 0 && (isspace((unsigned char)path[len - 1]) || path[len - 1] == '.')) {
        path[len - 1] = '\0';
        len--;
    }
}

static bool starts_with(const char *s, const char *prefix)
{
    size_t len = strlen(prefix);
    return strncmp(s, prefix, len) == 0;
}

static bool match_pattern_ci(const char *pattern, const char *text)
{
    while (*pattern) {
        if (*pattern == '*') {
            pattern++;
            if (!*pattern) {
                return true;
            }
            while (*text) {
                if (match_pattern_ci(pattern, text)) {
                    return true;
                }
                text++;
            }
            return false;
        }
        if (*pattern == '?') {
            if (!*text) {
                return false;
            }
            pattern++;
            text++;
            continue;
        }
        if (tolower((unsigned char)*pattern) != tolower((unsigned char)*text)) {
            return false;
        }
        pattern++;
        text++;
    }
    return *text == '\0';
}

static bool find_match(const char *pattern, const char *path)
{
    if (strchr(pattern, '*') || strchr(pattern, '?')) {
        return match_pattern_ci(pattern, path);
    }
    const char *p = path;
    size_t plen = strlen(pattern);
    for (; *p; ++p) {
        if (strncasecmp(p, pattern, plen) == 0) {
            return true;
        }
    }
    return false;
}

static void cli_update_cb(vss_handle_t handle)
{
    (void)k_msgq_put(&cli_update_q, &handle, K_NO_WAIT);
}

static int read_line(int fd, char *out, size_t out_len)
{
    static char buf[512];
    static size_t buf_len = 0;

    while (1) {
        for (size_t i = 0; i < buf_len; ++i) {
            if (buf[i] == '\n') {
                size_t copy_len = (i < out_len - 1) ? i : out_len - 1;
                memcpy(out, buf, copy_len);
                out[copy_len] = '\0';
                memmove(buf, buf + i + 1, buf_len - i - 1);
                buf_len -= (i + 1);
                return (int)copy_len;
            }
        }

        int rc = zsock_recv(fd, buf + buf_len, sizeof(buf) - buf_len, 0);
        if (rc == 0) {
            return -1;
        }
        if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }
            return -1;
        }
        buf_len += rc;
        if (buf_len >= sizeof(buf)) {
            buf_len = 0;
            return -1;
        }
    }
}

static void send_update(int fd, vss_handle_t handle)
{
    vss_value_t value;
    uint64_t ts_ms = 0;
    uint32_t seq = 0;
    if (vo_get_meta(handle, &value, &ts_ms, &seq) != 0) {
        return;
    }

    char path_esc[256];
    json_escape(vss_registry[handle].path, path_esc, sizeof(path_esc));

    char value_buf[256];
    switch (value.type) {
    case VSS_TYPE_BOOL:
        snprintf(value_buf, sizeof(value_buf), "%s", value.scalar.b ? "true" : "false");
        break;
    case VSS_TYPE_INT32:
        snprintf(value_buf, sizeof(value_buf), "%d", value.scalar.i32);
        break;
    case VSS_TYPE_UINT32:
        snprintf(value_buf, sizeof(value_buf), "%u", value.scalar.u32);
        break;
    case VSS_TYPE_FLOAT:
        snprintf(value_buf, sizeof(value_buf), "%f", (double)value.scalar.f);
        break;
    case VSS_TYPE_DOUBLE:
        snprintf(value_buf, sizeof(value_buf), "%f", value.scalar.d);
        break;
    default: {
        char esc[192];
        json_escape(value.str ? value.str : "", esc, sizeof(esc));
        snprintf(value_buf, sizeof(value_buf), "\"%s\"", esc);
        break;
    }
    }

    cli_send_line(fd,
                  "{\"ts\":%llu,\"event\":\"update\",\"path\":\"%s\",\"value\":%s,"
                  "\"seq\":%u,\"slice\":\"%s\",\"domain\":\"%s\",\"notify_only\":false,"
                  "\"value_type\":\"%s\"}",
                  (unsigned long long)ts_ms,
                  path_esc,
                  value_buf,
                  seq,
                  slice_str(vss_registry[handle].slice),
                  domain_str(vss_registry[handle].domain),
                  type_str(vss_registry[handle].type));
}

static void handle_help(int fd)
{
    cli_send_line(fd,
                  "{\"event\":\"response\",\"cmd\":\"help\",\"status\":\"ok\","
                  "\"data\":{\"commands\":["
                  "\"help\",\"domains\",\"ls <prefix>\",\"find <pattern>\","
                  "\"describe <path>\",\"get <path>\",\"set <path> <value>\","
                  "\"watch <prefix>\"],"
                  "\"examples\":["
                  "\"ls vss.Vehicle.Cabin\","
                  "\"find temperature\","
                  "\"describe vss.Vehicle.Cabin.HVAC.TargetTemperature\","
                  "\"get vss.Vehicle.Cabin.HVAC.CabinTemperature\","
                  "\"set vss.Vehicle.Cabin.HVAC.TargetTemperature 22.0\","
                  "\"watch vss.Vehicle.Cabin\"]}}");
}

static void handle_domains(int fd)
{
    bool seen[5] = {0};
    char buf[CLI_TX_MAX];
    size_t off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "{\"event\":\"response\",\"cmd\":\"domains\",\"status\":\"ok\",\"data\":{\"domains\":[");

    for (int i = 0; i < VSS_SIGNAL_COUNT; ++i) {
        vss_domain_t d = vss_registry[i].domain;
        if (d < 0 || d >= 5 || seen[d]) {
            continue;
        }
        seen[d] = true;
        off += snprintf(buf + off, sizeof(buf) - off, "\"%s\",", domain_str(d));
    }
    if (off > 0 && buf[off - 1] == ',') {
        off--;
    }
    snprintf(buf + off, sizeof(buf) - off, "]}}");
    cli_send_line(fd, "%s", buf);
}

static void handle_ls(int fd, char *prefix)
{
    normalize_path(prefix);
    char buf[CLI_TX_MAX];
    size_t off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "{\"event\":\"response\",\"cmd\":\"ls\",\"status\":\"ok\",\"data\":{\"items\":[");

    char seen[64][128];
    size_t seen_count = 0;

    for (int i = 0; i < VSS_SIGNAL_COUNT; ++i) {
        const char *path = vss_registry[i].path;
        if (prefix[0] != '\0') {
            if (!starts_with(path, prefix)) {
                continue;
            }
            if (path[strlen(prefix)] != '.') {
                continue;
            }
        }
        const char *rest = (prefix[0] == '\0') ? path : path + strlen(prefix) + 1;
        if (*rest == '\0') {
            continue;
        }
        const char *dot = strchr(rest, '.');
        size_t seg_len = dot ? (size_t)(dot - rest) : strlen(rest);
        if (seg_len == 0 || seg_len >= 100) {
            continue;
        }

        char child_path[128];
        if (prefix[0] == '\0') {
            snprintf(child_path, sizeof(child_path), "%.*s", (int)seg_len, rest);
        } else {
            snprintf(child_path, sizeof(child_path), "%s.%.*s", prefix, (int)seg_len, rest);
        }

        bool dup = false;
        for (size_t s = 0; s < seen_count; ++s) {
            if (strcmp(seen[s], child_path) == 0) {
                dup = true;
                break;
            }
        }
        if (dup || seen_count >= 64) {
            continue;
        }
        strncpy(seen[seen_count++], child_path, sizeof(seen[0]) - 1);

        int handle = vss_find_handle_by_path(child_path);
        if (handle >= 0) {
            char esc[256];
            json_escape(child_path, esc, sizeof(esc));
            off += snprintf(buf + off, sizeof(buf) - off,
                            "{\"path\":\"%s\",\"type\":\"%s\",\"class\":\"%s\"},",
                            esc,
                            type_str(vss_registry[handle].type),
                            class_str(vss_registry[handle].class));
        } else {
            char esc[256];
            json_escape(child_path, esc, sizeof(esc));
            off += snprintf(buf + off, sizeof(buf) - off,
                            "{\"path\":\"%s\",\"type\":\"node\",\"class\":\"node\"},",
                            esc);
        }
    }
    if (off > 0 && buf[off - 1] == ',') {
        off--;
    }
    snprintf(buf + off, sizeof(buf) - off, "]}}");
    cli_send_line(fd, "%s", buf);
}

static void handle_find(int fd, char *pattern)
{
    normalize_path(pattern);
    char buf[CLI_TX_MAX];
    size_t off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "{\"event\":\"response\",\"cmd\":\"find\",\"status\":\"ok\",\"data\":{\"items\":[");

    for (int i = 0; i < VSS_SIGNAL_COUNT; ++i) {
        const char *path = vss_registry[i].path;
        if (!find_match(pattern, path)) {
            continue;
        }
        char esc[256];
        json_escape(path, esc, sizeof(esc));
        off += snprintf(buf + off, sizeof(buf) - off,
                        "{\"path\":\"%s\",\"type\":\"%s\",\"class\":\"%s\",\"slice\":\"%s\","
                        "\"domain\":\"%s\",\"owner\":\"%s\"},",
                        esc,
                        type_str(vss_registry[i].type),
                        class_str(vss_registry[i].class),
                        slice_str(vss_registry[i].slice),
                        domain_str(vss_registry[i].domain),
                        vss_registry[i].owner ? vss_registry[i].owner : "");
    }
    if (off > 0 && buf[off - 1] == ',') {
        off--;
    }
    snprintf(buf + off, sizeof(buf) - off, "]}}");
    cli_send_line(fd, "%s", buf);
}

static void handle_describe(int fd, char *path)
{
    normalize_path(path);
    int handle = vss_find_handle_by_path(path);
    if (handle < 0) {
        cli_send_line(fd,
                      "{\"event\":\"error\",\"cmd\":\"describe\",\"status\":\"error\",\"message\":\"Unknown path\"}");
        return;
    }

    char esc[256];
    json_escape(vss_registry[handle].path, esc, sizeof(esc));

    char ack_path[256] = {0};
    if (vss_registry[handle].ack_required) {
        snprintf(ack_path, sizeof(ack_path), "%s.Ack", vss_registry[handle].path);
        if (vss_find_handle_by_path(ack_path) < 0) {
            ack_path[0] = '\0';
        }
    }

    char proc_state[256] = {0};
    char proc_resp[256] = {0};
    if (vss_registry[handle].class == VSS_CLASS_PROCEDURE_REQUEST) {
        const char *suffix = ".Request";
        size_t len = strlen(vss_registry[handle].path);
        size_t slen = strlen(suffix);
        if (len > slen && strcmp(vss_registry[handle].path + (len - slen), suffix) == 0) {
            snprintf(proc_state, sizeof(proc_state), "%.*s.State", (int)(len - slen), vss_registry[handle].path);
            snprintf(proc_resp, sizeof(proc_resp), "%.*s.Response", (int)(len - slen), vss_registry[handle].path);
            if (vss_find_handle_by_path(proc_state) < 0) {
                proc_state[0] = '\0';
            }
            if (vss_find_handle_by_path(proc_resp) < 0) {
                proc_resp[0] = '\0';
            }
        }
    }

    char min_buf[32];
    char max_buf[32];
    if (vss_registry[handle].has_min) {
        snprintf(min_buf, sizeof(min_buf), "%f", vss_registry[handle].min);
    } else {
        snprintf(min_buf, sizeof(min_buf), "null");
    }
    if (vss_registry[handle].has_max) {
        snprintf(max_buf, sizeof(max_buf), "%f", vss_registry[handle].max);
    } else {
        snprintf(max_buf, sizeof(max_buf), "null");
    }

    char ack_esc[256];
    if (ack_path[0]) {
        json_escape(ack_path, ack_esc, sizeof(ack_esc));
    }

    char proc_state_esc[256] = {0};
    char proc_resp_esc[256] = {0};
    if (proc_state[0]) {
        json_escape(proc_state, proc_state_esc, sizeof(proc_state_esc));
    }
    if (proc_resp[0]) {
        json_escape(proc_resp, proc_resp_esc, sizeof(proc_resp_esc));
    }

    cli_send_line(fd,
                  "{\"event\":\"response\",\"cmd\":\"describe\",\"status\":\"ok\",\"data\":{"
                  "\"path\":\"%s\",\"handle\":%d,\"vss_type\":\"%s\",\"class\":\"%s\","
                  "\"slice\":\"%s\",\"domain\":\"%s\",\"owner\":\"%s\","
                  "\"ack_required\":%s,\"ack_paths\":[%s%s%s],"
                  "\"constraints\":{\"unit\":null,\"min\":%s,\"max\":%s,\"eps\":null,\"min_period\":null},"
                  "\"procedure\":{\"state\":\"%s\",\"response\":\"%s\"}"
                  "}}",
                  esc,
                  handle,
                  type_str(vss_registry[handle].type),
                  class_str(vss_registry[handle].class),
                  slice_str(vss_registry[handle].slice),
                  domain_str(vss_registry[handle].domain),
                  vss_registry[handle].owner ? vss_registry[handle].owner : "",
                  vss_registry[handle].ack_required ? "true" : "false",
                  ack_path[0] ? "\"" : "",
                  ack_path[0] ? ack_esc : "",
                  ack_path[0] ? "\"" : "",
                  min_buf,
                  max_buf,
                  proc_state_esc,
                  proc_resp_esc);
}

static void handle_get(int fd, char *path)
{
    normalize_path(path);
    int handle = vss_find_handle_by_path(path);
    if (handle < 0) {
        cli_send_line(fd,
                      "{\"event\":\"error\",\"cmd\":\"get\",\"status\":\"error\",\"message\":\"Unknown path\"}");
        return;
    }

    vss_value_t value;
    uint64_t ts_ms = 0;
    uint32_t seq = 0;
    int rc = vo_get_meta(handle, &value, &ts_ms, &seq);
    if (rc != 0) {
        cli_send_line(fd,
                      "{\"event\":\"response\",\"cmd\":\"get\",\"status\":\"ok\",\"data\":{"
                      "\"path\":\"%s\",\"valid\":false}}",
                      vss_registry[handle].path);
        return;
    }

    char value_buf[256];
    switch (value.type) {
    case VSS_TYPE_BOOL:
        snprintf(value_buf, sizeof(value_buf), "%s", value.scalar.b ? "true" : "false");
        break;
    case VSS_TYPE_INT32:
        snprintf(value_buf, sizeof(value_buf), "%d", value.scalar.i32);
        break;
    case VSS_TYPE_UINT32:
        snprintf(value_buf, sizeof(value_buf), "%u", value.scalar.u32);
        break;
    case VSS_TYPE_FLOAT:
        snprintf(value_buf, sizeof(value_buf), "%f", (double)value.scalar.f);
        break;
    case VSS_TYPE_DOUBLE:
        snprintf(value_buf, sizeof(value_buf), "%f", value.scalar.d);
        break;
    default: {
        char esc[192];
        json_escape(value.str ? value.str : "", esc, sizeof(esc));
        snprintf(value_buf, sizeof(value_buf), "\"%s\"", esc);
        break;
    }
    }

    cli_send_line(fd,
                  "{\"event\":\"response\",\"cmd\":\"get\",\"status\":\"ok\",\"data\":{"
                  "\"path\":\"%s\",\"value\":%s,\"value_type\":\"%s\",\"ts\":%llu,\"seq\":%u,\"valid\":true}}",
                  vss_registry[handle].path,
                  value_buf,
                  type_str(vss_registry[handle].type),
                  (unsigned long long)ts_ms,
                  seq);
}

static int parse_value(vss_handle_t handle, const char *value_str, vss_value_t *out)
{
    if (!value_str || !out) {
        return -EINVAL;
    }
    memset(out, 0, sizeof(*out));
    out->type = vss_registry[handle].type;

    switch (out->type) {
    case VSS_TYPE_BOOL: {
        if (strcasecmp(value_str, "true") == 0 || strcmp(value_str, "1") == 0 || strcasecmp(value_str, "on") == 0) {
            out->scalar.b = true;
            return 0;
        }
        if (strcasecmp(value_str, "false") == 0 || strcmp(value_str, "0") == 0 || strcasecmp(value_str, "off") == 0) {
            out->scalar.b = false;
            return 0;
        }
        return -EINVAL;
    }
    case VSS_TYPE_INT32:
        out->scalar.i32 = (int32_t)strtol(value_str, NULL, 0);
        return 0;
    case VSS_TYPE_UINT32:
        out->scalar.u32 = (uint32_t)strtoul(value_str, NULL, 0);
        return 0;
    case VSS_TYPE_FLOAT:
        out->scalar.f = strtof(value_str, NULL);
        return 0;
    case VSS_TYPE_DOUBLE:
        out->scalar.d = strtod(value_str, NULL);
        return 0;
    case VSS_TYPE_ENUM:
    case VSS_TYPE_STRUCT:
    case VSS_TYPE_STRING:
        out->str = alloc_cli_string(value_str);
        return 0;
    default:
        return -EINVAL;
    }
}

static void handle_set(int fd, char *path, char *value_str)
{
    normalize_path(path);
    int handle = vss_find_handle_by_path(path);
    if (handle < 0) {
        cli_send_line(fd,
                      "{\"event\":\"error\",\"cmd\":\"set\",\"status\":\"error\",\"message\":\"Unknown path\"}");
        return;
    }

    vss_value_t value;
    if (parse_value(handle, value_str, &value) != 0) {
        cli_send_line(fd,
                      "{\"event\":\"error\",\"cmd\":\"set\",\"status\":\"error\","
                      "\"message\":\"DENIED: type mismatch expected=%s\","
                      "\"reason_code\":\"type_mismatch\",\"expected_type\":\"%s\"}",
                      type_str(vss_registry[handle].type),
                      type_str(vss_registry[handle].type));
        return;
    }

    int rc = vo_set(handle, &value);
    if (rc != 0) {
        const char *reason = "unknown";
        const char *msg = "DENIED";
        if (rc == -EPERM) {
            reason = "publish_only";
            msg = "DENIED: class is publish-only";
        } else if (rc == -ERANGE) {
            reason = "out_of_range";
            msg = "DENIED: value out of range";
        }
        cli_send_line(fd,
                      "{\"event\":\"error\",\"cmd\":\"set\",\"status\":\"error\",\"message\":\"%s\","
                      "\"reason_code\":\"%s\",\"expected_type\":\"%s\",\"class\":\"%s\"}",
                      msg,
                      reason,
                      type_str(vss_registry[handle].type),
                      class_str(vss_registry[handle].class));
        return;
    }

    bool ack_required = vss_registry[handle].ack_required;
    char ack_path[256] = {0};
    if (ack_required) {
        snprintf(ack_path, sizeof(ack_path), "%s.Ack", vss_registry[handle].path);
        if (vss_find_handle_by_path(ack_path) < 0) {
            ack_path[0] = '\0';
        }
    }
    uint32_t corr_id = ++g_corr_seq;

    char ack_json[256];
    if (ack_path[0]) {
        char esc[200];
        json_escape(ack_path, esc, sizeof(esc));
        snprintf(ack_json, sizeof(ack_json), "\"%s\"", esc);
    } else {
        snprintf(ack_json, sizeof(ack_json), "null");
    }

    cli_send_line(fd,
                  "{\"event\":\"response\",\"cmd\":\"set\",\"status\":\"ok\",\"data\":{"
                  "\"path\":\"%s\",\"correlation_id\":%u,\"ack_required\":%s,\"ack_path\":%s}}",
                  vss_registry[handle].path,
                  corr_id,
                  ack_required ? "true" : "false",
                  ack_json);
}

static void handle_watch(int fd, char *prefix)
{
    normalize_path(prefix);
    if (g_watch_active) {
        vo_unsubscribe_prefix(g_watch_prefix, cli_update_cb);
        g_watch_active = false;
    }
    strncpy(g_watch_prefix, prefix, sizeof(g_watch_prefix) - 1);
    g_watch_prefix[sizeof(g_watch_prefix) - 1] = '\0';

    if (vo_subscribe_prefix(g_watch_prefix, cli_update_cb) != 0) {
        cli_send_line(fd,
                      "{\"event\":\"error\",\"cmd\":\"watch\",\"status\":\"error\",\"message\":\"Subscribe failed\"}");
        return;
    }
    g_watch_active = true;
    cli_send_line(fd,
                  "{\"event\":\"response\",\"cmd\":\"watch\",\"status\":\"ok\",\"data\":{\"prefix\":\"%s\"}}",
                  g_watch_prefix);
}

static void cli_handle_line(int fd, char *line)
{
    while (isspace((unsigned char)*line)) {
        line++;
    }
    if (*line == '\0') {
        return;
    }

    char *saveptr = NULL;
    char *cmd = strtok_r(line, " ", &saveptr);
    if (!cmd) {
        return;
    }

    if (strcmp(cmd, "help") == 0) {
        handle_help(fd);
        return;
    }
    if (strcmp(cmd, "domains") == 0) {
        handle_domains(fd);
        return;
    }
    if (strcmp(cmd, "ls") == 0) {
        char *prefix = strtok_r(NULL, "", &saveptr);
        if (!prefix) {
            cli_send_line(fd, "{\"event\":\"error\",\"cmd\":\"ls\",\"status\":\"error\",\"message\":\"Missing prefix\"}");
            return;
        }
        handle_ls(fd, prefix);
        return;
    }
    if (strcmp(cmd, "find") == 0) {
        char *pattern = strtok_r(NULL, "", &saveptr);
        if (!pattern) {
            cli_send_line(fd, "{\"event\":\"error\",\"cmd\":\"find\",\"status\":\"error\",\"message\":\"Missing pattern\"}");
            return;
        }
        handle_find(fd, pattern);
        return;
    }
    if (strcmp(cmd, "describe") == 0) {
        char *path = strtok_r(NULL, "", &saveptr);
        if (!path) {
            cli_send_line(fd, "{\"event\":\"error\",\"cmd\":\"describe\",\"status\":\"error\",\"message\":\"Missing path\"}");
            return;
        }
        handle_describe(fd, path);
        return;
    }
    if (strcmp(cmd, "get") == 0) {
        char *path = strtok_r(NULL, "", &saveptr);
        if (!path) {
            cli_send_line(fd, "{\"event\":\"error\",\"cmd\":\"get\",\"status\":\"error\",\"message\":\"Missing path\"}");
            return;
        }
        handle_get(fd, path);
        return;
    }
    if (strcmp(cmd, "set") == 0) {
        char *path = strtok_r(NULL, " ", &saveptr);
        char *value_str = strtok_r(NULL, "", &saveptr);
        if (!path || !value_str) {
            cli_send_line(fd, "{\"event\":\"error\",\"cmd\":\"set\",\"status\":\"error\",\"message\":\"Usage: set <path> <value>\"}");
            return;
        }
        while (isspace((unsigned char)*value_str)) {
            value_str++;
        }
        handle_set(fd, path, value_str);
        return;
    }
    if (strcmp(cmd, "watch") == 0) {
        char *prefix = strtok_r(NULL, " ", &saveptr);
        if (!prefix) {
            cli_send_line(fd, "{\"event\":\"error\",\"cmd\":\"watch\",\"status\":\"error\",\"message\":\"Missing prefix\"}");
            return;
        }
        handle_watch(fd, prefix);
        return;
    }

    cli_send_line(fd, "{\"event\":\"error\",\"status\":\"error\",\"message\":\"Unknown command\"}");
}

static void cli_server_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    int server_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        LOG_ERR("Socket create failed: %d", errno);
        return;
    }

    int opt = 1;
    zsock_setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CLI_SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (zsock_bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("Bind failed: %d", errno);
        zsock_close(server_fd);
        return;
    }

    if (zsock_listen(server_fd, 1) < 0) {
        LOG_ERR("Listen failed: %d", errno);
        zsock_close(server_fd);
        return;
    }

    LOG_INF("CLI server listening on port %d", CLI_SERVER_PORT);

    while (1) {
        struct sockaddr_in client_addr = {0};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = zsock_accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            LOG_ERR("Accept failed: %d", errno);
            continue;
        }

        struct timeval tv = {.tv_sec = 0, .tv_usec = 100000};
        zsock_setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        LOG_INF("CLI client connected");

        char line[CLI_RX_LINE_MAX];
        while (1) {
            vss_handle_t handle;
            while (k_msgq_get(&cli_update_q, &handle, K_NO_WAIT) == 0) {
                send_update(client_fd, handle);
            }

            int rc = read_line(client_fd, line, sizeof(line));
            if (rc < 0) {
                break;
            }
            if (rc == 0) {
                continue;
            }
            cli_handle_line(client_fd, line);
        }

        if (g_watch_active) {
            vo_unsubscribe_prefix(g_watch_prefix, cli_update_cb);
            g_watch_active = false;
        }
        zsock_close(client_fd);
        LOG_INF("CLI client disconnected");
    }
}

void cli_server_init(void)
{
    k_thread_create(&cli_server_thread_data, cli_server_stack, CLI_SERVER_STACK_SIZE,
                    cli_server_thread, NULL, NULL, NULL,
                    CLI_SERVER_PRIORITY, 0, K_NO_WAIT);
    LOG_INF("CLI server ready (TCP)");
}
