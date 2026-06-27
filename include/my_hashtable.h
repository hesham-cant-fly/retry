/**
 * @file my_hashtable.h
 * @brief Generic open-addressing hash table with Robin Hood hashing.
 *
 * Items are stored directly in hash slots (no sparse index indirection).
 * Supports keys of any type: configure `HM_HASH`, `HM_EQ`, and
 * `HM_KEY_OFFSET` before including.
 *
 * Usage:
 * @code
 *   struct Person { char *name; int age; };
 *   struct People { struct Person *items; size_t len; size_t cap; };
 *
 *   // (optional) override defaults for non-string keys:
 *   // #define HM_KEY_OFFSET offsetof(struct Person, name)
 *   // #define HM_HASH(item_ptr) my_hash_fn(item_ptr)
 *   // #define HM_EQ(a, b) my_eq_fn(a, b)
 *
 *   #define MY_HASHTABLE_IMPL
 *   #include "my_hashtable.h"
 *
 *   int main(void) {
 *     struct Allocator alloc = get_c_allocator();
 *     struct People people = {0};
 *
 *     hmput(alloc, people, (&(struct Person){ "Alice", 30 }));
 *     hmput(alloc, people, (&(struct Person){ "Bob", 25 }));
 *
 *     struct Person *p = hmget(people, (&(struct Person){ .name = "Bob" }));
 *     if (p) printf("age: %d\n", p->age);
 *
 *     hmfree(alloc, people);
 *   }
 * @endcode
 *
 * Define `MY_HASHTABLE_IMPL` in exactly one translation unit.
 * Define `MY_HASHTABLE_DEF` to `static` before inclusion for internal linkage.
 */

#ifndef MY_HASHTABLE_H_
#define MY_HASHTABLE_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "my_allocator.h"

#ifndef MY_HASHTABLE_DEF
#  define MY_HASHTABLE_DEF
#endif

/* #define MY_HASHTABLE_IMPL */

#ifndef MY_HASHTABLE_CAPACITY
#  define MY_HASHTABLE_CAPACITY 16
#endif

#ifndef HM_LOAD_FACTOR_NUM
#  define HM_LOAD_FACTOR_NUM 7
#endif

#ifndef HM_LOAD_FACTOR_DEN
#  define HM_LOAD_FACTOR_DEN 10
#endif

/**
 * @name Key configuration macros
 * Override these before including the header for non-string keys.
 * @{
 */

#ifndef HM_KEY_OFFSET
/** Byte offset of the key field within the item struct (default 0 for `char *` as first member). */
#  define HM_KEY_OFFSET 0
#endif

#ifndef HM_HASH
/** Hash function. Receives `const void *` pointing to a full item. Returns `size_t`. */
#  define HM_HASH(item_ptr) _hm_hash_str(*(const char **)(item_ptr))
#endif

#ifndef HM_EQ
/** Equality function. Receives two `const void *` pointing to full items. Returns nonzero if equal. */
#  define HM_EQ(a, b) (strcmp(*(const char **)(a), *(const char **)(b)) == 0)
#endif

/** @} */

/* --- internal bookkeeping --- */

typedef struct hm_header {
	size_t item_size;
	size_t key_offset;
} hm_header_t;

#define _HM_OCC(items_, cap_, i_) (((uint8_t *)(items_) - (cap_) * 2)[(i_)])
#define _HM_DIST(items_, cap_, i_) (((uint8_t *)(items_) - (cap_))[(i_)])
#define _HM_HDR(items_, cap_) ((hm_header_t *)((char *)(items_) - (cap_) * 2 - sizeof(hm_header_t)))

/* --- forward declarations --- */

MY_HASHTABLE_DEF size_t _hm_hash_str(const char *str);
MY_HASHTABLE_DEF void _hm_create(allocator_t allocator, void **items_ptr, size_t *cap_ptr, size_t item_size);
MY_HASHTABLE_DEF void _hm_free(allocator_t allocator, void *items, size_t cap, size_t item_size);
MY_HASHTABLE_DEF void _hm_grow(allocator_t allocator, void **items_ptr, size_t *len_ptr, size_t *cap_ptr, size_t item_size);
MY_HASHTABLE_DEF void _hm_put_impl(allocator_t allocator, void **items_ptr, size_t *len_ptr, size_t *cap_ptr, size_t item_size, void *item);
MY_HASHTABLE_DEF void *_hm_get_impl(void *items, size_t len, size_t cap, size_t item_size, void *key_item);
MY_HASHTABLE_DEF void _hm_del_impl(void *items, size_t *len_ptr, size_t cap, size_t item_size, void *key_item);

/* --- public macros --- */

/** Initialize the hash table with default capacity. */
#define hmcreate(allocator_, hm_) \
	_hm_create((allocator_), (void **)&(hm_).items, &(hm_).cap, sizeof(*(hm_).items))

/** Insert or replace an item.
 *  @param allocator_  The allocator.
 *  @param hm_         The hash table struct (must have .items, .len, .cap).
 *  @param item_       Pointer to the item to insert (C89) or ... (C99).
 *
 *  In C99 you may pass a bare compound literal: `hmput(al, m, &(type){...})`.
 *  In C89 wrap it in parens or use a variable:   `hmput(al, m, (&(type){...}))`. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define hmput(allocator_, hm_, ...) \
	_hm_put_impl((allocator_), (void **)&(hm_).items, &(hm_).len, &(hm_).cap, sizeof(*(hm_).items), (void *)(__VA_ARGS__))
#else
#  define hmput(allocator_, hm_, item_) \
	_hm_put_impl((allocator_), (void **)&(hm_).items, &(hm_).len, &(hm_).cap, sizeof(*(hm_).items), (void *)(item_))
#endif

/** Look up an item by key.
 *  @param hm_          The hash table struct.
 *  @param key_item_    Pointer to an item with the key field set (only key is read).
 *  @return Pointer to the stored item, or NULL. */
#define hmget(hm_, key_item_) \
	_hm_get_impl((hm_).items, (hm_).len, (hm_).cap, sizeof(*(hm_).items), (void *)(key_item_))

/** Delete an item by key.
 *  @param hm_          The hash table struct.
 *  @param key_item_    Pointer to an item with the key field set. */
#define hmdel(hm_, key_item_) \
	_hm_del_impl((hm_).items, &(hm_).len, (hm_).cap, sizeof(*(hm_).items), (void *)(key_item_))

/** Free the entire hash table and reset to zero. */
#define hmfree(allocator_, hm_) \
	do { _hm_free((allocator_), (hm_).items, (hm_).cap, sizeof(*(hm_).items)); (hm_).items = NULL; (hm_).len = 0; (hm_).cap = 0; } while (0)

/** Current capacity (number of slots). */
#define hmcap(hm_) ((hm_).cap)

/** Number of occupied slots. */
#define hmlen(hm_) ((hm_).len)

#ifndef hmforeach
/** Iterate over occupied slots.  `i_` is a `size_t` variable. */
#  define hmforeach(hm_, i_) \
	for ((i_) = 0; (i_) < hmcap(hm_); (i_)++) \
		if (_HM_OCC((hm_).items, (hm_).cap, (i_)))
#endif /* !hmforeach */

/** Check if slot `i_` is occupied. */
#define hmoccupied(hm_, i_) (_HM_OCC((hm_).items, (hm_).cap, (i_)) != 0)

/* ====================================================================
 *  Implementation
 * ==================================================================== */

#ifdef MY_HASHTABLE_IMPL

#ifndef MY_HASHTABLE_DEF
#  error "MY_HASHTABLE_DEF must be defined before including implementation"
#endif

/** FNV-1a hash for null-terminated strings. */
MY_HASHTABLE_DEF size_t _hm_hash_str(const char *str)
{
	size_t h = 1469598103934665603ULL;
	while (*str != '\0') {
		h ^= (unsigned char)*str++;
		h *= 1099511628211ULL;
	}
	return h;
}

MY_HASHTABLE_DEF void _hm_create(allocator_t allocator, void **items_ptr,
                                  size_t *cap_ptr, size_t item_size)
{
	size_t cap;
	size_t total;
	uint8_t *mem;
	hm_header_t *hdr;

	cap = MY_HASHTABLE_CAPACITY;
	total = sizeof(hm_header_t) + cap * 2 + cap * item_size;
	mem = (uint8_t *)alloc(allocator, total);
	memset(mem, 0, total);
	hdr = (hm_header_t *)mem;
	hdr->item_size = item_size;
	hdr->key_offset = HM_KEY_OFFSET;
	*items_ptr = (void *)(mem + sizeof(hm_header_t) + cap * 2);
	*cap_ptr = cap;
}

MY_HASHTABLE_DEF void _hm_free(allocator_t allocator, void *items,
                                size_t cap, size_t item_size)
{
	hm_header_t *hdr;
	size_t total;

	if (!items) return;
	hdr = _HM_HDR(items, cap);
	total = sizeof(hm_header_t) + cap * 2 + cap * item_size;
	xdestroy(allocator, total, hdr);
}

MY_HASHTABLE_DEF void _hm_grow(allocator_t allocator, void **items_ptr,
                                size_t *len_ptr, size_t *cap_ptr,
                                size_t item_size)
{
	void *old_items;
	size_t old_cap, new_cap, total, i;
	uint8_t *mem;
	hm_header_t *hdr;
	void *new_items;

	old_items = *items_ptr;
	old_cap = *cap_ptr;
	new_cap = old_cap ? old_cap * 2 : MY_HASHTABLE_CAPACITY;
	total = sizeof(hm_header_t) + new_cap * 2 + new_cap * item_size;
	mem = (uint8_t *)alloc(allocator, total);
	memset(mem, 0, total);
	hdr = (hm_header_t *)mem;
	hdr->item_size = item_size;
	hdr->key_offset = HM_KEY_OFFSET;
	new_items = (void *)(mem + sizeof(hm_header_t) + new_cap * 2);
	*items_ptr = new_items;
	*cap_ptr = new_cap;
	*len_ptr = 0;

	for (i = 0; i < old_cap; i++) {
		if (_HM_OCC(old_items, old_cap, i)) {
			_hm_put_impl(allocator, items_ptr, len_ptr, cap_ptr,
			             item_size,
			             (char *)old_items + i * item_size);
		}
	}

	_hm_free(allocator, old_items, old_cap, item_size);
}

MY_HASHTABLE_DEF void _hm_put_impl(allocator_t allocator, void **items_ptr,
                                    size_t *len_ptr, size_t *cap_ptr,
                                    size_t item_size, void *item)
{
	void *items;
	size_t cap, h, pos, dist, i;
	void *buf;

	if (*items_ptr == NULL) {
		_hm_create(allocator, items_ptr, cap_ptr, item_size);
	}

	if (*len_ptr * HM_LOAD_FACTOR_DEN >= *cap_ptr * HM_LOAD_FACTOR_NUM) {
		_hm_grow(allocator, items_ptr, len_ptr, cap_ptr, item_size);
	}

	items = *items_ptr;
	cap = *cap_ptr;
	h = HM_HASH(item);
	pos = h % cap;
	dist = 0;

	buf = alloc(allocator, item_size);
	memcpy(buf, item, item_size);

	while (1) {
		if (!_HM_OCC(items, cap, pos)) {
			memcpy((char *)items + pos * item_size, buf, item_size);
			_HM_OCC(items, cap, pos) = 1;
			_HM_DIST(items, cap, pos) = (uint8_t)dist;
			(*len_ptr)++;
			goto done;
		}

		if (HM_EQ((char *)items + pos * item_size, buf)) {
			memcpy((char *)items + pos * item_size, buf, item_size);
			goto done;
		}

		if (_HM_DIST(items, cap, pos) < dist) {
			uint8_t tmp_dist;
			char *slot;

			tmp_dist = _HM_DIST(items, cap, pos);
			_HM_DIST(items, cap, pos) = (uint8_t)dist;
			dist = tmp_dist;

			slot = (char *)items + pos * item_size;
			for (i = 0; i < item_size; i++) {
				char t = ((char *)buf)[i];
				((char *)buf)[i] = slot[i];
				slot[i] = t;
			}
		}

		pos = (pos + 1) % cap;
		dist++;
	}

done:
	xdestroy(allocator, item_size, buf);
}

MY_HASHTABLE_DEF void *_hm_get_impl(void *items, size_t len, size_t cap,
                                     size_t item_size, void *key_item)
{
	size_t h, pos, dist;

	(void)len;
	if (!items || !cap) return NULL;

	h = HM_HASH(key_item);
	pos = h % cap;
	dist = 0;

	while (1) {
		if (!_HM_OCC(items, cap, pos)) return NULL;
		if (_HM_DIST(items, cap, pos) < dist) return NULL;

		if (HM_EQ((char *)items + pos * item_size, key_item)) {
			return (char *)items + pos * item_size;
		}

		pos = (pos + 1) % cap;
		dist++;
	}
}

MY_HASHTABLE_DEF void _hm_del_impl(void *items, size_t *len_ptr,
                                    size_t cap, size_t item_size,
                                    void *key_item)
{
	size_t h, pos, dist, cur, next;

	if (!items || !cap) return;

	h = HM_HASH(key_item);
	pos = h % cap;
	dist = 0;

	while (1) {
		if (!_HM_OCC(items, cap, pos)) return;
		if (_HM_DIST(items, cap, pos) < dist) return;

		if (HM_EQ((char *)items + pos * item_size, key_item)) {
			break;
		}

		pos = (pos + 1) % cap;
		dist++;
	}

	_HM_OCC(items, cap, pos) = 0;
	_HM_DIST(items, cap, pos) = 0;

	cur = pos;
	next = (cur + 1) % cap;

	while (_HM_OCC(items, cap, next) && _HM_DIST(items, cap, next) > 0) {
		memcpy((char *)items + cur * item_size,
		       (char *)items + next * item_size,
		       item_size);
		_HM_OCC(items, cap, cur) = 1;
		_HM_DIST(items, cap, cur) = _HM_DIST(items, cap, next) - 1;

		_HM_OCC(items, cap, next) = 0;
		_HM_DIST(items, cap, next) = 0;

		cur = next;
		next = (next + 1) % cap;
	}

	(*len_ptr)--;
}

#endif /* MY_HASHTABLE_IMPL */

#endif /* MY_HASHTABLE_H_ */
