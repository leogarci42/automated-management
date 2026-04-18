#ifdef TRACK_FD

/* Undefine the macros in this file so we can call original functions */
#undef open
#undef close
#undef dup
#undef dup2
#undef pipe
#undef fopen
#undef fclose

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include "fd_tracker.h"

#define MAX_FDS 4096

typedef struct {
    int active;
    char *path;
    const char *src_file;
    int src_line;
} fd_info_t;

static fd_info_t fd_registry[MAX_FDS] = {0};
static pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;

static void register_fd(int fd, const char *path, const char *src_file, int src_line) {
    if (fd >= 0 && fd < MAX_FDS) {
        pthread_mutex_lock(&fd_mutex);
        if (fd_registry[fd].active && fd_registry[fd].path) {
            free(fd_registry[fd].path);
        }
        fd_registry[fd].active = 1;
        fd_registry[fd].path = path ? strdup(path) : strdup("unknown");
        fd_registry[fd].src_file = src_file;
        fd_registry[fd].src_line = src_line;
        pthread_mutex_unlock(&fd_mutex);
    }
}

static void unregister_fd(int fd, const char *src_file, int src_line) {
    if (fd >= 0 && fd < MAX_FDS) {
        pthread_mutex_lock(&fd_mutex);
        if (!fd_registry[fd].active && fd > 2) {
            fprintf(stderr, "\n=================================================================\n");
            fprintf(stderr, "FD-Sanitizer: ERROR: Double close or invalid FD: %d\n", fd);
            fprintf(stderr, "  Closed at %s:%d\n", src_file, src_line);
            fprintf(stderr, "=================================================================\n\n");
        } else {
            fd_registry[fd].active = 0;
            if (fd_registry[fd].path) {
                free(fd_registry[fd].path);
                fd_registry[fd].path = NULL;
            }
        }
        pthread_mutex_unlock(&fd_mutex);
    }
}

int tracked_open(const char *src_file, int src_line, const char *pathname, int flags, ...) {
    int mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }
    // Use raw syscall to avoid routing back into open() unpredictably
    int fd = syscall(SYS_open, pathname, flags, mode);
    if (fd >= 0) {
        register_fd(fd, pathname, src_file, src_line);
    }
    return fd;
}

int tracked_close(const char *src_file, int src_line, int fd) {
    unregister_fd(fd, src_file, src_line);
    return syscall(SYS_close, fd);
}

int tracked_dup(const char *src_file, int src_line, int oldfd) {
    int newfd = syscall(SYS_dup, oldfd);
    if (newfd >= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "dup of FD %d", oldfd);
        register_fd(newfd, buf, src_file, src_line);
    }
    return newfd;
}

int tracked_dup2(const char *src_file, int src_line, int oldfd, int newfd) {
    int ret = syscall(SYS_dup2, oldfd, newfd);
    if (ret >= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "dup2 of FD %d", oldfd);
        register_fd(newfd, buf, src_file, src_line);
    }
    return ret;
}

int tracked_pipe(const char *src_file, int src_line, int pipefd[2]) {
    int ret = syscall(SYS_pipe, pipefd);
    if (ret == 0) {
        register_fd(pipefd[0], "pipe read-end", src_file, src_line);
        register_fd(pipefd[1], "pipe write-end", src_file, src_line);
    }
    return ret;
}

FILE *tracked_fopen(const char *src_file, int src_line, const char *pathname, const char *mode) {
    FILE *f = (fopen)(pathname, mode); 
    if (f) register_fd(fileno(f), pathname, src_file, src_line);
    return f;
}

int tracked_fclose(const char *src_file, int src_line, FILE *stream) {
    if (stream) unregister_fd(fileno(stream), src_file, src_line);
    return (fclose)(stream);
}

__attribute__((destructor)) static void check_fd_leaks(void) {
    int leak_count = 0;
    pthread_mutex_lock(&fd_mutex);
    fprintf(stderr, "\n=================================================================\n");
    fprintf(stderr, "FD-Sanitizer: File Descriptor Leak Report\n");
    fprintf(stderr, "=================================================================\n");
    
    // We start from 3 to skip tracking standard stdin/stdout/stderr
    for (int i = 3; i < MAX_FDS; i++) {
        if (fd_registry[i].active) {
            fprintf(stderr, "LEAK: FD %d open in %s:%d\n", i, fd_registry[i].src_file, fd_registry[i].src_line);
            fprintf(stderr, "  Path/Details: %s\n\n", fd_registry[i].path);
            leak_count++;
            
            // Clean up surviving memory in the tracker
            free(fd_registry[i].path);
            fd_registry[i].path = NULL;
        }
    }
    
    if (leak_count == 0) {
        fprintf(stderr, "SUMMARY: No file descriptor leaks detected.\n");
    } else {
        fprintf(stderr, "SUMMARY: %d file descriptor(s) leaked.\n", leak_count);
    }
    fprintf(stderr, "=================================================================\n\n");
    pthread_mutex_unlock(&fd_mutex);
}

#endif /* TRACK_FD */
