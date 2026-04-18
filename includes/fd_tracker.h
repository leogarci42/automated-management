#ifndef FD_TRACKER_H
#define FD_TRACKER_H

#ifdef TRACK_FD

#include <stdio.h>
#include <stdarg.h>

/* Forward declarations for intercepts */
int tracked_open(const char *src_file, int src_line, const char *pathname, int flags, ...);
int tracked_close(const char *src_file, int src_line, int fd);
int tracked_dup(const char *src_file, int src_line, int oldfd);
int tracked_dup2(const char *src_file, int src_line, int oldfd, int newfd);
int tracked_pipe(const char *src_file, int src_line, int pipefd[2]);
FILE *tracked_fopen(const char *src_file, int src_line, const char *pathname, const char *mode);
int tracked_fclose(const char *src_file, int src_line, FILE *stream);

/* Macro overrides injecting __FILE__ and __LINE__ */
#define open(...) tracked_open(__FILE__, __LINE__, __VA_ARGS__)
#define close(fd) tracked_close(__FILE__, __LINE__, fd)
#define dup(oldfd) tracked_dup(__FILE__, __LINE__, oldfd)
#define dup2(oldfd, newfd) tracked_dup2(__FILE__, __LINE__, oldfd, newfd)
#define pipe(pfd) tracked_pipe(__FILE__, __LINE__, pfd)
#define fopen(path, mode) tracked_fopen(__FILE__, __LINE__, path, mode)
#define fclose(stream) tracked_fclose(__FILE__, __LINE__, stream)

#endif /* TRACK_FD */

#endif /* FD_TRACKER_H */
