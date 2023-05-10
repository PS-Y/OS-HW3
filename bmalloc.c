#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include "bmalloc.h"

#define BLOCK_SIZE 12
#define MIN_BLOCK_SIZE 4 // 분할할 블록의 최소 크기
#define HEADER_SIZE 9

bm_option bm_mode = BestFit;
bm_header_ptr bm_list_head = NULL;

#if 1
#define DPRINT(func) func;
#else
#define DPRINT(func)
#endif

void *init_buddy_heap()
{

	bm_header_ptr block = mmap(NULL, pow(2, BLOCK_SIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (block == MAP_FAILED)
	{
		perror("mmap failed");
		exit(1);
	}
	block->used = 0;
	block->size = BLOCK_SIZE;
	block->next = NULL;

	return block;
}

/*
	head의 예상 sibling header 주소를 return한다.
	h와 형제의 블록 크기가 다를 경우 sibling(h)가 h의 형제가 아닐 수 있습니다.
*/
// void *sibling(void *h)
// {
// 	// TODO
// 	// 주어진 블록의 형제 블록 주소를 반환
// 	int block_size = 1 << h->size;
// 	bm_header *parent = (bm_header *)((char *)h - block_size);
// 	int parent_size = h->size + 1;
// 	int parent_level = parent_size - 1;
// 	int parent_block_size = 1 << parent_size;
// 	int parent_index = ((char *)parent - (char *)free_list) / parent_block_size;
// 	int sibling_index = (parent_index % 2 == 0) ? (parent_index + 1) : (parent_index - 1);
// 	bm_header *sibling = (bm_header *)((char *)free_list + sibling_index * parent_block_size);
// 	return (void *)sibling;
// }
/*
	주어진 크기 s에 맞는 가장 작은 블록의 크기 필드 값을 반환한다.
*/
int fitting(size_t s)
{
	// TODO
	// 요청한 크기 s에 맞는 가장 작은 블록의 크기 필드 값을 반환
	int size = 4;
	while (pow(2, size) < s)
	{
		size++;
	}
	return size;
}
// 블록을 반으로 분할하고, 나머지 블록을 반환
bm_header_ptr split_block(bm_header_ptr block)
{
	bm_header_ptr second_next = block->next;

	// Split the block into two half-byte blocks
	bm_header_ptr first_block = block;
	bm_header_ptr second_block = (bm_header_ptr)((char *)block + (int)pow(2, block->size - 1));

	// Update the header of the first block
	first_block->used = 0;
	first_block->size = block->size - 1;
	first_block->next = second_block;
	// Update the header of the second block
	second_block->used = 0;
	second_block->size = block->size - 1;
	second_block->next = second_next;

	return first_block;
}

/*
	주어진 s에 대해 맞는 블록이 있으면 그 블록을 사용. 만약 맞는 블록이 없다면,
	"BestFit(기본값)"인 경우, Fitting block size 보다 큰 unused block 중에서 가장 작은 블록을 선택하여 분할한다.
	"FirstFit"인 경우, select the first feasible block in the linked list, 이를 분할하여 피팅 블록을 가져온다.
*/
void *bmalloc(size_t s)
{
	// TODO
	if (bm_list_head == NULL)
	{
		// 새로운 4096 블락 생성
		bm_list_head = init_buddy_heap();
		// TEST
		DPRINT(printf("%d, %d, %p\n", bm_list_head->used, bm_list_head->size, bm_list_head->next));
	}
	// 들어온 s에 fit 한 박스 크기 선언
	int size = fitting(s);

	//* DEBUGGING
	DPRINT(printf("fitted size: %d\n", size));

	// 현재 블럭이 이미 사용중 일 때
	while (bm_list_head->used == 1)
	{
		if (bm_list_head->next != NULL)
		{
			DPRINT(printf("현재블럭 사용중:\n현재 %p\n다음 %p\n", bm_list_head, bm_list_head->next));
			// next에 블럭이 걸려있다면
			bm_list_head = bm_list_head->next;
		}
		else
		{
			bm_header_ptr bm_new_block = NULL;
			bm_new_block = init_buddy_heap();
			bm_list_head->next = bm_new_block;
		}
	}
	// 여기는 무조건 used = 0
	// 현재 블락의 크기가 fitting size보다 더 클 경우 -> 블락을 나눠야함.
	while (bm_list_head->size > size)
	{
		// 해당 크기의 블록이 없으면 더 큰 블록에서 분할
		bm_list_head = split_block(bm_list_head);

		//* DEBUGGING
		DPRINT(printf("split size: %d\n", bm_list_head->size));
	}

	// 현재 블럭보다 들어온 사이즈가 더 클 때
	while (bm_list_head->size < size)
	{
		if (bm_list_head->next != NULL)
		{
			DPRINT(printf("현재블럭<입력사이즈: \n현재 %p\n다음 %p\n", bm_list_head, bm_list_head->next));
			// next에 블럭이 걸려있다면
			bm_list_head = bm_list_head->next;
		}
		else
		{
			bm_header_ptr bm_new_block = NULL;
			bm_new_block = init_buddy_heap();
			bm_list_head->next = bm_new_block;
		}
	}
	while (bm_list_head->size == size)
	{
		// arriving test
		DPRINT(printf("arrive_start - %d, %d, %p\n", bm_list_head->used, bm_list_head->size, bm_list_head + 9));
		bm_list_head->used = 1;
		// arriving test
		DPRINT(printf("1arrive_end - %d, %d, %p\n", bm_list_head->used, bm_list_head->size, bm_list_head + 9));
		// payload의 시작주소를 리턴
		return (void *)bm_list_head + 9;
	}
	bm_list_head = bmalloc(s);
}
/*
	사용 중인 블록을 사용하지 않는 상태로 전환 후, 만약 sibling이 모두 사용되고 있지 않다면 merge, merging의 경우는 가능한 경우까지 상위로 계속 반복해야 함. 전체 페이지가 모두 비었다면 페이지를 해제한다.
*/
// void bfree(void *p)
// {
// 	bm_header_ptr block = (bm_header_ptr)p - 1; // 1?
// 	// 미사용으로 변경
// 	block->used = 0;

// 	// Merge with buddy if possible
// 	while (block_index < NUM_BLOCK_SIZES - 1)
// 	{
// 		block_header *buddy = get_buddy(header, block_size);
// 		if (!buddy->is_free)
// 		{
// 			break;
// 		}

// 		// Remove buddy from the free list
// 		remove_from_list(buddy);

// 		// Merge with buddy
// 		header = get_merged_block(header, buddy);
// 		block_size *= 2;
// 		block_index++;
// 	}
// }
/*
	allocate된 메모리 버퍼 크기를 주어진 s 사이즈로 조정한다.
	이 결과로 데이터는 다른 주소로 이동될 수 있다. perform like realloc().
*/
// void *brealloc(void *p, size_t s)
// {
// 	// TODO
// 	bm_header_ptr block = (bm_header_ptr)p - 1;
// 	int old_size = pow(2, block->size);
// 	int new_size;
// 	if (new_size == old_size)
// 	{
// 		// 크기가 같으면 그대로 반환
// 		return p;
// 	}
// 	else if (new_size < old_size)
// 	{
// 		// 블록을 분할해서 크기를 맞춤
// 		while (block->size > log2(new_size))
// 		{
// 			bm_header_ptr buddy = split_block(block);
// 			buddy->used = 0;
// 		}
// 		return p;
// 	}
// 	else
// 	{
// 		// 새로운 블록을 할당하고 데이터를 복사한 후 이전 블록 해제
// 		void *new_block = bmalloc(s);
// 		if (new_block == NULL)
// 		{
// 			return NULL;
// 		}
// 		memcpy(new_block, p, old_size);
// 		bfree(p);
// 		return new_block;
// 	}
// }
/*
	bestfit, firstfit 사이의 관리 방식을 설정해준다.
*/
void bmconfig(bm_option opt)
{
	// TODO
	bm_mode = opt;
}
/* 연결된 리스트의 각 블록 통계 및 상태 표시 */
void bmprint()
{
	bm_header_ptr itr;
	int i;

	printf("==================== bm_list ====================\n");
	for (itr = bm_list_head->next, i = 0; itr != 0x0; itr = itr->next, i++)
	{
		printf("%3d:%p:%1d %8d:", i, ((void *)itr) + sizeof(bm_header), (int)itr->used, (int)itr->size);

		int j;
		char *s = ((char *)itr) + sizeof(bm_header);
		for (j = 0; j < (itr->size >= 8 ? 8 : itr->size); j++)
			printf("%02x ", s[j]);
		printf("\n");
	}
	printf("=================================================\n");

	// TODO: print out the stat's.
}

int main()
{
	printf("TEST\n");
	void *p1, *p2, *p3;
	p1 = bmalloc(2000);
	p2 = bmalloc(2500);
	p3 = bmalloc(1000);
}