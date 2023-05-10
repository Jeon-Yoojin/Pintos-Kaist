#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/palloc.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
	filesys_lock = (struct lock *)malloc(sizeof(struct lock));
	lock_init(filesys_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	// printf ("system call!\n");
	
	// 시스템 콜(halt, exit,create, remove)을 구현하고 시스템 콜 핸들러를 통해 호출
	check_address(f->rsp);
	switch (f->R.rax) {
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			f->R.rax = fork((const char *)f->R.rdi, f);
			break;
		case SYS_EXEC:
			f->R.rax = exec((const int *)f->R.rdi);
			break;
		case SYS_WAIT:
			f->R.rax = wait((tid_t)f->R.rdi);
			break;
		case SYS_CREATE:
			/* filename, filesize*/
			f->R.rax = create((char *)f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			/* filename */
			f->R.rax = remove((char *)f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = open ((char *)f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize((int)f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = read((int)f->R.rdi, (void *)f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write((int)f->R.rdi, (void *)f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek((int)f->R.rdi, (unsigned int)f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell((int)f->R.rdi);
			break;
		case SYS_CLOSE:
			close((int)f->R.rdi);
			break;
		default:
			thread_exit();
			break;
	}
}

void check_address (void *addr) {
	/* 주소 값이 유저 영역에서 사용하는 주소 값인지 확인 하는 함수
	유저 영역을 벗어난 영역일 경우 프로세스 종료(exit(-1)) */
	struct thread *curr = thread_current();
	if (!is_user_vaddr(addr) || !pml4_get_page(thread_current()->pml4,addr) || !addr) {
		exit(-1);
	}
}

void halt (void) {
	/* 핀토스 종료 */
	power_off ();
}

void exit (int status) {
	/* 실행중인 스레드 구조체를 가져옴 */
	struct thread *curr = thread_current();
	curr->exit_status  = status;
	/* 프로세스 종료 메시지 출력, 
	출력 양식: “프로세스이름: exit(종료상태)” */ 
	printf ("%s: exit(%d)\n", curr->name, status);
	/* 스레드 종료 */
	thread_exit();
}

bool create(const char *file , unsigned initial_size)
{
	if (file == NULL) {
		exit(-1);
	}
	/* 파일 이름과 크기에 해당하는 파일 생성 */
	bool success = filesys_create (file, initial_size);
	/* 파일 생성 성공 시 true 반환, 실패 시 false 반환 */
	if (success) {
		return true;
	}
	else {
		return false;
	}
}

bool remove(const char *file)
{
	if (file == NULL) {
		exit(-1);
	}
	/* 파일 이름에 해당하는 파일을 제거 */
	bool success = filesys_remove(file);
	/* 파일 제거 성공 시 true 반환, 실패 시 false 반환 */
	if (success) {
		return true;
	}
	else {
		return false;
	}
}

int open (const char *file)
{
	if (file == NULL) {
		exit(-1);
	}
	/* 파일을 open */
	struct file *fp = filesys_open(file);
	if (fp == NULL) return -1;
	/* 해당 파일 객체에 파일 디스크립터 부여 */
	int fd = process_add_file(fp);
	/* 파일 디스크립터 리턴 */
	if (fd != -1) {
		return fd;
	}
	else{
		file_close(fp);
		return -1;
	}
}

int filesize (int fd)
{
	/* 파일 디스크립터를 이용하여 파일 객체 검색 */
	struct file *fp = process_get_file(fd);
	/* 해당 파일의 길이를 리턴 */
	if (fp != NULL) {
		return file_length(fp);
	}
	else {
		/* 해당 파일이 존재하지 않으면 -1 리턴 */
		return -1;
	}
}

int read (int fd, void *buffer, unsigned size)
{
	/* 파일에 동시 접근이 일어날 수 있으므로 Lock 사용 */
	lock_acquire(filesys_lock);
	/* 파일 디스크립터를 이용하여 파일 객체 검색 */
	struct file *fp = process_get_file(fd);
	/* 파일 디스크립터가 0일 경우 키보드에 입력을 버퍼에 저장 후
	버퍼의 저장한 크기를 리턴 (input_getc() 이용) */
	if (fd == 0) {
		int buf_size = input_getc();
		lock_release(filesys_lock);
		return size;
	}
	/* 파일 디스크립터가 0이 아닐 경우 파일의 데이터를 크기만큼 저
	장 후 읽은 바이트 수를 리턴 */
	else {
		off_t read_size = file_read(fp, buffer, size);
		lock_release(filesys_lock);
		return read_size;
	}
}

int write(int fd, void *buffer, unsigned size)
{
	/* 파일에 동시 접근이 일어날 수 있으므로 Lock 사용 */
	lock_acquire(filesys_lock);
	/* 파일 디스크립터를 이용하여 파일 객체 검색 */
	struct file *fp = process_get_file(fd);
	/* 파일 디스크립터가 1일 경우 버퍼에 저장된 값을 화면에 출력
	후 버퍼의 크기 리턴 (putbuf() 이용) */
	if (fd == 1) {
		putbuf((char *)buffer, size);
		/* TODO: 버퍼 사이즈 리턴 */
		lock_release(filesys_lock);
		return strlen((char *)buffer);
	}
	/* 파일 디스크립터가 1이 아닐 경우 버퍼에 저장된 데이터를 크기
	만큼 파일에 기록후 기록한 바이트 수를 리턴 */
	else {
		off_t write_size = file_write(fp, buffer, size);
		lock_release(filesys_lock);
		return write_size;
	}
}

void seek (int fd, unsigned position)
{
	/* 파일 디스크립터를 이용하여 파일 객체 검색 */
	struct file *fp = process_get_file(fd);
	/* 해당 열린 파일의 위치(offset)를 position만큼 이동 */
	file_seek(fp, position);
}

unsigned tell (int fd)
{
	/* 파일 디스크립터를 이용하여 파일 객체 검색 */
	/* 해당 열린 파일의 위치를 반환 */
	return (unsigned)process_get_file(fd);
}

void close (int fd)
{
	/* 해당 파일 디스크립터에 해당하는 파일을 닫음 */
	/* 파일 디스크립터 엔트리 초기화 */ 
	process_close_file(fd);
}

tid_t fork (const char *thread_name, struct intr_frame *f) {
	return process_fork(thread_name, f);
}

tid_t exec (const char *cmd_line)
{
	// /* 자식 프로세스 생성 */
	// tid_t pid = fork(cmd_line, &thread_current()->tf);
	/* 생성된 자식 프로세스의 프로세스 디스크립터를 검색 */
	char *new_file = palloc_get_page(PAL_USER);
	if (new_file == NULL) exit(-1);
	strlcpy(new_file, cmd_line, PGSIZE);
	if (process_exec(new_file) == -1) return -1;
}

int wait (tid_t pid)
{
	return process_wait(pid);
}