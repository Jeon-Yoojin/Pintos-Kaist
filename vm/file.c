/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	/* is_dirty 0으로 set하기 */
	/* 파일에 쓰기 */
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
			/* fd로 열린 파일의 길이가 0바이트인 경우 mmap에 대한 호출이 실패할 수 있습니다.
			or length가 0일 때
			addr이 page-aligned되지 않았거나,
			기존 매핑된 페이지 집합(실행가능 파일이 동작하는 동안 매핑된 스택 또는 페이지를 포함)과 겹치는 경우 실패해야 합니다. 
			-> length도 확인하고, spt_find_page로 page를 찾는다.
			*/
			/* 파일의 오프셋 바이트부터 length 만큼을 프로세스의 가상주소공간의 addr에 매핑*/
			/* 한 페이지만큼을 읽고 가상주소에 매핑 -> 페이지 새로 만들고 do_claim */
			uint32_t read_bytes = length;
			uint32_t zero_bytes;
			uint8_t *upage = (uint8_t *)addr;
			off_t ofs = offset;

			while (read_bytes > 0 || zero_bytes > 0)
			{
				/* Do calculate how to fill this page.
				* We will read PAGE_READ_BYTES bytes from FILE
				* and zero the final PAGE_ZERO_BYTES bytes. */
				size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
				size_t page_zero_bytes = PGSIZE - page_read_bytes;

				struct lazy_load_arg *aux = NULL;
				aux = (struct lazy_load_arg*)malloc(sizeof(struct lazy_load_arg));
				aux->file = file;
				aux->ofs = ofs;
				aux->read_bytes = page_read_bytes;
				aux->zero_bytes = page_zero_bytes;

				if (!vm_alloc_page_with_initializer(VM_FILE, upage,
													writable, lazy_load_segment, aux))
					return false;

				/* Advance. */
				read_bytes -= page_read_bytes;
				zero_bytes -= page_zero_bytes;
				upage += PGSIZE;
				ofs += page_read_bytes;
			}
			return true;
}

/* Do the munmap */
void
do_munmap (void *addr) {
}