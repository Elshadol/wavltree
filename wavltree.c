#include "wavltree.h"

/*
 * Rank difference of a node is the difference betweenthe rank of the node
 * and the rank of node's parent.
 *
 * A node is a x,y-node if it has one child with rank difference x,
 * and one child with rank difference y.
 *
 * A node is a x-child if its rank difference is x.
 */

/*
 * weak Avl(wavl) trees satisfy the following properties:
 * 1) All rank difference is 1 or 2.
 * 2) Every leaf has rank 0.
 */

/*
 * In this implementation, we use rank parity instead of rank.
 * parity(x) == parity(x's parent) means rank difference of x is 0 or 2.
 * parity(x) != parity(x's parent) means rank difference of x is 1 or 3.
 */

static inline unsigned long __wavl_parity(const struct wavl_node *node) {
  return (node->__wavl_parent_parity & 1lu);
}

static inline unsigned long wavl_parity(const struct wavl_node *node) {
  /* null's rank is -1, so its rank parity is 1 */
  return (node ? __wavl_parity(node) : 1lu);
}

static inline struct wavl_node *wavl_even_parent(const struct wavl_node *node) {
  return (struct wavl_node *)node->__wavl_parent_parity;
}

static inline struct wavl_node *wavl_parent(const struct wavl_node *node) {
  return (struct wavl_node *)(node->__wavl_parent_parity & ~3lu);
}

static inline void wavl_set_parent_parity(struct wavl_node *node,
                                          struct wavl_node *parent,
                                          unsigned long parity) {
  node->__wavl_parent_parity = (unsigned long)parent + parity;
}

static inline void wavl_set_parent(struct wavl_node *node,
                                   struct wavl_node *parent) {
  wavl_set_parent_parity(node, parent, __wavl_parity(node));
}

static inline void __wavl_change_child(struct wavl_node *old,
                                       struct wavl_node *new,
                                       struct wavl_node *parent,
                                       struct wavl_root *root) {
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
 * - old gets assigned new as a parent and 'parity' as a rank parity
 */
static inline void __wavl_rotate_set_parents(struct wavl_node *old,
                                             struct wavl_node *new,
                                             struct wavl_root *root,
                                             unsigned long parity) {
  struct wavl_node *parent = wavl_parent(old);
  new->__wavl_parent_parity = old->__wavl_parent_parity;
  wavl_set_parent_parity(old, new, parity);
  __wavl_change_child(old, new, parent, root);
}

void wavl_insert_fixup(struct wavl_node *node, struct wavl_root *root) {
  unsigned long p1 = 0lu;
  struct wavl_node *parent = wavl_even_parent(node), *tmp1, *tmp2;

  while (1) {
    /*
     * Loop invariant: node's rank just increased by 1.
     */

    /*
     * The inserted node is root. Either this is the
     * first node, or we recursed at Case 1 below
     * are no longer violating 1).
     */
    if (!parent)
      break;

    /*
     * If after promotion, node is from 2-child to 1-child, we're done.
     * Otherwise, need take some corrective action to handle 0-child case.
     */
    if (p1 != __wavl_parity(parent))
      break;

    /*
     * Sine node and parent have the same rank parity,
     * we flip p1 for promoting/demoting node and parent later.
     */
    p1 ^= 1lu;

    tmp1 = parent->wavl_right;
    if (node != tmp1) { /* node == parent->wavl_left */
      if (p1 == wavl_parity(tmp1)) {
        /*
         * Case 1: node’s sibling is a 1-child.
         * In this case, we promote parent once by a single bit flip,
         *
         *         n-p           p
         *            \  -->    / \
         *             s       n   \
         *                          s
         *
         * However, after promotion, parent might be a 0-child, and 1)
         * does not allow this, we need recurse at parent.
         */
        node = parent;
        parent = wavl_parent(node);
        wavl_set_parent_parity(node, parent, p1);
        continue;
      }

      tmp1 = node->wavl_right;
      if (p1 == wavl_parity(tmp1)) {
        /*
         * Case 2: node’s sibling is a 2-child and node's right child
         * is a 1-child (left rotate at node).
         *
         *      n-p              t1-p
         *     / \ \     -->     /   \
         *    /  t1 \           n     \
         *   t2      s         /       s
         *                    t2
         *
         * This still leaves us in violation of 1), the continuation
         * into Case 3 will fix that.
         */
        node->wavl_right = tmp2 = tmp1->wavl_left;
        tmp1->wavl_left = node;
        if (tmp2)
          wavl_set_parent(tmp2, node);
        wavl_set_parent_parity(node, tmp1, p1);
        node = tmp1;
        tmp1 = node->wavl_right;
      }

      /*
       * Case 3: node’s sibling is a 2-child and node's right child
       * is a 2-child (right rotate at parent).
       *
       *      n-p    -->   n
       *     / \ \        / \
       *    t2  \ \     t2   p
       *        t1 s        / \
       *                  t1   s
       */
      parent->wavl_left = tmp1;
      node->wavl_right = parent;
      if (tmp1)
        wavl_set_parent(tmp1, parent);
      __wavl_rotate_set_parents(parent, node, root, p1);
      break;
    } else {
      tmp1 = parent->wavl_left;
      if (p1 == wavl_parity(tmp1)) {
        node = parent;
        parent = wavl_parent(node);
        wavl_set_parent_parity(node, parent, p1);
        continue;
      }

      tmp1 = node->wavl_left;
      if (p1 == wavl_parity(tmp1)) {
        node->wavl_left = tmp2 = tmp1->wavl_right;
        tmp1->wavl_right = node;
        if (tmp2)
          wavl_set_parent(tmp2, node);
        wavl_set_parent_parity(node, tmp1, p1);
        node = tmp1;
        tmp1 = node->wavl_left;
      }

      parent->wavl_right = tmp1;
      node->wavl_left = parent;
      if (tmp1)
        wavl_set_parent(tmp1, parent);
      __wavl_rotate_set_parents(parent, node, root, p1);
      break;
    }
  }
}

static inline void __wavl_erase_fixup(struct wavl_node *node,
                                      struct wavl_node *parent,
                                      unsigned long p1,
                                      struct wavl_root *root) {
  unsigned long p2;
  struct wavl_node *sibling, *tmp1, *tmp2;

  if (parent->wavl_left == parent->wavl_right) {
    /* if parent become a leaf, demote parent once to make its rank 0 */
    p1 ^= 1lu;
    node = parent;
    parent = wavl_parent(node);
    wavl_set_parent_parity(node, parent, p1);
    if (!parent)
      return;
  }

  do {
    /*
     * Loop invariant: node's rank just decreased by 1.
     */

    p2 = __wavl_parity(parent);

    /*
     * If after demotion, node is from 1-child to 2-child, we're done.
     * Otherwise, need take some corrective action to handle 3-child case.
     */
    if (p1 == p2)
      break;

    sibling = parent->wavl_right;
    if (node != sibling) { /* node == parent->wavl_left */
      if (p1 != __wavl_parity(sibling)) {
        /*
         * Case 1: node's sibling is a 2-child (demote parent once).
         *
         *          p              p
         *         / \    -->     / \
         *        /   \          /   s
         *       /     s        n
         *      n
         *
         * After demotion, parent might be a 3-child, and 1)
         * does not allow this, we need recurse at parent.
         */
        node = parent;
        parent = wavl_parent(node);
        wavl_set_parent_parity(node, parent, p1);
        continue;
      }

      tmp1 = sibling->wavl_right;
      tmp2 = sibling->wavl_left;
      if (p2 != wavl_parity(tmp1)) {
        if (p2 != __wavl_parity(tmp2)) {
          /*
           * Case 2: node's sibling is a 2,2-node.
           * In this case, demote both parent and sibling once.
           *          p                    p
           *         / \                  / \
           *        /   s       -->      /   s
           *       /   / \              n   / \
           *      n   /   \                t2  t1
           *         t2    t1
           * After demotion, parent might be a 3-child, and 1)
           * does not allow this, we also need recurse at parent.
           */
          wavl_set_parent_parity(sibling, parent, p2);
          node = parent;
          parent = wavl_parent(node);
          wavl_set_parent_parity(node, parent, p1);
          continue;
        }

        /*
         * Case 3: node's sibling is a 1-child and node's far
         * nephew(tmp1) is a 2-child. This is a double-rotate case,
         * we perform a right rotate at sibling first.
         *
         *            p                          p
         *           / \                        / \
         *          /   s         -->          /   t2
         *         /   / \                    /     \
         *        n   t2  \                  n       s
         *                 t1                         \
         *                                             t1
         * This still leaves us in violation 1), the
         * continuation into Case 4 will fix that.
         */
        sibling->wavl_left = tmp1 = tmp2->wavl_right;
        tmp2->wavl_right = sibling;
        if (tmp1)
          wavl_set_parent(tmp1, sibling);
        wavl_set_parent_parity(sibling, tmp2, p2);
        sibling = tmp2;
        tmp2 = sibling->wavl_left;
      } else {
        /*
         * In single-rotate case, if parent doesn't become a leaf,
         * demote parent's rank once by a single flip.
         */
        if (node != tmp2)
          p2 = p1;
        /*
         * if parent will become a leaf(both node and tmp2 are null),
         * demote parent's rank twice. Since demoting twice doesn't
         * change parent's rank parity, we do nothing.
         */
      }

      /*
       * Case 4: node's sibling is a 1-child and its far
       * nephew(tmp1) is a 1-child (left rotate at parent).
       *      p              s            p            s
       *     / \            / \          / \          / \
       *    /   s    -->   p   \    or  /   s  -->   p   \
       *   /   / \        / \   t1     /   / \      / \   t1
       *  n  t2   t1     /   t2       n   /  t1    /   \
       *                n                t2       n    t2
       */
      parent->wavl_right = tmp2;
      sibling->wavl_left = parent;
      if (tmp2)
        wavl_set_parent(tmp2, parent);
      __wavl_rotate_set_parents(parent, sibling, root, p2);
      break;
    } else {
      sibling = parent->wavl_left;
      if (p1 != __wavl_parity(sibling)) {
        node = parent;
        parent = wavl_parent(node);
        wavl_set_parent_parity(node, parent, p1);
        continue;
      }

      tmp1 = sibling->wavl_left;
      tmp2 = sibling->wavl_right;
      if (p2 != wavl_parity(tmp1)) {
        if (p2 != __wavl_parity(tmp2)) {
          wavl_set_parent_parity(sibling, parent, p2);
          node = parent;
          parent = wavl_parent(node);
          wavl_set_parent_parity(node, parent, p1);
          continue;
        }

        sibling->wavl_right = tmp1 = tmp2->wavl_left;
        tmp2->wavl_left = sibling;
        if (tmp1)
          wavl_set_parent(tmp1, sibling);
        wavl_set_parent_parity(sibling, tmp2, p2);
        sibling = tmp2;
        tmp2 = sibling->wavl_right;
      } else {
        if (node != tmp2)
          p2 = p1;
      }

      parent->wavl_left = tmp2;
      sibling->wavl_right = parent;
      if (tmp2)
        wavl_set_parent(tmp2, parent);
      __wavl_rotate_set_parents(parent, sibling, root, p2);
      break;
    }
  } while (parent);
}

/* stolen from freebsd's tree.h */
void wavl_erase(struct wavl_node *node, struct wavl_root *root) {
  unsigned long p1 = 1lu;
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
    wavl_set_parent(child, tmp);
    child = tmp->wavl_right;
    if (parent != tmp) {
      /* node's successor is leftmost under node's right child subtree */
      wavl_set_parent(parent, tmp);
      tmp->wavl_right = parent;
      parent = wavl_parent(tmp);
      parent->wavl_left = child;
    }
    tmp->__wavl_parent_parity = node->__wavl_parent_parity;
  }
  __wavl_change_child(node, tmp, parent1, root);
  if (child) {
    p1 = 0lu;
    child->__wavl_parent_parity = (unsigned long)parent;
  }
  if (parent)
    __wavl_erase_fixup(child, parent, p1, root);
}

struct wavl_node *wavl_first(const struct wavl_root *root) {
  struct wavl_node *node = root->wavl_node;
  if (node) {
    while (node->wavl_left)
      node = node->wavl_left;
  }
  return node;
}

struct wavl_node *wavl_last(const struct wavl_root *root) {
  struct wavl_node *node = root->wavl_node;
  if (node) {
    while (node->wavl_right)
      node = node->wavl_right;
  }
  return node;
}

struct wavl_node *wavl_next(const struct wavl_node *node) {
  struct wavl_node *parent;

  if (WAVL_EMPTY_NODE(node))
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

struct wavl_node *wavl_prev(const struct wavl_node *node) {
  struct wavl_node *parent;

  if (WAVL_EMPTY_NODE(node))
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
