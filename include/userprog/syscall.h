#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "kernel/stdio.h"

void syscall_init (void);
struct lock *filesys_lock;
bool create(const char *file , unsigned initial_size);
bool remove(const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
int wait (tid_t pid);
tid_t fork (const char *thread_name, struct intr_frame *f);
tid_t exec (const *cmd_line);

#endif /* userprog/syscall.h */
