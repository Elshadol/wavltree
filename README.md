# wavltree
A wavl-tree implementation with the similar API as linux rbtree.
This implementation is compact and a litte hard to read, but has extremly fast performance.
Test results in MacBook Air(m1, 16GB) shows as following.
linux rbtree with 100000 nodes:
insert time: 22ms, height=20
search time: 21ms error=0
delete time: 14ms
total: 57ms

wavlmini with 100000 nodes:
insert time: 30ms, height=20
search time: 24ms error=0
delete time: 13ms
total: 67ms

linux rbtree with 1000000 nodes:
insert time: 202ms, height=26
search time: 157ms error=0
delete time: 59ms
total: 418ms

wavlmini with 1000000 nodes:
insert time: 213ms, height=24
search time: 153ms error=0
delete time: 60ms
total: 426ms

linux rbtree with 10000000 nodes:
insert time: 2265ms, height=32
search time: 1508ms error=0
delete time: 424ms
total: 4197ms

wavlmini with 10000000 nodes:
insert time: 2259ms, height=27
search time: 1512ms error=0
delete time: 438ms
total: 4209ms
