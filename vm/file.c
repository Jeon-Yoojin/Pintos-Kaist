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
	struct lazy_load_arg * arg = (struct lazy_load_arg *)page->uninit.aux;

	struct file_page *file_page = &page->file;
	file_page->file = arg->file;
	file_page->file_ofs = arg->ofs;
	file_page->read_bytes = arg->read_bytes;
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
	struct supplemental_page_table *spt = &thread_current()->spt;
	struct file_page *arg = &page->file;
		if (pml4_is_dirty(thread_current()->pml4, page->va)){
			/* 어떤 offset부터 썼는지 확인 후 그 offset부터 write */
			file_write_at(arg->file, page->va, arg->read_bytes, arg->file_ofs);
			/* dirty bit 0으로 set */
			pml4_set_dirty(thread_current()->pml4, page->va, 0);
		}
	pml4_clear_page(thread_current()->pml4, page->va);
	// struct file_page *file_page UNUSED = &page->file;
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
			/* 한 페이지만큼을 읽고 가상주소에 매핑 -> 페이지 새로 만들기 */
			struct file *f = file_reopen(file);
			uint32_t read_bytes = length > file_length(f)? file_length(f) : length;
			uint32_t zero_bytes = PGSIZE - read_bytes % PGSIZE;
			/* 시작 주소를 return해야 한다. */
			void *ret_val = addr;
			
			ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
			ASSERT(pg_ofs(addr) == 0);	 // upage가 페이지 정렬되어 있는지 확인
			ASSERT(offset % PGSIZE == 0) // ofs가 페이지 정렬되어 있는지 확인

			while (read_bytes > 0 || zero_bytes > 0)
			{
				/* Do calculate how to fill this page.
				* We will read PAGE_READ_BYTES bytes from FILE
				* and zero the final PAGE_ZERO_BYTES bytes. */
				size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
				size_t page_zero_bytes = PGSIZE - page_read_bytes;

				struct lazy_load_arg *aux = NULL;
				aux = (struct lazy_load_arg*)malloc(sizeof(struct lazy_load_arg));
				aux->file = f;
				aux->ofs = offset;
				aux->read_bytes = page_read_bytes;
				aux->zero_bytes = page_zero_bytes;

				if (!vm_alloc_page_with_initializer(VM_FILE, addr,
													writable, lazy_load_segment, aux))
					return NULL;

				struct page *p = spt_find_page(&thread_current()->spt, addr);
				p->page_cnt = read_bytes / PGSIZE + 1;
				
				/* Advance. */
				read_bytes -= page_read_bytes;
				zero_bytes -= page_zero_bytes;
				addr += PGSIZE;
				offset += page_read_bytes;
			}
			return ret_val;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	// page의 전체 길이 -> spt_find_page로 해당 addr를 찾아 그 페이지 구조체의 길이 얻어오기
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	off_t page_cnt = page->page_cnt;
	
	/* 변경된 파일은 쓴 후, dirty bit 원래대로 돌려주기
	spt 테이블에서 삭제하고 pml4 테이블에서 삭제*/
	for (int i = 0; i < page_cnt; i++)
	{
		addr += PGSIZE;
		if (page)
		{
			/* spt_remove_page -> vm_dealloc_page -> destroy(file_destroy)순으로 호출 */
			spt_remove_page(&thread_current()->spt, page);		
		}
		page = spt_find_page(&thread_current()->spt, addr);
	}
	
}