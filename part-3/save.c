#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *const checkpoints[] = {
    "truncated", "created", "written", "synced", "renamed", "dir-synced",
};

static const char *stop_at;

/* Імітує аварійне завершення процесу: SIGKILL не можна перехопити. */
static void checkpoint(const char *name)
{
    if (stop_at != NULL && strcmp(stop_at, name) == 0) {
        fprintf(stderr, "killed after %s\n", name);
        kill(getpid(), SIGKILL);
    }
}

static int write_all(int fd, const char *buf, size_t n)
{
    /* TODO 1: записати всі n байтів з buf у fd. Повернути 0, якщо все
     * записано, або -1 при помилці (errno — від write). */
    size_t total = 0;
    while (total < n) {
        ssize_t w = write(fd, buf + total, n - total);
        if (w == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (w == 0) { errno = EIO; return -1; }
        total += (size_t)w;
    }
    return 0;
}

static int replace_naive(const char *path, const char *data, size_t size)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror(path);
        return -1;
    }
    checkpoint("truncated");
    if (write_all(fd, data, size) == -1) {
        perror("write");
        close(fd);
        return -1;
    }
    if (close(fd) == -1) {
        perror("close");
        return -1;
    }
    return 0;
}

static void parent_dir(const char *path, char *dir, size_t size)
{
    const char *slash = strrchr(path, '/');
    if (slash == NULL)
        snprintf(dir, size, ".");
    else if (slash == path)
        snprintf(dir, size, "/");
    else
        snprintf(dir, size, "%.*s", (int)(slash - path), path);
}

static int replace_safe(const char *path, const char *data, size_t size)
{
    char tmp[PATH_MAX];
    char dir[PATH_MAX];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    parent_dir(path, dir, sizeof dir);

    /* TODO 2: створити tmp (або обрізати, якщо він уже є) і відкрити для запису. */
    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) { perror(tmp); return -1; }

    checkpoint("created");

    /* TODO 3: записати data у tmp. */
    if (write_all(fd, data, size) == -1) { perror("write"); close(fd); unlink(tmp); return -1; }

    checkpoint("written");

    /* TODO 4: дочекатися, поки дані tmp будуть на диску, і закрити tmp. */
    if (fsync(fd) == -1) { perror("fsync"); close(fd); unlink(tmp); return -1; }
    if (close(fd) == -1) { perror("close"); unlink(tmp); return -1; }

    checkpoint("synced");

    /* TODO 5: замінити path на tmp. */
    if (rename(tmp, path) == -1) { perror("rename"); unlink(tmp); return -1; }

    checkpoint("renamed");

    /* TODO 6: дочекатися, поки зміна каталогу dir буде на диску. */
    int dirfd = open(dir, O_RDONLY | O_DIRECTORY);
    if (dirfd == -1) { perror(dir); return -1; }
    if (fsync(dirfd) == -1) { perror("fsync dir"); close(dirfd); return -1; }
    if (close(dirfd) == -1) { perror("close dir"); return -1; }

    checkpoint("dir-synced");

    /* При будь-якій помилці в TODO 2–6: надрукувати її через perror і
     * повернути -1. Якщо помилка сталася до заміни (TODO 2–5), path має
     * лишитися як був, а tmp — не лишитися.
     * Коли все готово, замініть заглушку нижче на return 0. */
    return 0;
}

static char *read_input(size_t *size)
{
    size_t capacity = 65536;
    size_t length = 0;
    char *data = malloc(capacity);
    if (data == NULL) {
        perror("malloc");
        exit(1);
    }
    for (;;) {
        if (length == capacity) {
            capacity *= 2;
            data = realloc(data, capacity);
            if (data == NULL) {
                perror("realloc");
                exit(1);
            }
        }
        ssize_t n = read(STDIN_FILENO, data + length, capacity - length);
        if (n == 0)
            break;
        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            exit(1);
        }
        length += n;
    }
    *size = length;
    return data;
}

static int known_checkpoint(const char *name)
{
    for (size_t i = 0; i < sizeof checkpoints / sizeof checkpoints[0]; i++)
        if (strcmp(checkpoints[i], name) == 0)
            return 1;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s naive|safe FILE [CHECKPOINT] < NEW_CONTENT\n", argv[0]);
        return 2;
    }

    int (*replace)(const char *, const char *, size_t);
    if (strcmp(argv[1], "naive") == 0)
        replace = replace_naive;
    else if (strcmp(argv[1], "safe") == 0)
        replace = replace_safe;
    else {
        fprintf(stderr, "unknown mode: %s\n", argv[1]);
        return 2;
    }
    if (argc == 4) {
        if (!known_checkpoint(argv[3])) {
            fprintf(stderr, "unknown checkpoint: %s\n", argv[3]);
            return 2;
        }
        stop_at = argv[3];
    }

    size_t size;
    char *data = read_input(&size);
    if (replace(argv[2], data, size) == -1)
        return 1;
    fprintf(stderr, "saved %zu bytes to %s\n", size, argv[2]);
    free(data);
    return 0;
}
