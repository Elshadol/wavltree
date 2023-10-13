#include "wavltree.h"

#define __wavl_parity(pp) ((pp)&1)
#define _wavl_parity(x) __wavl_parity((x)->__wavl_parent_parity)
#define wavl_parity(x) ((x) ? _wavl_parity((x)) : 1)
#define __wavl_parent(pp) ((struct wavl_node*)(pp & ~3))
#define wavl_parent(x) __wavl_parent((x)->__wavl_parent_parity)

static inline void wavl_set_parent(struct wavl_node *x, struct wavl_node *p)
{
    x->__wavl_parent_parity = _wavl_parity(x) | (unsigned long)p;
}

static inline void __wavl_flip_parity(struct wavl_node *x)
{
    x->__wavl_parent_parity ^= 1;
}

static inline void __wavl_promote_rank(struct wavl_node *x)
{
    __wavl_flip_parity(x);
}

static inline void __wavl_demote_rank(struct wavl_node *x)
{
    __wavl_flip_parity(x);
}

static inline void __wavl_change_child(struct wavl_node* x,
                                       struct wavl_node* y,
                                       struct wavl_node* x_parent,
                                       struct wavl_root* root)
{
    if (x_parent) {
        if (x_parent->wavl_left == x)
            x_parent->wavl_left = y;
        else
            x_parent->wavl_right = y;
    } else
        root->wavl_node = y;
}

static inline void __wavl_set_parent_parity(struct wavl_node *x,
        struct wavl_node *parent, int parity)
{
    x->__wavl_parent_parity = (unsigned long)parent | parity;
}

static inline void __wavl_rotate_set_parents_parity(struct wavl_node* x,
        struct wavl_node* y, int parity, struct wavl_root* root)
{
    struct wavl_node* x_parent = wavl_parent(x);
    y->__wavl_parent_parity = x->__wavl_parent_parity;
    __wavl_set_parent_parity(x, y, parity);
    __wavl_change_child(x, y, x_parent, root);
}

void wavl_insert_fixup(struct wavl_node* x, struct wavl_root* root)
{
    int x_parity, xp_parity;
    struct wavl_node *x_parent = wavl_parent(x), *tmp, *tmp1;
    while (x_parent) {
        x_parity = _wavl_parity(x);
        xp_parity = _wavl_parity(x_parent);
        // parent from 2-2 to 2-1 or from 1-2 to 1-1, we're done
        if (x_parity != xp_parity)
            break;
        tmp = x_parent->wavl_right;
        if (x != tmp) {
            // parent from 1-1 to 0-1, promote rank of parent, then climb up
            if (xp_parity != wavl_parity(tmp)) {
                __wavl_promote_rank(x_parent);
                x = x_parent;
                x_parent = wavl_parent(x);
                continue;
            }
            // parent from 1-2 to 0-2, single rot or double rot, parent always demote
            tmp = x->wavl_right;
            // x is 2-1, perform a double rot
            if (x_parity != wavl_parity(tmp)) {
                x->wavl_right = tmp1 = tmp->wavl_left;
                tmp->wavl_left = x;
                if (tmp1)
                    wavl_set_parent(tmp1, x);
                __wavl_set_parent_parity(x, tmp, x_parity ^ 1);
                x = tmp;
            }
            // otherwise, x is 1-2, perform a single rot
            x_parent->wavl_left = tmp1 = x->wavl_right;
            x->wavl_right = x_parent;
            if (tmp1)
                wavl_set_parent(tmp1, x_parent);
            __wavl_rotate_set_parents_parity(x_parent, x, xp_parity ^ 1, root);
            break;
        } else {
            tmp = x_parent->wavl_left;
            if (xp_parity != wavl_parity(tmp)) {
                __wavl_promote_rank(x_parent);
                x = x_parent;
                x_parent = wavl_parent(x);
                continue;
            }
            tmp = x->wavl_left;
            if (x_parity != wavl_parity(tmp)) {
                x->wavl_left = tmp1 = tmp->wavl_right;
                tmp->wavl_right = x;
                if (tmp1)
                    wavl_set_parent(tmp1, x);
                __wavl_set_parent_parity(x, tmp, x_parity ^ 1);
                x = tmp;
            }
            x_parent->wavl_right = tmp1 = x->wavl_left;
            x->wavl_left = x_parent;
            if (tmp1)
                wavl_set_parent(tmp1, x_parent);
            __wavl_rotate_set_parents_parity(x_parent, x, xp_parity ^ 1, root);
            break;
        }
    }
}

static void __wavl_erase_fixup(struct wavl_node* x,
                               struct wavl_node* x_parent,
                               struct wavl_root* root)
{
    struct wavl_node *y, *tmp1, *tmp2;
    int xp_parity, y_parity, p1, p2;
    if (!x_parent->wavl_left && !x_parent->wavl_right) {
        // parent now is a 2-2 leaf, demote and retry from it
        __wavl_flip_parity(x_parent);
        x = x_parent;
        x_parent = wavl_parent(x_parent);
        if (!x_parent)
            return;
    }
    xp_parity = _wavl_parity(x_parent);
    // parent from a binay to unary, no need to climb up
    if (wavl_parity(x) == xp_parity)
        return;
    do {
        // now x is a 3-child, and its sibling is always non-null
        y = x_parent->wavl_right;
        if (x != y) {
            y_parity = _wavl_parity(y);
            if (y_parity == xp_parity) {
                // parent is 3-2, demote and retry from it
                __wavl_demote_rank(x_parent);
                x = x_parent;
                x_parent = wavl_parent(x_parent);
                continue;
            }
            tmp1 = y->wavl_left;
            tmp2 = y->wavl_right;
            p1 = wavl_parity(tmp1);
            p2 = wavl_parity(tmp2);
            if (y_parity == p2) {
                if (y_parity == p1) {
                    // parent is 3-1, sibling is 2-2, demote both parent and sibling,
                    // then retry from parent
                    __wavl_demote_rank(y);
                    __wavl_demote_rank(x_parent);
                    x = x_parent;
                    x_parent = wavl_parent(x_parent);
                    continue;
                }
                // sibling is 1-2, perform a double rot, demote y
                y->wavl_left = tmp2 = tmp1->wavl_right;
                tmp1->wavl_right = y;
                if (tmp2)
                    wavl_set_parent(tmp2, y);
                __wavl_set_parent_parity(y, tmp1, y_parity ^ 1);
                y = tmp1;
            } else {
                // y is 1-1, or 2-1, perform a single rot
                // note: after a rot, if parent become a leaf, demote it twice
                // otherwise demote parent once
                if (x_parent->wavl_left || tmp1)   
                    xp_parity ^= 1;
            }
            x_parent->wavl_right = tmp2 = y->wavl_left;
            y->wavl_left = x_parent;
            if (tmp2)
                wavl_set_parent(tmp2, x_parent);
            __wavl_rotate_set_parents_parity(x_parent, y, xp_parity, root);
            break;
        } else {
            y = x_parent->wavl_left;
            y_parity = _wavl_parity(y);
            if (y_parity == xp_parity) {
                // parent is 3-2, demote and retry from it
                __wavl_demote_rank(x_parent);
                x = x_parent;
                x_parent = wavl_parent(x_parent);
                continue;
            }
            tmp1 = y->wavl_left;
            tmp2 = y->wavl_right;
            p1 = wavl_parity(tmp1);
            p2 = wavl_parity(tmp2);
            if (y_parity == p1) {
                if (y_parity == p2) {
                    // parent is 3-1, sibling is 2-2, demote both parent and sibling,
                    // then retry from parent
                    __wavl_demote_rank(y);
                    __wavl_demote_rank(x_parent);
                    x = x_parent;
                    x_parent = wavl_parent(x_parent);
                    continue;
                }
                // y is 2-1, perform a double rotation
                y->wavl_right = tmp1 = tmp2->wavl_left;
                tmp2->wavl_left = y;
                if (tmp1)
                    wavl_set_parent(tmp1, y);
                __wavl_set_parent_parity(y, tmp2, y_parity ^ 1);
                y = tmp2;
            } else {
                if (x_parent->wavl_right || tmp2)
                    xp_parity ^= 1;
            }
            x_parent->wavl_left = tmp2 = y->wavl_right;
            y->wavl_right = x_parent;
            if (tmp2)
                wavl_set_parent(tmp2, x_parent);
            __wavl_rotate_set_parents_parity(x_parent, y, xp_parity, root);
            break;
        }
        // loop invariant: x is not root, and x is a 3-child
    } while (x_parent && (xp_parity = _wavl_parity(x_parent)) != _wavl_parity(x));
}

void wavl_erase(struct wavl_node* node, struct wavl_root* root)
{
    struct wavl_node *child = node->wavl_left, *tmp = node->wavl_right, *parent;
    unsigned long pp = node->__wavl_parent_parity;
    if (!child || !tmp) {
        tmp = child = (!tmp ? child : tmp);
        pp = (pp & ~3);
        parent = (struct wavl_node*)pp;
    } else {
        parent = tmp;
        while (tmp->wavl_left != NULL)
            tmp = tmp->wavl_left;
        tmp->wavl_left = child;
        wavl_set_parent(child, tmp);
        child = tmp->wavl_right;
        if (parent != tmp) {
            wavl_set_parent(parent, tmp);
            tmp->wavl_right = parent;
            parent = wavl_parent(tmp);
            parent->wavl_left = child;
        }
        tmp->__wavl_parent_parity = pp;
        pp = (pp & ~3);
    }
    __wavl_change_child(node, tmp, (struct wavl_node*)pp, root);
    if (child)
        wavl_set_parent(child, parent);
    if (parent)
        __wavl_erase_fixup(child, parent, root);
}

void wavl_replace_node(struct wavl_node* victim,
                       struct wavl_node* newnode,
                       struct wavl_root* root)
{
    struct wavl_node* parent = wavl_parent(victim);
    __wavl_change_child(victim, newnode, parent, root);

    if (victim->wavl_left)
        wavl_set_parent(victim->wavl_left, newnode);

    if (victim->wavl_right)
        wavl_set_parent(victim->wavl_right, newnode);

    *newnode = *victim;
}

struct wavl_node* wavl_first(const struct wavl_root* root)
{
    struct wavl_node* n;

    n = root->wavl_node;
    if (!n)
        return NULL;
    while (n->wavl_left)
        n = n->wavl_left;
    return n;
}

struct wavl_node* wavl_last(const struct wavl_root* root)
{
    struct wavl_node* n;

    n = root->wavl_node;
    if (!n)
        return NULL;
    while (n->wavl_right)
        n = n->wavl_right;
    return n;
}

struct wavl_node* wavl_next(const struct wavl_node* node)
{
    struct wavl_node* parent;

    if (node == NULL)
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

struct wavl_node* wavl_prev(const struct wavl_node* node)
{
    struct wavl_node* parent;

    if (node == NULL)
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
