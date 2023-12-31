#include "wavltree.h"

#define __wavl_parity(pp) ((pp) & 1lu)
#define _wavl_parity(x) __wavl_parity((x)->__wavl_parent_parity)
// in HST, external node(null) has rank -1lu, so its parity is 1lu
#define wavl_parity(x) ((x) ? _wavl_parity((x)) : 1lu)

#define __wavl_parent(pp) ((struct wavl_node*)(pp & ~3lu))
#define wavl_parent(x) __wavl_parent((x)->__wavl_parent_parity)

static inline void
__wavl_set_parent(struct wavl_node *node, struct wavl_node *parent)
{
    node->__wavl_parent_parity = _wavl_parity(node) | (unsigned long)parent;
}

static inline void
__wavl_set_parent_parity(struct wavl_node *node, struct wavl_node * parent,
                         unsigned long parity)
{
    node->__wavl_parent_parity = (unsigned long)parent | parity;
}

static inline void
__wavl_change_child(struct wavl_node* old, struct wavl_node* new,
                    struct wavl_node* parent, struct wavl_root* root)
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

void wavl_insert_fixup(struct wavl_node* node, struct wavl_root* root)
{
    unsigned long parity = _wavl_parity(node), p_parity;
    struct wavl_node *parent = wavl_parent(node), *tmp, *tmp1;
    while (parent && ((p_parity = _wavl_parity(parent)) == parity)) {
        tmp = parent->wavl_right;
        if (node != tmp) {
            // parent from 1lu-1lu to 0-1lu, promote parent once, and retry from parent
            if (p_parity != wavl_parity(tmp)) {
                node = parent;
                parent = wavl_parent(node);
                __wavl_set_parent_parity(node, parent, parity ^= 1lu);
                continue;
            }
            //parent now is 0-2, check whether node is 2-1lu
            tmp = node->wavl_right;
            if (parity != wavl_parity(tmp)) {
                // node is 2-1lu, need perform a double rotation
                // in a double rotation, demote node once, promote tmp once
                node->wavl_right = tmp1 = tmp->wavl_left;
                tmp->wavl_left = node;
                if (tmp1)
                    __wavl_set_parent(tmp1, node);
                __wavl_set_parent_parity(node, tmp, parity ^ 1lu);
                node = tmp;
                tmp = node->wavl_right;
            }
            // perform a single rotation, demote parent once
            parent->wavl_left = tmp;
            node->wavl_right = parent;
            if (tmp)
                __wavl_set_parent(tmp, parent);
            __wavl_rotate_set_parents(parent, node, root, p_parity ^ 1lu);
            // after a double rotation or single rotation,
            // all violations are restored
            break;
        } else {
            tmp = parent->wavl_left;
            if (p_parity != wavl_parity(tmp)) {
                node = parent;
                parent = wavl_parent(node);
                __wavl_set_parent_parity(node, parent, parity ^= 1lu);
                continue;
            }
            tmp = node->wavl_left;
            if (parity != wavl_parity(tmp)) {
                node->wavl_left = tmp1 = tmp->wavl_right;
                tmp->wavl_right = node;
                if (tmp1)
                    __wavl_set_parent(tmp1, node);
                __wavl_set_parent_parity(node, tmp, parity ^ 1lu);
                node = tmp;
                tmp = node->wavl_left;
            }
            parent->wavl_right = tmp;
            node->wavl_left = parent;
            if (tmp)
                __wavl_set_parent(tmp, parent);
            __wavl_rotate_set_parents(parent, node, root, p_parity ^ 1lu);
            break;
        }
    }
}

static void __wavl_erase_fixup(struct wavl_node* node, struct wavl_node* parent,
                               struct wavl_root* root)
{
    struct wavl_node *sibling, *tmp1, *tmp2;
    unsigned long p_parity, s_parity, p1, p2;
    if (!parent->wavl_left && !parent->wavl_right) {
        // parent now is a 2-2 leaf, demote parent once and retry from it
        p_parity = _wavl_parity(parent);
        node = parent;
        parent = wavl_parent(parent);
        __wavl_set_parent_parity(node, parent, p_parity ^ 1lu);
    }
    s_parity = wavl_parity(node);
    // s_parity stores rank parity of node whose rank demoted
    while (parent && (p_parity = _wavl_parity(parent)) != s_parity) {
        sibling = parent->wavl_right;
        if (node != sibling) {
            s_parity = _wavl_parity(sibling);
            if (s_parity == p_parity) {
                // parent is 3-2, demote parent once and retry from it
                node = parent;
                parent = wavl_parent(parent);
                // now s_parity has the same value as node's rank parity
                __wavl_set_parent_parity(node, parent, s_parity ^= 1lu);
                continue;
            }
            tmp1 = sibling->wavl_right;
            tmp2 = sibling->wavl_left;
            p1 = wavl_parity(tmp1);
            if (s_parity == p1) {
                p2 = wavl_parity(tmp2);
                if (s_parity == p2) {
                    // parent is 3-1lu, sibling is 2-2, demote both parent and sibling once,
                    // then retry from parent
                    // now s_parity has the same value as node's rank parity
                    __wavl_set_parent_parity(sibling, parent, s_parity ^ 1lu);
                    node = parent;
                    parent = wavl_parent(node);
                    __wavl_set_parent_parity(node, parent, p_parity ^ 1lu);
                    continue;
                }
                // sibling is 1lu-2, perform a double rot, demote sibling once,
                // demote parent twice, promote tmp2 twice,
                sibling->wavl_left = tmp1 = tmp2->wavl_right;
                tmp2->wavl_right = sibling;
                if (tmp1)
                    __wavl_set_parent(tmp1, sibling);
                __wavl_set_parent_parity(sibling, tmp2, s_parity ^ 1lu);
                sibling = tmp2;
                tmp2 = sibling->wavl_left;
            } else {
                // sibling is 1lu-1lu, or 2-1lu, perform a single rot
                // note: after a rot, if parent doesn't become a leaf, demote it once,
                // otherwise demote parent twicei
                if (node || tmp2)
                    p_parity ^= 1lu;
            }
            parent->wavl_right = tmp2;
            sibling->wavl_left = parent;
            if (tmp2)
                __wavl_set_parent(tmp2, parent);
            __wavl_rotate_set_parents(parent, sibling, root, p_parity);
            break;
        } else {
            sibling = parent->wavl_left;
            s_parity = _wavl_parity(sibling);
            if (s_parity == p_parity) {
                node = parent;
                parent = wavl_parent(parent);
                __wavl_set_parent_parity(node, parent, s_parity ^= 1lu);
                continue;
            }
            tmp1 = sibling->wavl_left;
            tmp2 = sibling->wavl_right;
            p1 = wavl_parity(tmp1);
            if (s_parity == p1) {
                p2 = wavl_parity(tmp2);
                if (s_parity == p2) {
                    __wavl_set_parent_parity(sibling, parent, s_parity ^ 1lu);
                    node = parent;
                    parent = wavl_parent(node);
                    __wavl_set_parent_parity(node, parent, p_parity ^ 1lu);
                    continue;
                }
                sibling->wavl_right = tmp1 = tmp2->wavl_left;
                tmp2->wavl_left = sibling;
                if (tmp1)
                    __wavl_set_parent(tmp1, sibling);
                __wavl_set_parent_parity(sibling, tmp2, s_parity ^ 1lu);
                sibling = tmp2;
                tmp2 = sibling->wavl_right;
            } else {
                if (node || tmp2)
                    p_parity ^= 1lu;
            }
            parent->wavl_left = tmp2; 
            sibling->wavl_right = parent;
            if (tmp2)
                __wavl_set_parent(tmp2, parent);
            __wavl_rotate_set_parents(parent, sibling, root, p_parity);
            break;
        }
    }
}

void wavl_erase(struct wavl_node* node, struct wavl_root* root)
{
    struct wavl_node *child = node->wavl_left, *tmp = node->wavl_right;
    struct wavl_node *parent, *parent1 = wavl_parent(node);
    if (!child || !tmp) {
        tmp = child = (!tmp ? child : tmp);
        parent = parent1;
    } else {
        parent = tmp;
        while (tmp->wavl_left != NULL)
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

void wavl_replace_node(struct wavl_node* victim, struct wavl_node* new,
                       struct wavl_root* root)
{
    struct wavl_node* parent = wavl_parent(victim);
    __wavl_change_child(victim, new, parent, root);

    if (victim->wavl_left)
        __wavl_set_parent(victim->wavl_left, new);

    if (victim->wavl_right)
        __wavl_set_parent(victim->wavl_right, new);

    *new = *victim;
}

struct wavl_node* wavl_first(const struct wavl_root* root)
{
    struct wavl_node *node = root->wavl_node;
    if (!node)
        return NULL;
    while (node->wavl_left)
        node = node->wavl_left;
    return node;
}

struct wavl_node* wavl_last(const struct wavl_root* root)
{
    struct wavl_node *node = root->wavl_node;
    if (!node)
        return NULL;
    while (node->wavl_right)
        node = node->wavl_right;
    return node;
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
