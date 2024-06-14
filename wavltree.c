#include "wavltree.h"

static inline unsigned long _wavl_parity(const struct wavl_node *node)
{
    return (node->__wavl_parent_parity & 1lu);
}

static inline unsigned long wavl_parity(const struct wavl_node *node)
{
    /* external node(null) has rank -1, so its rank parity is 1 */
    return (node ? _wavl_parity(node) : 1);
}

static inline struct wavl_node *wavl_even_parent(const struct wavl_node *node)
{
    return (struct wavl_node *)node->__wavl_parent_parity;
}

static inline struct wavl_node *wavl_parent(const struct wavl_node *node)
{
    return (struct wavl_node *)(node->__wavl_parent_parity & ~3lu);
}

static inline void
__wavl_set_parent_parity(struct wavl_node *node, struct wavl_node * parent,
                         unsigned long parity)
{
    node->__wavl_parent_parity = (unsigned long)parent | parity;
}

static inline void
__wavl_set_parent(struct wavl_node *node, struct wavl_node *parent)
{
    __wavl_set_parent_parity(node, parent, _wavl_parity(node));
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

/*
 * Helper function(stolen from linux rbtree) for rotations:
 * - old's parent and rank parity get assigned to new
 * - old gets assigned new as a parent and 'parity' as a rank parity.
 */
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
    unsigned long p_parity, parity = _wavl_parity(node);
    struct wavl_node *parent = wavl_even_parent(node), *tmp, *tmp1;

    /* if after promotion, node is the root, then we're done */
    while (parent) {
        parity = parity ^ 1lu;
        p_parity = _wavl_parity(parent);
        /* if after promotion, node has rank difference 1, then we're done, too */
        if (parity == p_parity)
            break;

        tmp = parent->wavl_right;
        if (node != tmp) { /* node == parent->wavl_left */
            /* parent now is 0-1, promote parent once, and retry from parent */
            if (parity == wavl_parity(tmp)) {
                tmp1 = node;
                node = parent;
                parent = wavl_parent(node);
                /*
                 * remember parity != p_parity, so we can simply assign
                 * parity to node instead of premoting rank of node.
                 * furthermore, parity can be used in next iteration, so
                 * we don't need to calculate rank parity of node in that case
                 */
                __wavl_set_parent_parity(node, parent, parity);
                continue;
            }
            /* parent now is 0-2, check whether node is 2-1 */
            tmp = node->wavl_right;
            if (tmp == tmp1) {
                /*
                 * node is 2-1, need perform a double rotation,
                 * we perform a left rotation at node first,
                 * in a double rotation, demote node once, promote tmp once
                 */
                node->wavl_right = tmp1 = tmp->wavl_left;
                tmp->wavl_left = node;
                if (tmp1)
                    __wavl_set_parent(tmp1, node);
                __wavl_set_parent_parity(node, tmp, parity);
                node = tmp;
                tmp = node->wavl_right;
            }
            /* perform a right rotation at parent, demote parent once */
            node->wavl_right = parent;
            parent->wavl_left = tmp;
            if (tmp)
                __wavl_set_parent(tmp, parent);
            __wavl_rotate_set_parents(parent, node, root, parity);
            /* after a double/single rotation, all violations are restored */
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
__wavl_erase_fixup(struct wavl_node* node, struct wavl_node* parent,
                   struct wavl_root* root)
{
    unsigned long s_parity, p_parity;
    struct wavl_node *sibling, *tmp1, *tmp2;

    /* parent now is a 2-2 leaf, demote parent once and retry from it */
    if (parent->wavl_left == parent->wavl_right) {
        node = parent;
        parent = wavl_parent(node);
        p_parity = _wavl_parity(node);
        __wavl_set_parent_parity(node, parent, p_parity ^ 1lu);
    }
    s_parity = wavl_parity(node);

    while (parent) {
        p_parity = _wavl_parity(parent);
        /* if after demotion, node hase rank difference 2, we're done */
        if (p_parity == s_parity)
            break;

        sibling = parent->wavl_right;
        if (node != sibling) {  /* node == parent->wavl_left */
            if (s_parity != _wavl_parity(sibling)) {
                /* parent is 3-2, demote parent once and retry from it */
                node = parent;
                parent = wavl_parent(node);
                /*
                 * remember s_parity != p_parity, so we can simply assign
                 * s_parity to node instead of demoting rank of node,
                 * furthermore, s_parity can be used in next iteration.
                 * so we don't need to calculate rank parity of node in that case
                 */
                __wavl_set_parent_parity(node, parent, s_parity);
                continue;
            }
            tmp1 = sibling->wavl_right;
            tmp2 = sibling->wavl_left;
            if (p_parity != wavl_parity(tmp1)) {
                /* tmp2 must be nonull in this case */
                if (p_parity != _wavl_parity(tmp2)) {
                    /*
                     * parent is 3-1, sibling is 2-2, demote both parent
                     * and sibling once, then retry from parent.
                     * remember s_parity != p_parity, so we can simply assign
                     * p_parity to sibling instead of demoting rank of sibling
                     */
                    __wavl_set_parent_parity(sibling, parent, p_parity);
                    node = parent;
                    parent = wavl_parent(node);
                    /*
                     * remember s_parity != p_parity, so we can simply assign
                     * s_parity to node instead of demoting rank node,
                     * furthermore, s_parity can be used in next iteration.
                     * so we don't need to calculate rank parity of node in that case
                     */
                    __wavl_set_parent_parity(node, parent, s_parity);
                    continue;
                }
                /*
                 * sibling is 1-2, perform a double rotation, first perform a right rotation at sibling.
                 * in double rotation, demote sibling once, demote parent twice, promote tmp2 twice,
                 */
                sibling->wavl_left = tmp1 = tmp2->wavl_right;
                tmp2->wavl_right = sibling;
                if (tmp1)
                    __wavl_set_parent(tmp1, sibling);
                __wavl_set_parent_parity(sibling, tmp2, p_parity);
                sibling = tmp2;
                tmp2 = sibling->wavl_left;
            } else {
                /*
                 * sibling is 1-1, or 2-1, need perform a left rotation at parent.
                 * note: after a left rotation, if parent become a leaf, demote parent twice,
                 * otherwise demote parent once.
                 */
                if (node != tmp2) /* parent has at least one child */
                    p_parity = s_parity;
            }
            /* sibling is 1-1, or 2-1, need perform a left rotation at parent */
            sibling->wavl_left = parent;
            parent->wavl_right = tmp2;
            if (tmp2)
                __wavl_set_parent(tmp2, parent);
            /* in a left rotation, demote sibling once */
            __wavl_rotate_set_parents(parent, sibling, root, p_parity);
            /* after a double/single rotation, all violations are restored */
            break;
        } else {
            sibling = parent->wavl_left;
            if (s_parity != _wavl_parity(sibling)) {
                node = parent;
                parent = wavl_parent(node);
                __wavl_set_parent_parity(node, parent, s_parity);
                continue;
            }
            tmp1 = sibling->wavl_left;
            tmp2 = sibling->wavl_right;
            if (p_parity != wavl_parity(tmp1)) {
                if (p_parity != _wavl_parity(tmp2)) {
                    __wavl_set_parent_parity(sibling, parent, p_parity);
                    node = parent;
                    parent = wavl_parent(node);
                    __wavl_set_parent_parity(node, parent, s_parity);
                    continue;
                }
                sibling->wavl_right = tmp1 = tmp2->wavl_left;
                tmp2->wavl_left = sibling;
                if (tmp1)
                    __wavl_set_parent(tmp1, sibling);
                __wavl_set_parent_parity(sibling, tmp2, p_parity);
                sibling = tmp2;
                tmp2 = sibling->wavl_right;
            } else {
                if (node != tmp2)
                    p_parity = s_parity;
            }
            sibling->wavl_right = parent;
            parent->wavl_left = tmp2;
            if (tmp2)
                __wavl_set_parent(tmp2, parent);
            __wavl_rotate_set_parents(parent, sibling, root, p_parity);
            break;
        }
    }
}

/* stolen from freebsd tree.h */
void wavl_erase(struct wavl_node* node, struct wavl_root* root)
{
    struct wavl_node *child = node->wavl_left, *tmp = node->wavl_right;
    struct wavl_node *parent, *parent1 = wavl_parent(node);
    if (!child || !tmp) {
        /* node to erase has no more than 1 child */
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
             /* node's successor is leftmost under node's right child subtree */
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

struct wavl_node* wavl_prev(const struct wavl_node* node)
{
    struct wavl_node* parent;

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
