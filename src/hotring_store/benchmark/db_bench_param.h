#pragma once
#include <atomic>
#include <cinttypes>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <optional>

#include "monitoring/histogram.h"
#include "monitoring/statistics_impl.h"
#include "port/port.h"
#include "port/stack_trace.h"
#include <gflags/gflags.h>

DEFINE_string(
    benchmarks,
    "fillseq,"
    "fillseqdeterministic,"
    "fillsync,"
    "fillrandom,"
    "filluniquerandomdeterministic,"
    "overwrite,"
    "readrandom,"
    "newiterator,"
    "newiteratorwhilewriting,"
    "seekrandom,"
    "seekrandomwhilewriting,"
    "seekrandomwhilemerging,"
    "readseq,"
    "readreverse,"
    "compact,"
    "compactall,"
    "flush,"
    "compact0,"
    "compact1,"
    "waitforcompaction,"
    "multireadrandom,"
    "mixgraph,"
    "readseq,"
    "readtorowcache,"
    "readtocache,"
    "readreverse,"
    "readwhilewriting,"
    "readwhilemerging,"
    "readwhilescanning,"
    "readrandomwriterandom,"
    "updaterandom,"
    "xorupdaterandom,"
    "approximatesizerandom,"
    "randomwithverify,"
    "fill100K,"
    "crc32c,"
    "xxhash,"
    "xxhash64,"
    "xxh3,"
    "compress,"
    "uncompress,"
    "acquireload,"
    "fillseekseq,"
    "randomtransaction,"
    "randomreplacekeys,"
    "timeseries,"
    "getmergeoperands,",
    "readrandomoperands,"
    "backup,"
    "restore"

    "Comma-separated list of operations to run in the specified"
    " order. Available benchmarks:\n"
    "\tfillseq       -- write N values in sequential key"
    " order in async mode\n"
    "\tfillseqdeterministic       -- write N values in the specified"
    " key order and keep the shape of the LSM tree\n"
    "\tfillrandom    -- write N values in random key order in async"
    " mode\n"
    "\tfilluniquerandomdeterministic       -- write N values in a random"
    " key order and keep the shape of the LSM tree\n"
    "\toverwrite     -- overwrite N values in random key order in "
    "async mode\n"
    "\tfillsync      -- write N/1000 values in random key order in "
    "sync mode\n"
    "\tfill100K      -- write N/1000 100K values in random order in"
    " async mode\n"
    "\tdeleteseq     -- delete N keys in sequential order\n"
    "\tdeleterandom  -- delete N keys in random order\n"
    "\treadseq       -- read N times sequentially\n"
    "\treadtocache   -- 1 thread reading database sequentially\n"
    "\treadreverse   -- read N times in reverse order\n"
    "\treadrandom    -- read N times in random order\n"
    "\treadmissing   -- read N missing keys in random order\n"
    "\treadwhilewriting      -- 1 writer, N threads doing random "
    "reads\n"
    "\treadwhilemerging      -- 1 merger, N threads doing random "
    "reads\n"
    "\treadwhilescanning     -- 1 thread doing full table scan, "
    "N threads doing random reads\n"
    "\treadrandomwriterandom -- N threads doing random-read, "
    "random-write\n"
    "\tupdaterandom  -- N threads doing read-modify-write for random "
    "keys\n"
    "\txorupdaterandom  -- N threads doing read-XOR-write for "
    "random keys\n"
    "\tappendrandom  -- N threads doing read-modify-write with "
    "growing values\n"
    "\tmergerandom   -- same as updaterandom/appendrandom using merge"
    " operator. "
    "Must be used with merge_operator\n"
    "\treadrandommergerandom -- perform N random read-or-merge "
    "operations. Must be used with merge_operator\n"
    "\tnewiterator   -- repeated iterator creation\n"
    "\tseekrandom    -- N random seeks, call Next seek_nexts times "
    "per seek\n"
    "\tseekrandomwhilewriting -- seekrandom and 1 thread doing "
    "overwrite\n"
    "\tseekrandomwhilemerging -- seekrandom and 1 thread doing "
    "merge\n"
    "\tcrc32c        -- repeated crc32c of <block size> data\n"
    "\txxhash        -- repeated xxHash of <block size> data\n"
    "\txxhash64      -- repeated xxHash64 of <block size> data\n"
    "\txxh3          -- repeated XXH3 of <block size> data\n"
    "\tacquireload   -- load N*1000 times\n"
    "\tfillseekseq   -- write N values in sequential key, then read "
    "them by seeking to each key\n"
    "\trandomtransaction     -- execute N random transactions and "
    "verify correctness\n"
    "\trandomreplacekeys     -- randomly replaces N keys by deleting "
    "the old version and putting the new version\n\n"
    "\ttimeseries            -- 1 writer generates time series data "
    "and multiple readers doing random reads on id\n\n"
    "Meta operations:\n"
    "\tcompact     -- Compact the entire DB; If multiple, randomly choose one\n"
    "\tcompactall  -- Compact the entire DB\n"
    "\tcompact0  -- compact L0 into L1\n"
    "\tcompact1  -- compact L1 into L2\n"
    "\twaitforcompaction - pause until compaction is (probably) done\n"
    "\tflush - flush the memtable\n"
    "\tstats       -- Print DB stats\n"
    "\tresetstats  -- Reset DB stats\n"
    "\tlevelstats  -- Print the number of files and bytes per level\n"
    "\tmemstats  -- Print memtable stats\n"
    "\tsstables    -- Print sstable info\n"
    "\theapprofile -- Dump a heap profile (if supported by this port)\n"
    "\treplay      -- replay the trace file specified with trace_file\n"
    "\tgetmergeoperands -- Insert lots of merge records which are a list of "
    "sorted ints for a key and then compare performance of lookup for another "
    "key by doing a Get followed by binary searching in the large sorted list "
    "vs doing a GetMergeOperands and binary searching in the operands which "
    "are sorted sub-lists. The MergeOperator used is sortlist.h\n"
    "\treadrandomoperands -- read random keys using `GetMergeOperands()`. An "
    "operation includes a rare but possible retry in case it got "
    "`Status::Incomplete()`. This happens upon encountering more keys than "
    "have ever been seen by the thread (or eight initially)\n"
    "\tbackup --  Create a backup of the current DB and verify that a new backup is corrected. "
    "Rate limit can be specified through --backup_rate_limit\n"
    "\trestore -- Restore the DB from the latest backup available, rate limit can be specified through --restore_rate_limit\n");

DEFINE_int64(num, 1000000, "Number of key/values to place in database");

DEFINE_int64(numdistinct, 1000,
             "Number of distinct keys to use. Used in RandomWithVerify to "
             "read/write on fewer keys so that gets are more likely to find the"
             " key and puts are more likely to update the same key");

DEFINE_int64(merge_keys, -1,
             "Number of distinct keys to use for MergeRandom and "
             "ReadRandomMergeRandom. "
             "If negative, there will be FLAGS_num keys.");
DEFINE_int32(num_column_families, 1, "Number of Column Families to use.");

DEFINE_int32(
    num_hot_column_families, 0,
    "Number of Hot Column Families. If more than 0, only write to this "
    "number of column families. After finishing all the writes to them, "
    "create new set of column families and insert to them. Only used "
    "when num_column_families > 1.");

DEFINE_string(column_family_distribution, "",
              "Comma-separated list of percentages, where the ith element "
              "indicates the probability of an op using the ith column family. "
              "The number of elements must be `num_hot_column_families` if "
              "specified; otherwise, it must be `num_column_families`. The "
              "sum of elements must be 100. E.g., if `num_column_families=4`, "
              "and `num_hot_column_families=0`, a valid list could be "
              "\"10,20,30,40\".");

DEFINE_int64(reads, -1,
             "Number of read operations to do.  "
             "If negative, do FLAGS_num reads.");

DEFINE_int64(deletes, -1,
             "Number of delete operations to do.  "
             "If negative, do FLAGS_num deletions.");

DEFINE_int32(bloom_locality, 0, "Control bloom filter probes locality");

DEFINE_int64(seed, 0,
             "Seed base for random number generators. "
             "When 0 it is derived from the current time.");
static std::optional<int64_t> seed_base;

DEFINE_int32(threads, 1, "Number of concurrent threads to run.");

DEFINE_int32(duration, 0,
             "Time in seconds for the random-ops tests to run."
             " When 0 then num & reads determine the test duration");

DEFINE_string(value_size_distribution_type, "fixed",
              "Value size distribution type: fixed, uniform, normal");

DEFINE_int32(value_size, 100, "Size of each value in fixed distribution");
static unsigned int value_size = 100;

DEFINE_int32(value_size_min, 100, "Min size of random value");

DEFINE_int32(value_size_max, 102400, "Max size of random value");

DEFINE_int32(seek_nexts, 0,
             "How many times to call Next() after Seek() in "
             "fillseekseq, seekrandom, seekrandomwhilewriting and "
             "seekrandomwhilemerging");

DEFINE_bool(reverse_iterator, false,
            "When true use Prev rather than Next for iterators that do "
            "Seek and then Next");

DEFINE_bool(auto_prefix_mode, false, "Set auto_prefix_mode for seek benchmark");

DEFINE_int64(max_scan_distance, 0,
             "Used to define iterate_upper_bound (or iterate_lower_bound "
             "if FLAGS_reverse_iterator is set to true) when value is nonzero");

DEFINE_bool(use_uint64_comparator, false, "use Uint64 user comparator");

DEFINE_int64(batch_size, 1, "Batch size");

static bool ValidateKeySize(const char* /*flagname*/, int32_t /*value*/) {
  return true;
}

static bool ValidateUint32Range(const char* flagname, uint64_t value) {
  if (value > std::numeric_limits<uint32_t>::max()) {
    fprintf(stderr, "Invalid value for --%s: %lu, overflow\n", flagname,
            (unsigned long)value);
    return false;
  }
  return true;
}

DEFINE_int32(key_size, 16, "size of each key");

DEFINE_int32(user_timestamp_size, 0,
             "number of bytes in a user-defined timestamp");

DEFINE_int32(num_multi_db, 0,
             "Number of DBs used in the benchmark. 0 means single DB.");

DEFINE_double(compression_ratio, 0.5,
              "Arrange to generate values that shrink to this fraction of "
              "their original size after compression");

DEFINE_double(
    overwrite_probability, 0.0,
    "Used in 'filluniquerandom' benchmark: for each write operation, "
    "we give a probability to perform an overwrite instead. The key used for "
    "the overwrite is randomly chosen from the last 'overwrite_window_size' "
    "keys previously inserted into the DB. "
    "Valid overwrite_probability values: [0.0, 1.0].");

DEFINE_uint32(overwrite_window_size, 1,
              "Used in 'filluniquerandom' benchmark. For each write operation,"
              " when the overwrite_probability flag is set by the user, the "
              "key used to perform an overwrite is randomly chosen from the "
              "last 'overwrite_window_size' keys previously inserted into DB. "
              "Warning: large values can affect throughput. "
              "Valid overwrite_window_size values: [1, kMaxUint32].");

DEFINE_uint64(
    disposable_entries_delete_delay, 0,
    "Minimum delay in microseconds for the series of Deletes "
    "to be issued. When 0 the insertion of the last disposable entry is "
    "immediately followed by the issuance of the Deletes. "
    "(only compatible with fillanddeleteuniquerandom benchmark).");

DEFINE_uint64(disposable_entries_batch_size, 0,
              "Number of consecutively inserted disposable KV entries "
              "that will be deleted after 'delete_delay' microseconds. "
              "A series of Deletes is always issued once all the "
              "disposable KV entries it targets have been inserted "
              "into the DB. When 0 no deletes are issued and a "
              "regular 'filluniquerandom' benchmark occurs. "
              "(only compatible with fillanddeleteuniquerandom benchmark)");

DEFINE_int32(disposable_entries_value_size, 64,
             "Size of the values (in bytes) of the entries targeted by "
             "selective deletes. "
             "(only compatible with fillanddeleteuniquerandom benchmark)");

DEFINE_uint64(
    persistent_entries_batch_size, 0,
    "Number of KV entries being inserted right before the deletes "
    "targeting the disposable KV entries are issued. These "
    "persistent keys are not targeted by the deletes, and will always "
    "remain valid in the DB. (only compatible with "
    "--benchmarks='fillanddeleteuniquerandom' "
    "and used when--disposable_entries_batch_size is > 0).");

DEFINE_int32(persistent_entries_value_size, 64,
             "Size of the values (in bytes) of the entries not targeted by "
             "deletes. (only compatible with "
             "--benchmarks='fillanddeleteuniquerandom' "
             "and used when--disposable_entries_batch_size is > 0).");

DEFINE_double(read_random_exp_range, 0.0,
              "Read random's key will be generated using distribution of "
              "num * exp(-r) where r is uniform number from 0 to this value. "
              "The larger the number is, the more skewed the reads are. "
              "Only used in readrandom and multireadrandom benchmarks.");

DEFINE_bool(histogram, false, "Print histogram of operation timings");

DEFINE_bool(confidence_interval_only, false,
            "Print 95% confidence interval upper and lower bounds only for "
            "aggregate stats.");

DEFINE_bool(enable_numa, false,
            "Make operations aware of NUMA architecture and bind memory "
            "and cpus corresponding to nodes together. In NUMA, memory "
            "in same node as CPUs are closer when compared to memory in "
            "other nodes. Reads can be faster when the process is bound to "
            "CPU and memory of same node. Use \"$numactl --hardware\" command "
            "to see NUMA memory architecture.");

DEFINE_bool(cost_write_buffer_to_cache, false,
            "The usage of memtable is costed to the block cache");

DEFINE_int32(num_bottom_pri_threads, 0,
             "The number of threads in the bottom-priority thread pool (used "
             "by universal compaction only).");

DEFINE_int32(num_high_pri_threads, 0,
             "The maximum number of concurrent background compactions"
             " that can occur in parallel.");

DEFINE_int32(num_low_pri_threads, 0,
             "The maximum number of concurrent background compactions"
             " that can occur in parallel.");

DEFINE_int32(universal_size_ratio, 0,
             "Percentage flexibility while comparing file size "
             "(for universal compaction only).");

DEFINE_int32(universal_min_merge_width, 0,
             "The minimum number of files in a single compaction run "
             "(for universal compaction only).");

DEFINE_int32(universal_max_merge_width, 0,
             "The max number of files to compact in universal style "
             "compaction");

DEFINE_int32(universal_max_size_amplification_percent, 0,
             "The max size amplification for universal style compaction");

DEFINE_int32(universal_compression_size_percent, -1,
             "The percentage of the database to compress for universal "
             "compaction. -1 means compress everything.");

DEFINE_bool(universal_allow_trivial_move, false,
            "Allow trivial move in universal compaction.");

DEFINE_bool(universal_incremental, false,
            "Enable incremental compactions in universal compaction.");

DEFINE_int64(cache_size, 32 << 20,  // 32MB
             "Number of bytes to use as a cache of uncompressed data");

DEFINE_int32(cache_numshardbits, -1,
             "Number of shards for the block cache"
             " is 2 ** cache_numshardbits. Negative means use default settings."
             " This is applied only if FLAGS_cache_size is non-negative.");

DEFINE_double(cache_high_pri_pool_ratio, 0.0,
              "Ratio of block cache reserve for high pri blocks. "
              "If > 0.0, we also enable "
              "cache_index_and_filter_blocks_with_high_priority.");

DEFINE_double(cache_low_pri_pool_ratio, 0.0,
              "Ratio of block cache reserve for low pri blocks.");

DEFINE_string(cache_type, "lru_cache", "Type of block cache.");

DEFINE_bool(use_compressed_secondary_cache, false,
            "Use the CompressedSecondaryCache as the secondary cache.");

DEFINE_int64(compressed_secondary_cache_size, 32 << 20,  // 32MB
             "Number of bytes to use as a cache of data");

DEFINE_int32(compressed_secondary_cache_numshardbits, 6,
             "Number of shards for the block cache"
             " is 2 ** compressed_secondary_cache_numshardbits."
             " Negative means use default settings."
             " This is applied only if FLAGS_cache_size is non-negative.");

DEFINE_double(compressed_secondary_cache_high_pri_pool_ratio, 0.0,
              "Ratio of block cache reserve for high pri blocks. "
              "If > 0.0, we also enable "
              "cache_index_and_filter_blocks_with_high_priority.");

DEFINE_double(compressed_secondary_cache_low_pri_pool_ratio, 0.0,
              "Ratio of block cache reserve for low pri blocks.");

DEFINE_string(compressed_secondary_cache_compression_type, "lz4",
              "The compression algorithm to use for large "
              "values stored in CompressedSecondaryCache.");

DEFINE_uint32(
    compressed_secondary_cache_compress_format_version, 2,
    "compress_format_version can have two values: "
    "compress_format_version == 1 -- decompressed size is not included"
    " in the block header."
    "compress_format_version == 2 -- decompressed size is included"
    " in the block header in varint32 format.");

DEFINE_bool(use_tiered_volatile_cache, false,
            "If use_compressed_secondary_cache is true and "
            "use_tiered_volatile_cache is true, then allocate a tiered cache "
            "that distributes cache reservations proportionally over both "
            "the caches.");

DEFINE_int64(simcache_size, -1,
             "Number of bytes to use as a simcache of "
             "uncompressed data. Nagative value disables simcache.");

DEFINE_bool(cache_index_and_filter_blocks, false,
            "Cache index/filter blocks in block cache.");

DEFINE_bool(use_cache_jemalloc_no_dump_allocator, false,
            "Use JemallocNodumpAllocator for block/blob cache.");

DEFINE_bool(use_cache_memkind_kmem_allocator, false,
            "Use memkind kmem allocator for block/blob cache.");

DEFINE_bool(partition_index_and_filters, false,
            "Partition index and filter blocks.");

DEFINE_bool(partition_index, false, "Partition index blocks");

DEFINE_bool(index_with_first_key, false, "Include first key in the index");

DEFINE_int64(
    index_shortening_mode, 2,
    "mode to shorten index: 0 for no shortening; 1 for only shortening "
    "separaters; 2 for shortening shortening and successor");

// The default reduces the overhead of reading time with flash. With HDD, which
// offers much less throughput, however, this number better to be set to 1.
DEFINE_int32(ops_between_duration_checks, 1000,
             "Check duration limit every x ops");

DEFINE_bool(pin_l0_filter_and_index_blocks_in_cache, false,
            "Pin index/filter blocks of L0 files in block cache.");

DEFINE_bool(
    pin_top_level_index_and_filter, false,
    "Pin top-level index of partitioned index/filter blocks in block cache.");

DEFINE_int64(prepopulate_block_cache, 0,
             "Pre-populate hot/warm blocks in block cache. 0 to disable and 1 "
             "to insert during flush");

DEFINE_bool(use_data_block_hash_index, false,
            "if use kDataBlockBinaryAndHash "
            "instead of kDataBlockBinarySearch. "
            "This is valid if only we use BlockTable");

DEFINE_double(data_block_hash_table_util_ratio, 0.75,
              "util ratio for data block hash index table. "
              "This is only valid if use_data_block_hash_index is "
              "set to true");

DEFINE_int64(compressed_cache_size, -1,
             "Number of bytes to use as a cache of compressed data.");

DEFINE_int64(row_cache_size, 0,
             "Number of bytes to use as a cache of individual rows"
             " (0 = disabled).");

DEFINE_int32(compaction_readahead_size, 0, "Compaction readahead size");

DEFINE_int32(log_readahead_size, 0, "WAL and manifest readahead size");

DEFINE_int32(random_access_max_buffer_size, 1024 * 1024,
             "Maximum windows randomaccess buffer size");

DEFINE_int32(writable_file_max_buffer_size, 1024 * 1024,
             "Maximum write buffer for Writable File");

DEFINE_int32(bloom_bits, -1,
             "Bloom filter bits per key. Negative means use default."
             "Zero disables.");

DEFINE_bool(use_ribbon_filter, false, "Use Ribbon instead of Bloom filter");

DEFINE_double(memtable_bloom_size_ratio, 0,
              "Ratio of memtable size used for bloom filter. 0 means no bloom "
              "filter.");
DEFINE_bool(memtable_whole_key_filtering, false,
            "Try to use whole key bloom filter in memtables.");
DEFINE_bool(memtable_use_huge_page, false,
            "Try to use huge page in memtables.");

DEFINE_bool(use_existing_db, false,
            "If true, do not destroy the existing database.  If you set this "
            "flag and also specify a benchmark that wants a fresh database, "
            "that benchmark will fail.");

DEFINE_bool(use_existing_keys, false,
            "If true, uses existing keys in the DB, "
            "rather than generating new ones. This involves some startup "
            "latency to load all keys into memory. It is supported for the "
            "same read/overwrite benchmarks as `-use_existing_db=true`, which "
            "must also be set for this flag to be enabled. When this flag is "
            "set, the value for `-num` will be ignored.");

DEFINE_bool(show_table_properties, false,
            "If true, then per-level table"
            " properties will be printed on every stats-interval when"
            " stats_interval is set and stats_per_interval is on.");

DEFINE_string(db, "", "Use the db with the following name.");

DEFINE_bool(progress_reports, true,
            "If true, db_bench will report number of finished operations.");

// Read cache flags

DEFINE_string(read_cache_path, "",
              "If not empty string, a read cache will be used in this path");

DEFINE_int64(read_cache_size, 4LL * 1024 * 1024 * 1024,
             "Maximum size of the read cache");

DEFINE_bool(read_cache_direct_write, true,
            "Whether to use Direct IO for writing to the read cache");

DEFINE_bool(read_cache_direct_read, true,
            "Whether to use Direct IO for reading from read cache");

DEFINE_bool(use_keep_filter, false, "Whether to use a noop compaction filter");

static bool ValidateCacheNumshardbits(const char* flagname, int32_t value) {
  if (value >= 20) {
    fprintf(stderr, "Invalid value for --%s: %d, must be < 20\n", flagname,
            value);
    return false;
  }
  return true;
}

DEFINE_bool(verify_checksum, true,
            "Verify checksum for every block read from storage");

DEFINE_bool(statistics, false, "Database statistics");
static class std::shared_ptr<LocalStatistics> dbstats;

DEFINE_int64(writes, -1,
             "Number of write operations to do. If negative, do --num reads.");

DEFINE_bool(finish_after_writes, false,
            "Write thread terminates after all writes are finished");

DEFINE_bool(sync, false, "Sync all writes to disk");

DEFINE_bool(use_fsync, false, "If true, issue fsync instead of fdatasync");

DEFINE_bool(disable_wal, false, "If true, do not write WAL for write.");

DEFINE_bool(manual_wal_flush, false,
            "If true, buffer WAL until buffer is full or a manual FlushWAL().");

DEFINE_string(wal_compression, "none",
              "Algorithm to use for WAL compression. none to disable.");

DEFINE_string(wal_dir, "", "If not empty, use the given dir for WAL");

DEFINE_string(truth_db, "/dev/shm/truth_db/dbbench",
              "Truth key/values used when using verify");

DEFINE_int32(num_levels, 7, "The total number of levels");

DEFINE_bool(level_compaction_dynamic_level_bytes, false,
            "Whether level size base is dynamic");

DEFINE_double(max_bytes_for_level_multiplier, 10,
              "A multiplier to compute max bytes for level-N (N >= 2)");

static std::vector<int> FLAGS_max_bytes_for_level_multiplier_additional_v;
DEFINE_string(max_bytes_for_level_multiplier_additional, "",
              "A vector that specifies additional fanout per level");

static bool ValidateInt32Percent(const char* flagname, int32_t value) {
  if (value <= 0 || value >= 100) {
    fprintf(stderr, "Invalid value for --%s: %d, 0< pct <100 \n", flagname,
            value);
    return false;
  }
  return true;
}
DEFINE_int32(readwritepercent, 90,
             "Ratio of reads to reads/writes (expressed as percentage) for "
             "the ReadRandomWriteRandom workload. The default value 90 means "
             "90% operations out of all reads and writes operations are "
             "reads. In other words, 9 gets for every 1 put.");

DEFINE_int32(mergereadpercent, 70,
             "Ratio of merges to merges&reads (expressed as percentage) for "
             "the ReadRandomMergeRandom workload. The default value 70 means "
             "70% out of all read and merge operations are merges. In other "
             "words, 7 merges for every 3 gets.");

DEFINE_int32(deletepercent, 2,
             "Percentage of deletes out of reads/writes/deletes (used in "
             "RandomWithVerify only). RandomWithVerify "
             "calculates writepercent as (100 - FLAGS_readwritepercent - "
             "deletepercent), so deletepercent must be smaller than (100 - "
             "FLAGS_readwritepercent)");

DEFINE_uint64(delete_obsolete_files_period_micros, 0,
              "Ignored. Left here for backward compatibility");

DEFINE_int64(writes_before_delete_range, 0,
             "Number of writes before DeleteRange is called regularly.");

DEFINE_int64(writes_per_range_tombstone, 0,
             "Number of writes between range tombstones");

DEFINE_int64(range_tombstone_width, 100, "Number of keys in tombstone's range");

DEFINE_int64(max_num_range_tombstones, 0,
             "Maximum number of range tombstones to insert.");

DEFINE_bool(expand_range_tombstones, false,
            "Expand range tombstone into sequential regular tombstones.");

// Transactions Options
DEFINE_bool(optimistic_transaction_db, false,
            "Open a OptimisticTransactionDB instance. "
            "Required for randomtransaction benchmark.");

DEFINE_bool(transaction_db, false,
            "Open a TransactionDB instance. "
            "Required for randomtransaction benchmark.");

DEFINE_uint64(transaction_sets, 2,
              "Number of keys each transaction will "
              "modify (use in RandomTransaction only).  Max: 9999");

DEFINE_bool(transaction_set_snapshot, false,
            "Setting to true will have each transaction call SetSnapshot()"
            " upon creation.");

DEFINE_int32(transaction_sleep, 0,
             "Max microseconds to sleep in between "
             "reading and writing a value (used in RandomTransaction only). ");

DEFINE_uint64(transaction_lock_timeout, 100,
              "If using a transaction_db, specifies the lock wait timeout in"
              " milliseconds before failing a transaction waiting on a lock");
DEFINE_string(
    options_file, "",
    "The path to a RocksDB options file.  If specified, then db_bench will "
    "run with the RocksDB options in the default column family of the "
    "specified options file. "
    "Note that with this setting, db_bench will ONLY accept the following "
    "RocksDB options related command-line arguments, all other arguments "
    "that are related to RocksDB options will be ignored:\n"
    "\t--use_existing_db\n"
    "\t--use_existing_keys\n"
    "\t--statistics\n"
    "\t--row_cache_size\n"
    "\t--row_cache_numshardbits\n"
    "\t--enable_io_prio\n"
    "\t--dump_malloc_stats\n"
    "\t--num_multi_db\n");

// FIFO Compaction Options
DEFINE_uint64(fifo_compaction_max_table_files_size_mb, 0,
              "The limit of total table file sizes to trigger FIFO compaction");

DEFINE_bool(fifo_compaction_allow_compaction, true,
            "Allow compaction in FIFO compaction.");

DEFINE_uint64(fifo_compaction_ttl, 0, "TTL for the SST Files in seconds.");

DEFINE_uint64(fifo_age_for_warm, 0, "age_for_warm for FIFO compaction.");

DEFINE_bool(report_bg_io_stats, false,
            "Measure times spents on I/Os while in compactions. ");

DEFINE_bool(use_stderr_info_logger, false,
            "Write info logs to stderr instead of to LOG file. ");


DEFINE_string(trace_file, "", "Trace workload to a file. ");

DEFINE_double(trace_replay_fast_forward, 1.0,
              "Fast forward trace replay, must > 0.0.");
DEFINE_int32(block_cache_trace_sampling_frequency, 1,
             "Block cache trace sampling frequency, termed s. It uses spatial "
             "downsampling and samples accesses to one out of s blocks.");
DEFINE_int64(
    block_cache_trace_max_trace_file_size_in_bytes,
    uint64_t{64} * 1024 * 1024 * 1024,
    "The maximum block cache trace file size in bytes. Block cache accesses "
    "will not be logged if the trace file size exceeds this threshold. Default "
    "is 64 GB.");
DEFINE_string(block_cache_trace_file, "", "Block cache trace file path.");
DEFINE_int32(trace_replay_threads, 1,
             "The number of threads to replay, must >=1.");

DEFINE_bool(io_uring_enabled, true,
            "If true, enable the use of IO uring if the platform supports it");
extern "C" bool RocksDbIOUringEnable() { return FLAGS_io_uring_enabled; }

DEFINE_bool(adaptive_readahead, false,
            "carry forward internal auto readahead size from one file to next "
            "file at each level during iteration");

DEFINE_bool(rate_limit_user_ops, false,
            "When true use Env::IO_USER priority level to charge internal rate "
            "limiter for reads associated with user operations.");

DEFINE_bool(file_checksum, false,
            "When true use FileChecksumGenCrc32cFactory for "
            "file_checksum_gen_factory.");

DEFINE_bool(rate_limit_auto_wal_flush, false,
            "When true use Env::IO_USER priority level to charge internal rate "
            "limiter for automatic WAL flush (`Options::manual_wal_flush` == "
            "false) after the user write operation.");

DEFINE_bool(async_io, false,
            "When set true, RocksDB does asynchronous reads for internal auto "
            "readahead prefetching.");

DEFINE_bool(optimize_multiget_for_io, true,
            "When set true, RocksDB does asynchronous reads for SST files in "
            "multiple levels for MultiGet.");

DEFINE_bool(charge_compression_dictionary_building_buffer, false,
            "Setting for "
            "CacheEntryRoleOptions::charged of "
            "CacheEntryRole::kCompressionDictionaryBuildingBuffer");

DEFINE_bool(charge_filter_construction, false,
            "Setting for "
            "CacheEntryRoleOptions::charged of "
            "CacheEntryRole::kFilterConstruction");

DEFINE_bool(charge_table_reader, false,
            "Setting for "
            "CacheEntryRoleOptions::charged of "
            "CacheEntryRole::kBlockBasedTableReader");

DEFINE_bool(charge_file_metadata, false,
            "Setting for "
            "CacheEntryRoleOptions::charged of "
            "CacheEntryRole::kFileMetadata");

DEFINE_bool(charge_blob_cache, false,
            "Setting for "
            "CacheEntryRoleOptions::charged of "
            "CacheEntryRole::kBlobCache");

DEFINE_uint64(backup_rate_limit, 0ull,
              "If non-zero, db_bench will rate limit reads and writes for DB "
              "backup. This "
              "is the global rate in ops/second.");

DEFINE_uint64(restore_rate_limit, 0ull,
              "If non-zero, db_bench will rate limit reads and writes for DB "
              "restore. This "
              "is the global rate in ops/second.");

DEFINE_string(backup_dir, "",
              "If not empty string, use the given dir for backup.");

DEFINE_string(restore_dir, "",
              "If not empty string, use the given dir for restore.");

DEFINE_int32(table_cache_numshardbits, 4, "");

DEFINE_string(env_uri, "",
              "URI for registry Env lookup. Mutually exclusive with --fs_uri");
DEFINE_string(fs_uri, "",
              "URI for registry Filesystem lookup. Mutually exclusive"
              " with --env_uri."
              " Creates a default environment with the specified filesystem.");
DEFINE_string(simulate_hybrid_fs_file, "",
              "File for Store Metadata for Simulate hybrid FS. Empty means "
              "disable the feature. Now, if it is set, last_level_temperature "
              "is set to kWarm.");
DEFINE_int32(simulate_hybrid_hdd_multipliers, 1,
             "In simulate_hybrid_fs_file or simulate_hdd mode, how many HDDs "
             "are simulated.");
DEFINE_bool(simulate_hdd, false, "Simulate read/write latency on HDD.");

DEFINE_int64(
    preclude_last_level_data_seconds, 0,
    "Preclude the latest data from the last level. (Used for tiered storage)");

DEFINE_int64(preserve_internal_time_seconds, 0,
             "Preserve the internal time information which stores with SST.");

DEFINE_int64(stats_interval, 0,
             "Stats are reported every N operations when this is greater than "
             "zero. When 0 the interval grows over time.");

DEFINE_int64(stats_interval_seconds, 0,
             "Report stats every N seconds. This overrides stats_interval when"
             " both are > 0.");

DEFINE_int32(stats_per_interval, 0,
             "Reports additional stats per interval when this is greater than "
             "0.");

DEFINE_uint64(slow_usecs, 1000000,
              "A message is printed for operations that take at least this "
              "many microseconds.");

DEFINE_int64(report_interval_seconds, 0,
             "If greater than zero, it will write simple stats in CSV format "
             "to --report_file every N seconds");

DEFINE_string(report_file, "report.csv",
              "Filename where some simple stats are reported to (if "
              "--report_interval_seconds is bigger than 0)");

DEFINE_int32(thread_status_per_interval, 0,
             "Takes and report a snapshot of the current status of each thread"
             " when this is greater than 0.");

DEFINE_uint64(soft_pending_compaction_bytes_limit, 64ull * 1024 * 1024 * 1024,
              "Slowdown writes if pending compaction bytes exceed this number");

DEFINE_uint64(hard_pending_compaction_bytes_limit, 128ull * 1024 * 1024 * 1024,
              "Stop writes if pending compaction bytes exceed this number");

DEFINE_uint64(delayed_write_rate, 8388608u,
              "Limited bytes allowed to DB when soft_rate_limit or "
              "level0_slowdown_writes_trigger triggers");

DEFINE_bool(enable_pipelined_write, true,
            "Allow WAL and memtable writes to be pipelined");

DEFINE_bool(
    unordered_write, false,
    "Enable the unordered write feature, which provides higher throughput but "
    "relaxes the guarantees around atomic reads and immutable snapshots");

DEFINE_bool(allow_concurrent_memtable_write, true,
            "Allow multi-writers to update mem tables in parallel.");

DEFINE_double(experimental_mempurge_threshold, 0.0,
              "Maximum useful payload ratio estimate that triggers a mempurge "
              "(memtable garbage collection).");

DEFINE_bool(enable_write_thread_adaptive_yield, true,
            "Use a yielding spin loop for brief writer thread waits.");

DEFINE_uint64(
    write_thread_max_yield_usec, 100,
    "Maximum microseconds for enable_write_thread_adaptive_yield operation.");

DEFINE_uint64(write_thread_slow_yield_usec, 3,
              "The threshold at which a slow yield is considered a signal that "
              "other processes or threads want the core.");

DEFINE_uint64(rate_limiter_bytes_per_sec, 0, "Set options.rate_limiter value.");

DEFINE_int64(rate_limiter_refill_period_us, 100 * 1000,
             "Set refill period on rate limiter.");

DEFINE_bool(rate_limiter_auto_tuned, false,
            "Enable dynamic adjustment of rate limit according to demand for "
            "background I/O");

DEFINE_bool(sine_write_rate, false, "Use a sine wave write_rate_limit");

DEFINE_uint64(
    sine_write_rate_interval_milliseconds, 10000,
    "Interval of which the sine wave write_rate_limit is recalculated");

DEFINE_double(sine_a, 1, "A in f(x) = A sin(bx + c) + d");

DEFINE_double(sine_b, 1, "B in f(x) = A sin(bx + c) + d");

DEFINE_double(sine_c, 0, "C in f(x) = A sin(bx + c) + d");

DEFINE_double(sine_d, 1, "D in f(x) = A sin(bx + c) + d");

DEFINE_bool(rate_limit_bg_reads, false,
            "Use options.rate_limiter on compaction reads");

DEFINE_uint64(
    benchmark_write_rate_limit, 0,
    "If non-zero, db_bench will rate-limit the writes going into RocksDB. This "
    "is the global rate in bytes/second.");

// the parameters of mix_graph
DEFINE_double(keyrange_dist_a, 0.0,
              "The parameter 'a' of prefix average access distribution "
              "f(x)=a*exp(b*x)+c*exp(d*x)");
DEFINE_double(keyrange_dist_b, 0.0,
              "The parameter 'b' of prefix average access distribution "
              "f(x)=a*exp(b*x)+c*exp(d*x)");
DEFINE_double(keyrange_dist_c, 0.0,
              "The parameter 'c' of prefix average access distribution"
              "f(x)=a*exp(b*x)+c*exp(d*x)");
DEFINE_double(keyrange_dist_d, 0.0,
              "The parameter 'd' of prefix average access distribution"
              "f(x)=a*exp(b*x)+c*exp(d*x)");
DEFINE_int64(keyrange_num, 1,
             "The number of key ranges that are in the same prefix "
             "group, each prefix range will have its key access distribution");
DEFINE_double(key_dist_a, 0.0,
              "The parameter 'a' of key access distribution model f(x)=a*x^b");
DEFINE_double(key_dist_b, 0.0,
              "The parameter 'b' of key access distribution model f(x)=a*x^b");
DEFINE_double(value_theta, 0.0,
              "The parameter 'theta' of Generized Pareto Distribution "
              "f(x)=(1/sigma)*(1+k*(x-theta)/sigma)^-(1/k+1)");
// Use reasonable defaults based on the mixgraph paper
DEFINE_double(value_k, 0.2615,
              "The parameter 'k' of Generized Pareto Distribution "
              "f(x)=(1/sigma)*(1+k*(x-theta)/sigma)^-(1/k+1)");
// Use reasonable defaults based on the mixgraph paper
DEFINE_double(value_sigma, 25.45,
              "The parameter 'theta' of Generized Pareto Distribution "
              "f(x)=(1/sigma)*(1+k*(x-theta)/sigma)^-(1/k+1)");
DEFINE_double(iter_theta, 0.0,
              "The parameter 'theta' of Generized Pareto Distribution "
              "f(x)=(1/sigma)*(1+k*(x-theta)/sigma)^-(1/k+1)");
// Use reasonable defaults based on the mixgraph paper
DEFINE_double(iter_k, 2.517,
              "The parameter 'k' of Generized Pareto Distribution "
              "f(x)=(1/sigma)*(1+k*(x-theta)/sigma)^-(1/k+1)");
// Use reasonable defaults based on the mixgraph paper
DEFINE_double(iter_sigma, 14.236,
              "The parameter 'sigma' of Generized Pareto Distribution "
              "f(x)=(1/sigma)*(1+k*(x-theta)/sigma)^-(1/k+1)");
DEFINE_double(mix_get_ratio, 1.0,
              "The ratio of Get queries of mix_graph workload");
DEFINE_double(mix_put_ratio, 0.0,
              "The ratio of Put queries of mix_graph workload");
DEFINE_double(mix_seek_ratio, 0.0,
              "The ratio of Seek queries of mix_graph workload");
DEFINE_int64(mix_max_scan_len, 10000, "The max scan length of Iterator");
DEFINE_int64(mix_max_value_size, 1024, "The max value size of this workload");
DEFINE_double(
    sine_mix_rate_noise, 0.0,
    "Add the noise ratio to the sine rate, it is between 0.0 and 1.0");
DEFINE_bool(sine_mix_rate, false,
            "Enable the sine QPS control on the mix workload");
DEFINE_uint64(
    sine_mix_rate_interval_milliseconds, 10000,
    "Interval of which the sine wave read_rate_limit is recalculated");
DEFINE_int64(mix_accesses, -1,
             "The total query accesses of mix_graph workload");

DEFINE_uint64(
    benchmark_read_rate_limit, 0,
    "If non-zero, db_bench will rate-limit the reads from RocksDB. This "
    "is the global rate in ops/second.");

DEFINE_bool(readonly, false, "Run read only benchmarks.");

DEFINE_bool(print_malloc_stats, false,
            "Print malloc stats to stdout after benchmarks finish.");

DEFINE_bool(disable_auto_compactions, false, "Do not auto trigger compactions");

DEFINE_uint64(wal_ttl_seconds, 0, "Set the TTL for the WAL Files in seconds.");
DEFINE_uint64(wal_size_limit_MB, 0,
              "Set the size limit for the WAL Files in MB.");
DEFINE_uint64(max_total_wal_size, 0, "Set total max WAL size");

DEFINE_bool(use_single_deletes, true,
            "Use single deletes (used in RandomReplaceKeys only).");

DEFINE_double(stddev, 2000.0,
              "Standard deviation of normal distribution used for picking keys"
              " (used in RandomReplaceKeys only).");

DEFINE_int32(key_id_range, 100000,
             "Range of possible value of key id (used in TimeSeries only).");

DEFINE_string(expire_style, "none",
              "Style to remove expired time entries. Can be one of the options "
              "below: none (do not expired data), compaction_filter (use a "
              "compaction filter to remove expired data), delete (seek IDs and "
              "remove expired data) (used in TimeSeries only).");

DEFINE_uint64(
    time_range, 100000,
    "Range of timestamp that store in the database (used in TimeSeries"
    " only).");

DEFINE_int32(num_deletion_threads, 1,
             "Number of threads to do deletion (used in TimeSeries and delete "
             "expire_style only).");

DEFINE_int32(max_successive_merges, 0,
             "Maximum number of successive merge operations on a key in the "
             "memtable");

DEFINE_int32(prefix_size, 0,
             "control the prefix size for HashSkipList and plain table");
DEFINE_int64(keys_per_prefix, 0,
             "control average number of keys generated per prefix, 0 means no "
             "special handling of the prefix, i.e. use the prefix comes with "
             "the generated random number.");
DEFINE_bool(total_order_seek, false,
            "Enable total order seek regardless of index format.");
DEFINE_bool(prefix_same_as_start, false,
            "Enforce iterator to return keys with prefix same as seek key.");
DEFINE_bool(
    seek_missing_prefix, false,
    "Iterator seek to keys with non-exist prefixes. Require prefix_size > 8");

DEFINE_int32(memtable_insert_with_hint_prefix_size, 0,
             "If non-zero, enable "
             "memtable insert with hint with the given prefix size.");
DEFINE_bool(enable_io_prio, false,
            "Lower the background flush/compaction threads' IO priority");
DEFINE_bool(enable_cpu_prio, false,
            "Lower the background flush/compaction threads' CPU priority");
DEFINE_bool(identity_as_first_hash, false,
            "the first hash function of cuckoo table becomes an identity "
            "function. This is only valid when key is 8 bytes");
DEFINE_bool(dump_malloc_stats, true, "Dump malloc stats in LOG ");
DEFINE_int64(multiread_stride, 0,
             "Stride length for the keys in a MultiGet batch");
DEFINE_bool(multiread_batched, false, "Use the new MultiGet API");

DEFINE_string(memtablerep, "skip_list", "");
DEFINE_int64(hash_bucket_count, 1024 * 1024, "hash bucket count");
DEFINE_bool(use_plain_table, false,
            "if use plain table instead of block-based table format");
DEFINE_bool(use_cuckoo_table, false, "if use cuckoo table format");
DEFINE_double(cuckoo_hash_ratio, 0.9, "Hash ratio for Cuckoo SST table.");
DEFINE_bool(use_hash_search, false,
            "if use kHashSearch instead of kBinarySearch. "
            "This is valid if only we use BlockTable");
DEFINE_string(merge_operator, "",
              "The merge operator to use with the database."
              "If a new merge operator is specified, be sure to use fresh"
              " database The possible merge operators are defined in"
              " utilities/merge_operators.h");
DEFINE_int32(skip_list_lookahead, 0,
             "Used with skip_list memtablerep; try linear search first for "
             "this many steps from the previous position");
DEFINE_bool(report_file_operations, false,
            "if report number of file operations");
DEFINE_bool(report_open_timing, false, "if report open timing");
DEFINE_int32(readahead_size, 0, "Iterator readahead size");

DEFINE_bool(read_with_latest_user_timestamp, true,
            "If true, always use the current latest timestamp for read. If "
            "false, choose a random timestamp from the past.");

DEFINE_int32(disable_seek_compaction, false,
             "Not used, left here for backwards compatibility");

DEFINE_uint32(write_batch_protection_bytes_per_key, 0,
              "Size of per-key-value checksum in each write batch. Currently "
              "only value 0 and 8 are supported.");

DEFINE_uint32(
    memtable_protection_bytes_per_key, 0,
    "Enable memtable per key-value checksum protection. "
    "Each entry in memtable will be suffixed by a per key-value checksum. "
    "This options determines the size of such checksums. "
    "Supported values: 0, 1, 2, 4, 8.");

DEFINE_uint32(block_protection_bytes_per_key, 0,
              "Enable block per key-value checksum protection. "
              "Supported values: 0, 1, 2, 4, 8.");

DEFINE_bool(build_info, false,
            "Print the build info via GetRocksBuildInfoAsString");

DEFINE_bool(track_and_verify_wals_in_manifest, false,
            "If true, enable WAL tracking in the MANIFEST");