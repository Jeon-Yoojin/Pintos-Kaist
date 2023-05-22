/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	/* 
		1. swap disk 생성 -> disk_get(1, 1)
		2. disk_size로 sector 개수가 몇 개 있는지 확인
		3. slot 구조체 생성->page, swap-elem, slot-number
		4. swap table에 빈 슬롯을 다 넣어줌
		5. list_init swap_table 해줌
		6. list_init frame_table 해줌 */
	swap_disk = NULL;
	swap_disk = disk_get(1, 1);

	/* disk size에 따라 sector 몇 개 있는지 확인후 list에 집어넣기 */
	list_init(&swap_table);
	lock_init(&swap_lock);

	disk_sector_t size = disk_size(swap_disk); //sector size
	/* swap_table에 slot 넣어 주기 */
	for (int i = 0; i < size; i = i + 8)
	{
		struct slot *new_slot = (struct slot *)malloc(sizeof(struct slot));
		new_slot->page = NULL;
		new_slot->slot_number = i / 8; // sector size로 나눠 줌
		list_push_back(&swap_table, &new_slot->swap_elem);
	}
}

/* Initialize the file mapping */
/* 필요에 따라 추가할 수 있음 */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct list_elem *e;
	struct slot *s;
	struct anon_page *anon_page = &page->anon;
	/* frame에 있는 page와 frame을 연결해 준다 */
	lock_acquire(&swap_lock);
	for (e = list_begin (&swap_table); e != list_end (&swap_table); e = list_next (e)) {
		/* swap_table을 돌면서 리스트 안의 페이지가 현재 찾는 페이지인지 확인 */
		s = list_entry(e, struct slot, swap_elem);
		if (s->page == page)
		{
			struct thread *t = thread_current();

			// 다시 frame table에 해당 frame을 올림
			// list_push_back(&frame_table, &page->frame->frame_elem);
			// swap disk의 sector num부터 한 페이지를 읽어 옴
			disk_sector_t sector_num = (s->slot_number) * 8;
			void *addr = kva;

			for (int i = sector_num; i < sector_num + 8; i++)
			{
				disk_read(swap_disk, i, addr);
				addr += DISK_SECTOR_SIZE;
			}
			s->page = NULL;
			lock_release(&swap_lock);
			return true;
		}
	}
	lock_release(&swap_lock);
	return false;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct list_elem *e;
	struct slot *s;
	struct anon_page *anon_page = &page->anon;
	/* swap table의 빈 곳 찾기 -> page 할당 -> slot num 찾아 disk_write 하기 */
	lock_acquire(&swap_lock);
	for (e = list_begin (&swap_table); e != list_end (&swap_table); e = list_next (e)) {
		/* swap_table을 돌면서 리스트 안의 페이지가 NULL인지 확인 */
		s = list_entry(e, struct slot, swap_elem);
		/* slot_num을 통해 sector num을 계산 */
		if (s->page == NULL)
		{
			disk_sector_t sector_num = (s->slot_number) * 8;
			/* disk_write으로 해당 슬롯에 page를 써 줌 */
			char *addr = page->frame->kva;
			for (int i = sector_num; i < sector_num + 8; i++)
			{
				disk_write(swap_disk, i, addr);
				addr += DISK_SECTOR_SIZE;
			}
			// [수정] slot의 page를 추가해 주기
			s->page = page;
			break;
		}
	}
	lock_release(&swap_lock);
	/* 만약 끝까지 탐색했는데 swap table에 자리가 없는 경우 */
	if (e == list_end(&swap_table))
	{
		PANIC("TODO");
		return false;
	}

	/* frame-page 연결 끊기 */
	page->frame->page = NULL;
	page->frame = NULL;
	/* pml4_clear */
	pml4_clear_page(thread_current()->pml4, page->va);
	
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
