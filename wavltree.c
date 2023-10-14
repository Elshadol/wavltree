#include "wavltree.h"

/* implement a wavltree according to HST's paper Rank-Banlaned Trees.
 * in HST, every node in a wavltree is 1-1, 1-2, or 2-2(rank difference),
 * and 2-2 leaf is not allowed.
 */

#define __wavl_parity(pp) ((pp) & 1)
#define _wavl_parity(x) __wavl_parity((x)->__wavl_parent_parity)
// in HST, external node(null) has rank -1, so its parity is 1
#define wavl_parity(x) ((x) ? _wavl_parity((x)) : 1)
#define __wavl_parent(pp) ((struct wavl_node*)(pp & ~3))
#define wavl_parent(x) __wavl_parent((x)->__wavl_parent_parity)

static inline void
wavl_set_parent(struct wavl_node *node, struct wavl_node *parent)
{
    node->__wavl_parent_parity = _wavl_parity(node) | (unsigned long)parent;
}

static inline void __wavl_flip_parity(struct wavl_node *node)
{
    node->__wavl_parent_parity ^= 1;
}

// promote rank once will change parity of rank
static inline void __wavl_promote_rank(struct wavl_node *node)
{
    __wavl_flip_parity(node);
}

// demote rank once will also change parity of rank
static inline void __wavl_demote_rank(struct wavl_node *node)
{
    __wavl_flip_parity(node);
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
__wavl_set_parent_parity(struct wavl_node *node, struct wavl_node *parent,
                         int parity)
{
    node->__wavl_parent_parity = (unsigned long)parent | parity;
}

static inline void
__wavl_rotate_set_parents(struct wavl_node* old, struct wavl_node* new,
                          struct wavl_root* root, int parity)
{
    struct wavl_node* parent = wavl_parent(old);
    new->__wavl_parent_parity = old->__wavl_parent_parity;
    __wavl_set_parent_parity(old, new, parity);
    __wavl_change_child(old, new, parent, root);
}

void wavl_insert_fixup(struct wavl_node* node, struct wavl_root* root)
{
    int parity, p_parity = _wavl_parity(node);
    struct wavl_node *parent = wavl_parent(node), *tmp, *tmp1;
    while (parent) {
        parity = p_parity;
        p_parity = _wavl_parity(parent);
        // parent from 2-2 to 2-1 or from 1-2 to 1-1, we're done
        if (parity != p_parity)
            break;
        tmp = parent->wavl_right;
        if (node != tmp) {
            // parent from 1-1 to 0-1, promote rank of parent, then climb up
            if (p_parity != wavl_parity(tmp)) {
                p_parity ^= 1;
                __wavl_promote_rank(parent);
                node = parent;
                parent = wavl_parent(node);
                continue;
            }
            // parent from 1-2 to 0-2
            tmp = node->wavl_right;
            // node is 2-1, perform a double rot
            if (parity != wavl_parity(tmp)) {
                node->wavl_right = tmp1 = tmp->wavl_left;
                tmp->wavl_left = node;
                if (tmp1)
                    wavl_set_parent(tmp1, node);
                // in double rotation, node demote once
                __wavl_set_parent_parity(node, tmp, parity ^ 1);
                node = tmp;
            }
            // otherwise, node is 1-2, perform a single rot
            parent->wavl_left = tmp1 = node->wavl_right;
            node->wavl_right = parent;
            if (tmp1)
                wavl_set_parent(tmp1, parent);
            // no matter single or double rot, parent always demote once
            __wavl_rotate_set_parents(parent, node, root, p_parity ^ 1);
            break;
        } else {
            tmp = parent->wavl_left;
            if (p_parity != wavl_parity(tmp)) {
                p_parity ^= 1;
                __wavl_promote_rank(parent);
                node = parent;
                parent = wavl_parent(node);
                continue;
            }
            tmp = node->wavl_left;
            if (parity != wavl_parity(tmp)) {
                node->wavl_left = tmp1 = tmp->wavl_right;
                tmp->wavl_right = node;
                if (tmp1)
                    wavl_set_parent(tmp1, node);
                __wavl_set_parent_parity(node, tmp, parity ^ 1);
                node = tmp;
            }
            parent->wavl_right = tmp1 = node->wavl_left;
            node->wavl_left = parent;
            if (tmp1)
                wavl_set_parent(tmp1, parent);
            __wavl_rotate_set_parents(parent, node, root, p_parity ^ 1);
            break;
        }
    }
}

static void __wavl_erase_fixup(struct wavl_node* node, struct wavl_node* parent,
                               struct wavl_root* root)
{
    struct wavl_node *sibling, *tmp1, *tmp2;
    int p_parity, s_parity, p1, p2;
    if (!parent->wavl_left && !parent->wavl_right) {
        // parent now is a 2-2 leaf, demote once and retry from it
        __wavl_demote_rank(parent);
        node = parent;
        parent = wavl_parent(parent);
        if (!parent)
            return;
    }
    p_parity = _wavl_parity(parent);
    // parent from a binary to unary, no need to climb up
    if (wavl_parity(node) == p_parity)
        return;
    do {
        // now node is a 3-child, cause leaf cannot be 2-2, so its sibling is always non-null
        sibling = parent->wavl_right;
        if (node != sibling) {
            s_parity = _wavl_parity(sibling);
            if (s_parity == p_parity) {
                // parent is 3-2, demote and retry from it
                s_parity ^= 1;       // s_parity now backup parent's rank parity
                __wavl_demote_rank(parent);
                node = parent;
                parent = wavl_parent(parent);
                continue;
            }
            tmp1 = sibling->wavl_right;
            tmp2 = sibling->wavl_left;
            p1 = wavl_parity(tmp1);
            if (s_parity == p1) {
                p2 = wavl_parity(tmp2);
                if (s_parity == p2) {
                    // parent is 3-1, sibling is 2-2, demote both parent and sibling once,
                    // then retry from parent
                    __wavl_demote_rank(sibling);
                    __wavl_demote_rank(parent);
                    node = parent;
                    parent = wavl_parent(parent);
                    continue;
                }
                // sibling is 1-2, perform a double rot, demote sibling once,
                // demote parent twice, promote tmp2 twice,
                // demote twice and promote twice will not change parity of rank
                sibling->wavl_left = tmp1 = tmp2->wavl_right;
                tmp2->wavl_right = sibling;
                if (tmp1)
                    wavl_set_parent(tmp1, sibling);
                __wavl_set_parent_parity(sibling, tmp2, s_parity ^ 1);
                sibling = tmp2;
            } else {
                // sibling is 1-1, or 2-1, perform a single rot
                // note: after a rot, if parent doesn't become a leaf, demote it once,
                // otherwise demote parent twice
                if (parent->wavl_left || tmp2)
                    p_parity ^= 1;
            }
            parent->wavl_right = tmp2 = sibling->wavl_left;
            sibling->wavl_left = parent;
            if (tmp2)
                wavl_set_parent(tmp2, parent);
            __wavl_rotate_set_parents(parent, sibling, root, p_parity);
            break;
        } else {
            sibling = parent->wavl_left;
            s_parity = _wavl_parity(sibling);
            if (s_parity == p_parity) {
                s_parity ^= 1; 
                __wavl_demote_rank(parent);
                node = parent;
                parent = wavl_parent(parent);
                continue;
            }
            tmp1 = sibling->wavl_left;
            tmp2 = sibling->wavl_right;
            p1 = wavl_parity(tmp1);
            if (s_parity == p1) {
                p2 = wavl_parity(tmp2);
                if (s_parity == p2) {
                    __wavl_demote_rank(sibling);
                    __wavl_demote_rank(parent);
                    node = parent;
                    parent = wavl_parent(parent);
                    continue;
                }
                sibling->wavl_right = tmp1 = tmp2->wavl_left;
                tmp2->wavl_left = sibling;
                if (tmp1)
                    wavl_set_parent(tmp1, sibling);
                __wavl_set_parent_parity(sibling, tmp2, s_parity ^ 1);
                sibling = tmp2;
            } else {
                if (parent->wavl_right || tmp2)
                    p_parity ^= 1;
            }
            parent->wavl_left = tmp2 = sibling->wavl_right;
            sibling->wavl_right = parent;
            if (tmp2)
                wavl_set_parent(tmp2, parent);
            __wavl_rotate_set_parents(parent, sibling, root, p_parity);
            break;
        }
        // loop invariant: node is not root, and node is a 3-child
        // remenber that s_parity stores node's rank parity in continue statements
    } while (parent && (p_parity = _wavl_parity(parent)) != s_parity);
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

void wavl_replace_node(struct wavl_node* victim, struct wavl_node* newnode,
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
