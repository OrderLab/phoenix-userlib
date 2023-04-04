#include "orbit.h"
#include <pthread.h>

typedef unsigned long long bitmap_t;
typedef unsigned char byte;

/* Page metadata. Including bitmap and some others. */
struct page_meta {
	pthread_spinlock_t lock;	/* Page level lock (maybe not used) */
	unsigned int used;		/* Used blocks in page */
	bitmap_t bitmap[2];		/* Bitmap of free blocks */
	byte free_count[8];		/* Used for optimization */
};

/* Structure of the bitmap allocator information.
 * This resides at the beginning of the pool.  The actual page used for
 * allocation starts after all the page metadata. */
struct orbit_bitmap_allocator {
	struct orbit_allocator base;	/* Allocator base struct, includes vtable */
	pthread_spinlock_t lock;	/* Global lock (maybe not used) */
	byte *first_page;		/* Start of data pages */
	size_t npages;			/* Number of pages */
	size_t allocated;		/* Last page that contains data */
	size_t *data_length;		/* External data length field to be updated */
	struct page_meta page_meta[];	/* Metadata of all pages */
};

/* Create an allocator */
struct orbit_allocator *orbit_bitmap_allocator_create(void *start, size_t length,
		void **data_start, size_t *data_length);

extern struct orbit_allocator_vtable orbit_bitmap_allocator_vtable;
