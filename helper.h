/*
 * helper.h - Robust I/O (RIO) package
 *
 * Provides safe and reliable read/write functions that handle partial reads/writes.
 * Also includes buffered I/O via rio_t for efficient line-based reading.
 */

#ifndef HELPER_H
#define HELPER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define RIO_BUFFER_SIZE 8192

/* Buffered I/O type */
typedef struct {
    int fd;                     /* File descriptor */
    ssize_t unread_bytes;       /* Number of unread bytes in buffer */
    char *buf_ptr;              /* Next unread byte in buffer */
    char buffer[RIO_BUFFER_SIZE]; /* Internal buffer */
} rio_t;

/* Robust I/O functions */
ssize_t rio_readn(int fd, void *user_buf, size_t n);     /* Read exactly n bytes */
ssize_t rio_writen(int fd, const void *user_buf, size_t n); /* Write exactly n bytes */

/* Buffered I/O initialization */
void rio_readinitb(rio_t *rp, int fd);

/* Buffered I/O functions */
ssize_t rio_readnb(rio_t *rp, void *user_buf, size_t n);       /* Read n bytes into buffer */
ssize_t rio_readlineb(rio_t *rp, void *user_buf, size_t maxlen); /* Read a line up to maxlen */

#endif /* HELPER_H */
