#ifndef	WAVLTREE_INCLUDED
#define	WAVLTREE_INCLUDED

#include <stddef.h>

struct wavl_node {
    unsigned long __wavl_parent_parity;
    // parity of rank, if rank is even, parity is 0, otherwise parity is 1
    struct wavl_node *wavl_left;
    struct wavl_node *wavl_right;
} __attribute__((aligned(sizeof(long))));

struct wavl_root {
    struct wavl_node *wavl_node;
};

#define WAVL_ROOT (struct wavl_root) { NULL, }

#define WAVL_EMPTY_NODE(node)  \
	((node)->__wavl_parent_parity == (unsigned long)(node))

#define WAVL_CLEAR_NODE(node)  \
	((node)->__wavl_parent_parity = (unsigned long)(node))

#define	wavl_entry(ptr, type, member) __extension__ ({ \
	const __typeof__(((type *)0)->member) *__pmember = (ptr); \
	(type *)((char *)__pmember - offsetof(type, member)); })

#define wavl_entry_safe(ptr, type, member) __extension__ ({ \
        const typeof(ptr) ___ptr = (ptr); \
        ___ptr ? wavl_entry(___ptr, type, member) : NULL; })

void wavl_insert_fixup(struct wavl_node *, struct wavl_root *);
void wavl_erase(struct wavl_node *, struct wavl_root *);

struct wavl_node *wavl_next(const struct wavl_node *);
struct wavl_node *wavl_prev(const struct wavl_node *);
struct wavl_node *wavl_first(const struct wavl_root *);
struct wavl_node *wavl_last(const struct wavl_root *);

void wavl_replace_node(struct wavl_node *victim, struct wavl_node *newnode,
                      struct wavl_root *root);

static inline void wavl_link_node(struct wavl_node *node, struct wavl_node *parent,
                                 struct wavl_node **wavl_link)
{
    node->__wavl_parent_parity = (unsigned long)parent;
    node->wavl_left = node->wavl_right = NULL;
    *wavl_link = node;
}

#endif	/* wavlTREE_INCLUDED */
