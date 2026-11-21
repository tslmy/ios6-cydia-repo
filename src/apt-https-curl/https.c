// apt "https" acquire method backed by curl + OpenSSL 1.1.1, for iOS 6.
//
// iOS 6's Secure Transport (used by the stock CFNetwork-based method) has no
// AES-GCM, so repos on GCM-only servers fail. This speaks apt's method
// protocol (v1.2) and runs /usr/bin/curl for each download.
//
// apt 0.7's reader is fragile: every message is emitted with exactly one
// write(), each request gets exactly one reply (201 or 400), and Pipeline is
// off, so apt never has two of our messages in flight at once.
#include <CommonCrypto/CommonDigest.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

#define CURL "/usr/bin/curl"
#define CA "/etc/ssl/certs/cacert.pem"
#define UA "Telesphoreo APT-HTTP/1.0.592"

static char machine[64], firmware[32];

static void send_msg(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void send_msg(const char *fmt, ...) {
    char buf[8192];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf - 2, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > (int)sizeof buf - 3) n = sizeof buf - 3;
    buf[n++] = '\n';
    buf[n++] = '\n';
    for (int off = 0; off < n;) {  // a pipe write this small is atomic
        ssize_t w = write(1, buf + off, n - off);
        if (w < 0) { if (errno == EINTR) continue; exit(1); }
        off += w;
    }
}

static void read_firmware(void) {
    size_t len = sizeof machine;
    if (sysctlbyname("hw.machine", machine, &len, NULL, 0) != 0) machine[0] = 0;
    FILE *f = fopen("/System/Library/CoreServices/SystemVersion.plist", "r");
    if (!f) return;
    char line[256];
    int next = 0;
    while (fgets(line, sizeof line, f)) {
        if (next) {
            char *a = strstr(line, "<string>"), *b = a ? strstr(a, "</string>") : NULL;
            if (a && b) { a += 8; snprintf(firmware, sizeof firmware, "%.*s", (int)(b - a), a); }
            break;
        }
        if (strstr(line, "<key>ProductVersion</key>")) next = 1;
    }
    fclose(f);
}

static void hex(char *out, const unsigned char *d, int n) {
    for (int i = 0; i < n; i++) sprintf(out + 2 * i, "%02x", d[i]);
}

static long long hash_file(const char *path, char *md5, char *sha1, char *sha256) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    CC_MD5_CTX m; CC_SHA1_CTX s1; CC_SHA256_CTX s2;
    CC_MD5_Init(&m); CC_SHA1_Init(&s1); CC_SHA256_Init(&s2);
    unsigned char buf[65536];
    long long total = 0;
    ssize_t r;
    while ((r = read(fd, buf, sizeof buf)) > 0) {
        CC_MD5_Update(&m, buf, (CC_LONG)r);
        CC_SHA1_Update(&s1, buf, (CC_LONG)r);
        CC_SHA256_Update(&s2, buf, (CC_LONG)r);
        total += r;
    }
    close(fd);
    unsigned char d[CC_SHA256_DIGEST_LENGTH];
    CC_MD5_Final(d, &m);    hex(md5, d, CC_MD5_DIGEST_LENGTH);
    CC_SHA1_Final(d, &s1);  hex(sha1, d, CC_SHA1_DIGEST_LENGTH);
    CC_SHA256_Final(d, &s2); hex(sha256, d, CC_SHA256_DIGEST_LENGTH);
    return total;
}

// Reads a whole small file into buf (NUL-terminated).
static void slurp(const char *path, char *buf, size_t cap) {
    buf[0] = 0;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return;
    ssize_t n = read(fd, buf, cap - 1);
    close(fd);
    buf[n > 0 ? n : 0] = 0;
}

// Last "Last-Modified:" header value (case-insensitive) from a curl -D dump.
static void last_modified(const char *hdrs, char *out, size_t cap) {
    out[0] = 0;
    for (const char *p = hdrs; *p;) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) : strlen(p);
        if (len > 14 && strncasecmp(p, "Last-Modified:", 14) == 0) {
            const char *v = p + 14;
            while (*v == ' ') v++;
            size_t vl = len - (v - p);
            while (vl && (v[vl - 1] == '\r' || v[vl - 1] == ' ')) vl--;
            snprintf(out, cap, "%.*s", (int)vl, v);
        }
        if (!eol) break;
        p = eol + 1;
    }
}

static void one_line(char *s) {
    for (; *s; s++) if (*s == '\n' || *s == '\r') *s = ' ';
}

static void fetch(const char *uri, const char *dest, const char *lm) {
    char tmp[1024], hdr[] = "/tmp/apt-https-hdr.XXXXXX", out[] = "/tmp/apt-https-out.XXXXXX";
    snprintf(tmp, sizeof tmp, "%s.curl.XXXXXX", dest);
    int tfd = mkstemp(tmp), hfd = mkstemp(hdr), ofd = mkstemp(out);
    if (tfd < 0 || hfd < 0 || ofd < 0) {
        send_msg("400 URI Failure\nURI: %s\nMessage: cannot create temp files: %s", uri, strerror(errno));
        if (tfd >= 0) { close(tfd); unlink(tmp); }
        if (hfd >= 0) { close(hfd); unlink(hdr); }
        if (ofd >= 0) { close(ofd); unlink(out); }
        return;
    }
    close(tfd); close(hfd);

    char xm[96], xf[64];
    const char *argv[40];
    int a = 0;
    argv[a++] = CURL; argv[a++] = "-sS"; argv[a++] = "-L";
    argv[a++] = "--connect-timeout"; argv[a++] = "30";
    argv[a++] = "--retry"; argv[a++] = "2";
    argv[a++] = "--cacert"; argv[a++] = CA;
    argv[a++] = "-A"; argv[a++] = UA;
    argv[a++] = "-D"; argv[a++] = hdr;
    argv[a++] = "-o"; argv[a++] = tmp;
    argv[a++] = "-w"; argv[a++] = "%{http_code}";
    if (machine[0]) { snprintf(xm, sizeof xm, "X-Machine: %s", machine); argv[a++] = "-H"; argv[a++] = xm; }
    if (firmware[0]) { snprintf(xf, sizeof xf, "X-Firmware: %s", firmware); argv[a++] = "-H"; argv[a++] = xf; }
    // Some repos (apt.bingner.com) 404 without X-Unique-ID; a dummy is accepted,
    // so the real UDID never leaves the device.
    argv[a++] = "-H"; argv[a++] = "X-Unique-ID: 0000000000000000000000000000000000000000";
    // apt keeps the current copy in lists/ and only expects "IMS-Hit: true" back.
    if (lm[0]) { argv[a++] = "-z"; argv[a++] = lm; }
    argv[a++] = uri;
    argv[a] = NULL;

    // curl: stdin </dev/null, stdout (http code) and stderr (errors) -> out file.
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_addopen(&fa, 0, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_adddup2(&fa, ofd, 1);
    posix_spawn_file_actions_adddup2(&fa, ofd, 2);
    pid_t pid;
    int rc = posix_spawn(&pid, CURL, &fa, NULL, (char *const *)argv, environ);
    posix_spawn_file_actions_destroy(&fa);
    close(ofd);
    int status = 0;
    if (rc == 0) while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}

    char outbuf[2048], hdrbuf[16384], newlm[128];
    slurp(out, outbuf, sizeof outbuf);
    slurp(hdr, hdrbuf, sizeof hdrbuf);
    unlink(out); unlink(hdr);
    last_modified(hdrbuf, newlm, sizeof newlm);

    // curl prints the 3-digit code last (after any error text on stderr).
    size_t ol = strlen(outbuf);
    int code = ol >= 3 ? atoi(outbuf + ol - 3) : 0;

    if (rc != 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        if (ol >= 3) outbuf[ol - 3] = 0;
        one_line(outbuf);
        send_msg("400 URI Failure\nURI: %s\nMessage: %s", uri,
                 rc ? strerror(rc) : (outbuf[0] ? outbuf : "curl failed"));
        unlink(tmp);
    } else if (code == 304) {
        send_msg("201 URI Done\nURI: %s\nFilename: %s\nLast-Modified: %s\nIMS-Hit: true",
                 uri, dest, newlm[0] ? newlm : lm);
        unlink(tmp);
    } else if (code < 200 || code >= 400) {
        send_msg("400 URI Failure\nURI: %s\nMessage: HTTP %d", uri, code);
        unlink(tmp);
    } else if (rename(tmp, dest) != 0) {
        send_msg("400 URI Failure\nURI: %s\nMessage: rename to %s: %s", uri, dest, strerror(errno));
        unlink(tmp);
    } else {
        chmod(dest, 0644);
        char md5[33], sha1[41], sha256[65];
        long long size = hash_file(dest, md5, sha1, sha256);
        char lmline[160] = "";
        if (newlm[0]) snprintf(lmline, sizeof lmline, "\nLast-Modified: %s", newlm);
        send_msg("201 URI Done\nURI: %s\nFilename: %s\nSize: %lld%s\n"
                 "MD5-Hash: %s\nMD5Sum-Hash: %s\nSHA1-Hash: %s\nSHA256-Hash: %s",
                 uri, dest, size, lmline, md5, md5, sha1, sha256);
    }
}

// Value of "Tag: value" inside a message, or "" if absent.
static void tag(const char *msg, const char *name, char *out, size_t cap) {
    out[0] = 0;
    size_t nl = strlen(name);
    for (const char *p = msg; *p;) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) : strlen(p);
        if (len > nl + 1 && strncmp(p, name, nl) == 0 && p[nl] == ':') {
            const char *v = p + nl + 1;
            while (*v == ' ') v++;
            snprintf(out, cap, "%.*s", (int)(len - (v - p)), v);
            return;
        }
        if (!eol) break;
        p = eol + 1;
    }
}

static void handle(char *msg) {
    if (strncmp(msg, "600 ", 4) != 0) return;  // 601 Configuration etc.: ignore
    char uri[2048], file[1024], lm[128];
    tag(msg, "URI", uri, sizeof uri);
    tag(msg, "Filename", file, sizeof file);
    tag(msg, "Last-Modified", lm, sizeof lm);
    if (!uri[0] || !file[0]) return;
    fetch(uri, file, lm);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    read_firmware();
    send_msg("100 Capabilities\nVersion: 1.2\nPipeline: false\nSend-Config: true");

    static char buf[1 << 17];
    size_t len = 0;
    for (;;) {
        ssize_t r = read(0, buf + len, sizeof buf - 1 - len);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) break;
        len += r;
        buf[len] = 0;
        char *start = buf, *end;
        while ((end = strstr(start, "\n\n"))) {
            *end = 0;
            while (*start == '\n') start++;
            handle(start);
            start = end + 2;
        }
        len -= start - buf;
        memmove(buf, start, len);
        if (len == sizeof buf - 1) len = 0;  // oversized garbage: drop it
    }
    return 0;
}
