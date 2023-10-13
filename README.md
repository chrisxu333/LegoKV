LegoKV: build kv engine like Lego

Component-based architecture of storage system. 
Change and upgrade of any component is unsensable to all other users and upper layers.
Dynamic registration

Whole datapath
- DB impl: put / get
- Memtable
- Trigger flush
- SST
- Trigger Compaction

RoadMap v1.0.0:
- Util Components:
    - autovector
        Unit test missing.
    - random
        A easy yet efficient random number generator.
        Unit test passed.
    - hashing
        Unit test passed.
    - FastRange: speedup hashing with modular op to a specific range.
        Unit test passed.
    - string_util
        Collection of convenient string utility funtions.
        Unit test missing.
    - math
        High performance implementation of various math operations.
        Unit test passed.
    - coding
        Encoder and Decoder
        Unit test passed.
- KV Components
    - Memtable Impl
        SkipList
    - Bloom Filter Impl
    - LRUCache Impl
    - Compaction Policy Impl
    - SSTable Reader Impl
    - Benchmarking Impl
    - Memory Allocator Impl (from rocksdb)
        Later on we could provide multiple Impl for pluggable usage.
    - Rate limiter Impl (from rocksdb)
    - Thread pool Impl (from rocksdb)
- New components from research paper
    - HotRing [https://www.usenix.org/system/files/fast20-chen_jiqiang.pdf]
    - REMIX []