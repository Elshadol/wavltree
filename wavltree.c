#include "wavltree.h"

static inline unsigned long _wavl_parity(const struct wavl_node *node)
{
    return (node->__wavl_parent_parity & 1lu);
}

static inline unsigned long wavl_parity(const struct wavl_node *node)
{
    return (node ? _wavl_parity(node) : 1lu);
}

static inline struct wavl_node *wavl_even_parent(const struct wavl_node *node)
{
    return (struct wavl_node*)node->__wavl_parent_parity;
}

static inline struct wavl_node *wavl_parent(const struct wavl_node *node)
{
    return (struct wavl_node*)(node->__wavl_parent_parity & ~3lu);
}

static inline void
__wavl_set_parent_parity(struct wavl_node *node, struct wavl_node *parent,
                         unsigned long parity)
{
    node->__wavl_parent_parity = (unsigned long)parent + parity;
}

static inline void
__wavl_set_parent(struct wavl_node *node, struct wavl_node *parent)
{
    __wavl_set_parent_parity(node, parent, _wavl_parity(node));
}

static inline void
__wavl_change_child(struct wavl_node *old, struct wavl_node *new,
                    struct wavl_node *parent, struct wavl_root *root)
{
    if (parent) {
        if (parent->wavl_left == old)
            parent->wavl_left = new;
        else
            parent->wavl_right = new;
    } else
        root->wavl_node = new;
}

static inline void
__wavl_rotate_set_parents(struct wavl_node *old, struct wavl_node *new,
                          struct wavl_root *root, unsigned long parity)
{
    struct wavl_node *parent = wavl_parent(old);
    new->__wavl_parent_parity = old->__wavl_parent_parity;
    __wavl_set_parent_parity(old, new, parity);
    __wavl_change_child(old, new, parent, root);
}

void wavl_insert_fixup(struct wavl_node *node, struct wavl_root *root)
{
    unsigned long parity = _wavl_parity(node);
    struct wavl_node *parent = wavl_even_parent(node), *tmp, *tmp1;

    while (parent) {
        parity = parity ^ 1lu;
        if (parity == _wavl_parity(parent))
            break;

        tmp = parent->wavl_right;
        if (node != tmp) {
            if (parity == wavl_parity(tmp)) {
                tmp1 = node;
                node = parent;
                parent = wavl_parent(node);
                __wavl_set_parent_parity(node, parent, parity);
                continue;
            }
            tmp = node->wavl_right;
            if (tmp == tmp1) {
                node->wavl_right = tmp1 = tmp->wavl_left;
                tmp->wavl_left = node;
                if (tmp1)
                    __wavl_set_parent(tmp1, node);
                __wavl_set_parent_parity(node, tmp, parity);
                node = tmp;
                tmp = node->wavl_right;
            }
            node->wavl_right = parent;
            parent->wavl_left = tmp;
            if (tmp)
                __wavl_set_parent(tmp, parent);
            __wavl_rotate_set_parents(parent, node, root, parity);
            break;
        } else {
            tmp = parent->wavl_left;
            if (parity == wavl_parity(tmp)) {
                tmp1 = node;
                node = parent;
                parent = wavl_parent(node);
                __wavl_set_parent_parity(node, parent, parity);
                continue;
            }
            tmp = node->wavl_left;
            if (tmp == tmp1) {
                node->wavl_left = tmp1 = tmp->wavl_right;
                tmp->wavl_right = node;
                if (tmp1)
                    __wavl_set_parent(tmp1, node);
                __wavl_set_parent_parity(node, tmp, parity);
                node = tmp;
                tmp = node->wavl_left;
            }
            node->wavl_left = parent;
            parent->wavl_right = tmp;
            if (tmp)
                __wavl_set_parent(tmp, parent);
            __wavl_rotate_set_parents(parent, node, root, parity);
            break;
        }
    }
}

static inline void
__wavl_erase_fixup(struct wavl_node *node, struct wavl_node *parent,
                   struct wavl_root *root)
{
    unsigned long parity1 = wavl_parity(node), parity2;
    struct wavl_node *sibling, *tmp1, *tmp2;

    if (!parent->wavl_left && !parent->wavl_right) {
        node = parent;
        parent = wavl_parent(node);
        parity1 = parity1 ^ 1lu;
        __wavl_set_parent_parity(node, parent, parity1);
    }

    while (parent) {
        parity2 = _wavl_parity(parent);
        if (parity1 == parity2)
            break;

        sibling = parent->wavl_right;
        if (node != sibling) {
            if (parity1 != _wavl_parity(sibling)) {
                node = parent;
                parent = wavl_parent(node);
                __wavl_set_parent_parity(node, parent, parity1);
                continue;
            }
            tmp1 = sibling->wavl_right;
            tmp2 = sibling->wavl_left;
            if (parity2 != wavl_parity(tmp1)) {
                if (parity2 != _wavl_parity(tmp2)) {
                    __wavl_set_parent_parity(sibling, parent, parity2);
                    node = parent;
                    parent = wavl_parent(node);
                    __wavl_set_parent_parity(node, parent, parity1);
                    continue;
                }
                sibling->wavl_left = tmp1 = tmp2->wavl_right;
                tmp2->wavl_right = sibling;
                if (tmp1)
                    __wavl_set_parent(tmp1, sibling);
                __wavl_set_parent_parity(sibling, tmp2, parity2);
                sibling = tmp2;
                tmp2 = sibling->wavl_left;
            } else {
                if (node != tmp2)
                    parity2 = parity1;
            }
            sibling->wavl_left = parent;
            parent->wavl_right = tmp2;
            if (tmp2)
                __wavl_set_parent(tmp2, parent);
            __wavl_rotate_set_parents(parent, sibling, root, parity2);
            break;
        } else {
            sibling = parent->wavl_left;
            if (parity1 != _wavl_parity(sibling)) {
                node = parent;
                parent = wavl_parent(node);
                __wavl_set_parent_parity(node, parent, parity1);
                continue;
            }
            tmp1 = sibling->wavl_left;
            tmp2 = sibling->wavl_right;
            if (parity2 != wavl_parity(tmp1)) {
                if (parity2 != _wavl_parity(tmp2)) {
                    __wavl_set_parent_parity(sibling, parent, parity2);
                    node = parent;
                    parent = wavl_parent(node);
                    __wavl_set_parent_parity(node, parent, parity1);
                    continue;
                }
                sibling->wavl_right = tmp1 = tmp2->wavl_left;
                tmp2->wavl_left = sibling;
                if (tmp1)
                    __wavl_set_parent(tmp1, sibling);
                __wavl_set_parent_parity(sibling, tmp2, parity2);
                sibling = tmp2;
                tmp2 = sibling->wavl_right;
            } else {
                if (node != tmp2)
                    parity2 = parity1;
            }
            sibling->wavl_right = parent;
            parent->wavl_left = tmp2;
            if (tmp2)
                __wavl_set_parent(tmp2, parent);
            __wavl_rotate_set_parents(parent, sibling, root, parity2);
            break;
        }
    }
}

void wavl_erase(struct wavl_node *node, struct wavl_root *root)
{
    struct wavl_node *child = node->wavl_left, *tmp = node->wavl_right;
    struct wavl_node *parent, *parent1 = wavl_parent(node);
    if (!child || !tmp) {
        tmp = child = (!tmp ? child : tmp);
        parent = parent1;
    } else {
        parent = tmp;
        while (tmp->wavl_left)
            tmp = tmp->wavl_left;
        tmp->wavl_left = child;
        __wavl_set_parent(child, tmp);
        child = tmp->wavl_right;
        if (parent != tmp) {
            __wavl_set_parent(parent, tmp);
            tmp->wavl_right = parent;
            parent = wavl_parent(tmp);
            parent->wavl_left = child;
        }
        tmp->__wavl_parent_parity = node->__wavl_parent_parity;
    }
    __wavl_change_child(node, tmp, parent1, root);
    if (child)
        __wavl_set_parent(child, parent);
    if (parent)
        __wavl_erase_fixup(child, parent, root);
}

void wavl_replace_node(struct wavl_node *victim, struct wavl_node *new,
                       struct wavl_root *root)
{
    struct wavl_node* parent = wavl_parent(victim);

    __wavl_change_child(victim, new, parent, root);
    if (victim->wavl_left)
        __wavl_set_parent(victim->wavl_left, new);
    if (victim->wavl_right)
        __wavl_set_parent(victim->wavl_right, new);

    *new = *victim;
}

struct wavl_node *wavl_first(const struct wavl_root *root)
{
    struct wavl_node *node = root->wavl_node;
    if (!node)
        return NULL;
    while (node->wavl_left)
        node = node->wavl_left;
    return node;
}

struct wavl_node *wavl_last(const struct wavl_root *root)
{
    struct wavl_node *node = root->wavl_node;
    if (!node)
        return NULL;
    while (node->wavl_right)
        node = node->wavl_right;
    return node;
}

struct wavl_node *wavl_next(const struct wavl_node *node)
{
    struct wavl_node *parent;

    if (WAVL_EMPTY_NODE(node))
        return NULL;

    if (node->wavl_right) {
        node = node->wavl_right;
        while (node->wavl_left)
            node = node->wavl_left;
        return (struct wavl_node*)node;
    }

    while ((parent = wavl_parent(node)) && node == parent->wavl_right)
        node = parent;

    return parent;
}

struct wavl_node *wavl_prev(const struct wavl_node *node)
{
    struct wavl_node *parent;

    if (WAVL_EMPTY_NODE(node))
        return NULL;

    if (node->wavl_left) {
        node = node->wavl_left;
        while (node->wavl_right)
            node = node->wavl_right;
        return (struct wavl_node*)node;
    }

    while ((parent = wavl_parent(node)) && node == parent->wavl_left)
        node = parent;

    return parent;
}
