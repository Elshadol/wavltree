#include "wavltree.h"

#define __wavl_parity(pp) ((pp) & 1)
#define _wavl_parity(x)  __wavl_parity((x)->__wavl_parent_parity)
#define wavl_parity(x) ((x) ? _wavl_parity((x)) : 1)
#define __wavl_flip_parity(x) do { (x)->__wavl_parent_parity ^= 1; } while (0)
#define __wavl_parent(pp) ((struct wavl_node *)(pp & ~3))
#define wavl_parent(x) __wavl_parent((x)->__wavl_parent_parity)
#define wavl_set_parent(n, p) do {\
    (n)->__wavl_parent_parity = _wavl_parity(n) | (unsigned long)p; \
} while (0)

struct wavl_node *wavl_first(const struct wavl_root *root)
{
    struct wavl_node *n;

    n = root->wavl_node;
    if (!n)
        return NULL;
    while (n->wavl_left)
        n = n->wavl_left;
    return n;
}

struct wavl_node *wavl_last(const struct wavl_root *root)
{
    struct wavl_node *n;

    n = root->wavl_node;
    if (!n)
        return NULL;
    while (n->wavl_right)
        n = n->wavl_right;
    return n;
}

struct wavl_node *wavl_next(const struct wavl_node *node)
{
    struct wavl_node *parent;

    if (node == NULL)
        return NULL;

    if (node->wavl_right) {
        node = node->wavl_right;
        while (node->wavl_left)
            node = node->wavl_left;
        return (struct wavl_node *)node;
    }

    while ((parent = wavl_parent(node)) && node == parent->wavl_right)
        node = parent;

    return parent;
}

struct wavl_node *wavl_prev(const struct wavl_node *node)
{
    struct wavl_node *parent;

    if (node == NULL)
        return NULL;

    if (node->wavl_left) {
        node = node->wavl_left;
        while (node->wavl_right)
            node = node->wavl_right;
        return (struct wavl_node *)node;
    }

    while ((parent = wavl_parent(node)) && node == parent->wavl_left)
        node = parent;

    return parent;
}

static inline void __wavl_change_child(struct wavl_node *x,
                                       struct wavl_node *y,
                                       struct wavl_node *x_parent,
                                       struct wavl_root *root)
{
    if (x_parent) {
        if (x_parent->wavl_left == x)
            x_parent->wavl_left = y;
        else
            x_parent->wavl_right = y;
    } else
        root->wavl_node = y;
}

void wavl_replace_node(struct wavl_node *victim, struct wavl_node *newnode,
                       struct wavl_root *root)
{
    struct wavl_node *parent = wavl_parent(victim);
    __wavl_change_child(victim, newnode, parent, root);

    if (victim->wavl_left)
        wavl_set_parent(victim->wavl_left, newnode);

    if (victim->wavl_right)
        wavl_set_parent(victim->wavl_right, newnode);

    *newnode = *victim;
}

static inline void __wavl_rotate_set_parents(struct wavl_node *x,
        struct wavl_node *y, struct wavl_root *root)
{
    struct wavl_node *x_parent = wavl_parent(x);
    wavl_set_parent(x, y);
    wavl_set_parent(y, x_parent);
    __wavl_change_child(x, y, x_parent, root);
}

static void __wavl_rotate_left(struct wavl_node *x, struct wavl_root *root)
{
    struct wavl_node *y = x->wavl_right, *tmp;
    x->wavl_right = tmp = y->wavl_left;
    y->wavl_left = x;
    if (tmp)
        wavl_set_parent(tmp, x);
    __wavl_rotate_set_parents(x, y, root);
}

static void __wavl_rotate_right(struct wavl_node *x, struct wavl_root *root)
{
    struct wavl_node *y = x->wavl_left, *tmp;
    x->wavl_left = tmp = y->wavl_right;
    y->wavl_right = x;
    if (tmp)
        wavl_set_parent(tmp, x);
    __wavl_rotate_set_parents(x, y, root);
}

static void __wavl_rotate_left_right(struct wavl_node *x, struct wavl_root *root)
{
    struct wavl_node *y = x->wavl_left, *tmp;
    struct wavl_node *z = y->wavl_right;
    x->wavl_left = tmp = z->wavl_right;
    z->wavl_right = x;
    if (tmp)
        wavl_set_parent(tmp, x);
    y->wavl_right = tmp = z->wavl_left;
    z->wavl_left = y;
    if (tmp)
        wavl_set_parent(tmp, y);
    wavl_set_parent(y, z);
    __wavl_rotate_set_parents(x, z, root);
}

static void __wavl_rotate_right_left(struct wavl_node *x, struct wavl_root *root)
{
    struct wavl_node *y = x->wavl_right, *tmp;
    struct wavl_node *z = y->wavl_left;
    x->wavl_right = tmp = z->wavl_left;
    z->wavl_left = x;
    if (tmp)
        wavl_set_parent(tmp, x);
    y->wavl_left = tmp = z->wavl_right;
    z->wavl_right = y;
    if (tmp)
        wavl_set_parent(tmp, y);
    wavl_set_parent(y, z);
    __wavl_rotate_set_parents(x, z, root);
}

void wavl_insert_fixup(struct wavl_node *x, struct wavl_root *root)
{
    int x_parity, xp_parity;
    struct wavl_node *x_parent = wavl_parent(x), *tmp;
    if (!x_parent || (x_parent->wavl_left && x_parent->wavl_right))
        return;
    // loop invariant: x'rank increase, and not root
    for (;;) {
        x = x_parent;
        __wavl_flip_parity(x);
        x_parent = wavl_parent(x);
        if (!x_parent)
            break;
        x_parity = _wavl_parity(x);
        xp_parity = _wavl_parity(x_parent);
        // parent from (2, 2) to (2, 1), or from (1, 2) to (1, 1), we're done
        if (x_parity != xp_parity)
            break;
        tmp = x_parent->wavl_right;
        if (tmp != x) {
            // parent from (1, 1) to (2, 1), need to climb up
            if (xp_parity != wavl_parity(tmp)) 
                continue;
            // parent now is (0, 2)
            tmp = x->wavl_right;
            __wavl_flip_parity(x_parent);
            if (x_parity == wavl_parity(tmp)) 
                // x is (1, 2)
                __wavl_rotate_right(x_parent, root);
            else {
                // x is (2, 1)
                __wavl_flip_parity(tmp);
                __wavl_flip_parity(x);
                __wavl_rotate_left_right(x_parent, root);
            }
        } else {
            tmp = x_parent->wavl_left;
            if (xp_parity != wavl_parity(tmp)) 
                continue;
            tmp = x->wavl_left;
            __wavl_flip_parity(x_parent);
            if (x_parity == wavl_parity(tmp)) 
                __wavl_rotate_left(x_parent, root);
            else {
                __wavl_flip_parity(tmp);
                __wavl_flip_parity(x);
                __wavl_rotate_right_left(x_parent, root);
            }
        }
        break;
    }
}

static void __wavl_erase_fixup(struct wavl_node *x,
                               struct wavl_node *x_parent,
                               struct wavl_root *root)
{
    struct wavl_node *y;
    int xp_parity, y_parity, yl_parity, yr_parity;
    if (!x_parent->wavl_left && !x_parent->wavl_right) {
        // if parent is a 2-2 leaf, demote and retry from it
        __wavl_flip_parity(x_parent);
        x = x_parent;
        x_parent = wavl_parent(x_parent);
        if (!x_parent)
            return;
    }
    // parent used to be (1, 1), now is (1, 2), didnot produce a 3-child
    xp_parity = _wavl_parity(x_parent);
    if (wavl_parity(x) == xp_parity)
       return;
    // now, x is a 3-child
    for (;;) {
        y = x_parent->wavl_right;
        if (x != y) {
            y_parity = _wavl_parity(y);
            // x_parent is (3, 2), demote x_parent and climb up
            if (y_parity == xp_parity)
                __wavl_flip_parity(x_parent);
            else {
                yl_parity = wavl_parity(y->wavl_left);
                yr_parity = wavl_parity(y->wavl_right);
                // x_parent is (3, 1),  y is (2, 2), demote both x_parent and y, then climb up
                __wavl_flip_parity(y);
                if ((y_parity == yl_parity) && (y_parity == yr_parity))
                    __wavl_flip_parity(x_parent);
                else {
                    // perform a single or double rotation, then we're done
                    // x_parent is (1, 3), y is (1, 2), (2, 1) or (1, 1)
                    if (y_parity != yr_parity) {
                        // y is (2, 1) or (1, 1)
                        __wavl_rotate_left(x_parent, root);
                        if (x_parent->wavl_left || x_parent->wavl_right)
                            // if parent become a leaf, demote it again, but double demote do nothing,
                            __wavl_flip_parity(x_parent);
                    } else
                        // y is (1, 2)
                        __wavl_rotate_right_left(x_parent, root);
                    break;
                }
            }
        } else {
            y = x_parent->wavl_left;
            y_parity = _wavl_parity(y);
            if (y_parity == xp_parity)
                __wavl_flip_parity(x_parent);
            else {
                yl_parity = wavl_parity(y->wavl_right);
                yr_parity = wavl_parity(y->wavl_left);
                __wavl_flip_parity(y);
                if ((y_parity == yl_parity) && (y_parity == yr_parity))
                    __wavl_flip_parity(x_parent);
                else {
                    if (y_parity != yr_parity) {
                        __wavl_rotate_right(x_parent, root);
                        if (x_parent->wavl_right || x_parent->wavl_left)
                            __wavl_flip_parity(x_parent);
                    } else
                        __wavl_rotate_left_right(x_parent, root);
                    break;
                }
            }
        }
        // loop invariant: x is a 3-child and not root
        x = x_parent;
        x_parent = wavl_parent(x_parent);
        if (!x_parent)
            break;
        xp_parity = _wavl_parity(x_parent);
        if (xp_parity == _wavl_parity(x))
            break;
    }
}

void wavl_erase(struct wavl_node *node, struct wavl_root *root)
{
    struct wavl_node *child = node->wavl_left,
                          *tmp = node->wavl_right,
                           *parent;
    unsigned long pp = node->__wavl_parent_parity;
    if (!child || !tmp) {
        tmp = child = (!tmp ? child : tmp);
        pp = (pp & ~3);
        parent = (struct wavl_node *)pp;
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
    __wavl_change_child(node, tmp, (struct wavl_node *)pp, root);
    if (child)
        wavl_set_parent(child, parent);
    if (parent)
        __wavl_erase_fixup(child, parent, root);
}
