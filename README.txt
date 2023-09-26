Qiyuan Yin
qy765@nyu.edu

Status of my code:
Completed

I included map, vector, list and most of my code for proj2 in this program.

I created a class named LRUcache standing for every row in the cache. In the class LRUcache, m_list is a vector to store different tags, and tags are arranged by their activity order. If the program uses one tag, it will move it to the front. The tag at the end of m_list is the least recent use one. Besides, a map called "block" is also created in the class to record the block and corresponding tags. The vector of LRUcache works like a cache, and every single LRUcache in this vector is like a single row.

Advantage: The class LRUcache guarantees that whatever the number of cache's associativity is, the program will not collapse. Every time when the program needs to check if it is a miss or eviction, it can just check the size of map "block" in class LRUcache. If the size of map in this row is as the same as associativity, it needs eviction. If smaller, it just needs to add new block.

Disadvantage: The code has some redundant parts. For example, instructions except SW and LW in L1 and L1 & L2 are the same. I tried to write a function to include these instructions, but there are too many parameters need to pass in and different variables should be returned. Eventually, I just copied my code for proj2 twice in this program.