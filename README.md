# wavltree
A weak avl-tree implementation(similar to linux rbtree API) according to Haeupler, Sen, Tarjan, "Rank-Balanced Trees".
This implementation is very very compact(about 534 lines of assembly codes, linux rbtree has 525 lines of assembly codes on my m1 macbook air).
This implementation takes many good ideas from linux rbtree and freebsd rbtree.
This implementation is litte hard to read, but has extremly fast performance(no more than 10% slower than linux rbtree when dealing with random data sets).
