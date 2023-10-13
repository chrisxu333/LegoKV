#include "hotring_index.h"
#include <bitset>
#include <iostream>
#include <gtest/gtest.h>
#include <thread>
#include <random>
#include "benchmark/zipfian_int_dist.h"

using HOTRING_NAMESPACE::tag_key_t;
using HOTRING_NAMESPACE::Ring;
using HOTRING_NAMESPACE::HotRingIndex;
using HOTRING_NAMESPACE::head_ptr_t;
using HOTRING_NAMESPACE::item_t;
using HOTRING_NAMESPACE::LocalStatistics;
using HOTRING_NAMESPACE::IndexOptions;

typedef std::pair<tag_key_t, std::string> kvtype_tag;
typedef std::pair<std::string, std::string> kvtype_string;
std::vector<kvtype_tag> kvs;
std::unordered_set<std::string> kvs_string;
std::vector<kvtype_string> kvs_string_real;
int num_keys = 10000;
int num_reader = 10;
int keys_per_reader = num_keys / num_reader;

void GenerateRandomKV() {
  kvs.clear();
  for(int i = 0; i < num_keys; ++i) {
    uint16_t tag = random() % 10;
    std::string key = std::to_string(random() % num_keys);
    auto tag_key = tag_key_t(tag, key);
    std::string value = "value" + key;
    kvs.push_back(std::make_pair(tag_key, value));
  }
}

void GenerateRandomKVString() {
  kvs_string_real.clear();
  for(int i = 0; i < num_keys; ++i) {
    std::string key = std::to_string(random() % num_keys);
    std::string value = "value" + key;
    kvs_string_real.push_back(std::make_pair(key, value));
  }
}

void GenerateRandomKVStringUnique() {
  kvs_string.clear();
  for(int i = 0; i < num_keys; ++i) {
    std::string key = std::to_string(random() % num_keys);
    std::string value = "value" + key;
    kvs_string.insert(key);
  }
}

void ringbase_writer_thread(Ring* ring) {
  for(int i = 0; i < num_keys; ++i) {
    ring->Insert(kvs[i].first, kvs[i].second);
  }
  std::cout << "writer done" << std::endl;
}

void ringbase_reader_thread(Ring* ring, int tid) {
  bool success = false;
  while(!success) {
    bool global_found = true;
    for(int i = tid * keys_per_reader; i < tid * keys_per_reader + keys_per_reader; ++i) {
      std::string value;
      bool found = ring->LookUp(kvs[i].first, value);
      global_found = global_found && found;
      if(found) {
        assert(value == kvs[i].second);
      }
    }
    if(global_found) success = true;
  }
  std::cout << "reader " << tid << " done" << std::endl;
}

void indexbase_writer_thread(HotRingIndex* index) {
  for(int i = 0; i < num_keys; ++i) {
    index->Put((kvs_string_real[i].first), (kvs_string_real[i].second));
  }
  std::cout << "writer done" << std::endl;
}

void indexbase_reader_thread(HotRingIndex* index, int tid) {
  bool success = false;
  while(!success) {
    bool global_found = true;
    for(int i = tid * keys_per_reader; i < tid * keys_per_reader + keys_per_reader; ++i) {
      std::string value;
      Status s = index->Get((kvs_string_real[i].first), value);
      global_found = global_found && (s == Status::OK());
      if(s == Status::OK()) {
        assert(value == kvs_string_real[i].second);
      }
    }
    if(global_found) success = true;
  }
  std::cout << "reader " << tid << " done" << std::endl;
}

TEST(HeaderOp, DISABLED_set_unset_active) {
  int a = 10;
  int* a_ptr = &a;
  head_ptr_t head_ptr(reinterpret_cast<uintptr_t>(a_ptr));
  std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  head_ptr.SetActive();
  std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  head_ptr.UnsetActive();
  std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  ASSERT_EQ(*(reinterpret_cast<int*>(head_ptr.GetRaw())), a);
}

TEST(HeaderOp, DISABLED_update_reset_counter) {
  int a = 10;
  int* a_ptr = &a;
  head_ptr_t head_ptr(reinterpret_cast<uintptr_t>(a_ptr));
  std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  for(int i = 0; i < 10; ++i) {
    head_ptr.UpdateCounter();
    std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  }
  std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  int counter = head_ptr.GetCounter();
  ASSERT_EQ(counter, 10);
  head_ptr.ResetCounter();
  std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  counter = head_ptr.GetCounter();
  ASSERT_EQ(counter, 0);
}

TEST(HeaderOp, DISABLED_setraw_with_metadata) {
  int a = 10;
  int b = 11;
  int* a_ptr = &a;
  head_ptr_t head_ptr(reinterpret_cast<uintptr_t>(a_ptr));
  std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  for(int i = 0; i < 10; ++i) {
    head_ptr.UpdateCounter();
    std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  }
  std::cout << std::bitset<64>(head_ptr.ptr) << std::endl;
  int counter = head_ptr.GetCounter();
  ASSERT_EQ(counter, 10);
  ASSERT_EQ(*(reinterpret_cast<int*>(head_ptr.GetRaw())), a);
  head_ptr.SetRaw(reinterpret_cast<item_t*>(&b));
  counter = head_ptr.GetCounter();
  ASSERT_EQ(counter, 10);
  ASSERT_EQ(*(reinterpret_cast<int*>(head_ptr.GetRaw())), b);
}

TEST(ItemOp, DISABLED_set_unset_rehash_occupied) {
  std::string key = "key";
  std::string value = "value";
  std::atomic<item_t*> atomic_ptr;
  atomic_ptr.store(new item_t(key, value));
  std::cout << "Original: " << std::bitset<64>(reinterpret_cast<uintptr_t>(atomic_ptr.load())) << std::endl;
  SetRehash(atomic_ptr);
  std::cout << "SetRehash: " << std::bitset<64>(reinterpret_cast<uintptr_t>(atomic_ptr.load())) << std::endl;
  ResetRehash(atomic_ptr);
  std::cout << "ResetRehash: " << std::bitset<64>(reinterpret_cast<uintptr_t>(atomic_ptr.load())) << std::endl;
  SetOccupied(atomic_ptr);
  std::cout << "SetOccupied: " << std::bitset<64>(reinterpret_cast<uintptr_t>(atomic_ptr.load())) << std::endl;
  ResetOccupied(atomic_ptr);
  std::cout << "ResetOccupied: " << std::bitset<64>(reinterpret_cast<uintptr_t>(atomic_ptr.load())) << std::endl;
}

TEST(ItemOp, DISABLED_update_reset_counter) {
  std::string key = "key";
  std::string value = "value";
  std::atomic<item_t*> atomic_ptr;
  atomic_ptr.store(new item_t(key, value));
  std::cout << "Original: \t" << std::bitset<64>(reinterpret_cast<uintptr_t>(atomic_ptr.load())) << std::endl;
  for(int i = 0; i < 10; ++i) {
    IncCounter(atomic_ptr);
  }
  std::cout << "Inc10: \t\t" << std::bitset<64>(reinterpret_cast<uintptr_t>(atomic_ptr.load())) << std::endl;
  int counter = GetCounter(atomic_ptr);
  ASSERT_EQ(counter, 10);
  ResetCounter(atomic_ptr);
  std::cout << "Reset: \t\t" << std::bitset<64>(reinterpret_cast<uintptr_t>(atomic_ptr.load())) << std::endl;
  counter = GetCounter(atomic_ptr);
  ASSERT_EQ(counter, 0);
}

TEST(RingOp, DISABLED_insert_lookup) {
  IndexOptions opts_;
  Ring* ring = new Ring(&opts_);
  ring->Insert({8, "20"}, "value1");
  ring->Insert({6, "30"}, "value2");
  ring->Insert({2, "36"}, "value3");
  ring->Insert({3, "25"}, "value4");
  ring->Insert({5, "65"}, "value5");
  ring->Insert({5, "68"}, "value6");

  ring->Show();

  std::string value;
  bool found = ring->LookUp({4, "35"}, value);
  ASSERT_EQ(found, false);
  found = ring->LookUp({5, "68"}, value);
  ASSERT_EQ(value, "value6");
  found = ring->LookUp({1, "45"}, value);
  ASSERT_EQ(found, false);
  found = ring->LookUp({9, "10"}, value);
  ASSERT_EQ(found, false);
}

TEST(RingOp, DISABLED_single_thread_insert_lookup) {
  GenerateRandomKV();
  IndexOptions opts_;
  Ring* ring = new Ring(&opts_);
  for(auto kv : kvs) {
    ring->Insert(kv.first, kv.second);
  }

  for(auto kv : kvs) {
    std::string value;
    bool found = ring->LookUp(kv.first, value);
    ASSERT_EQ(found, true);
    ASSERT_EQ(value, kv.second);
  }
}

TEST(RingOpMultithread, DISABLED_multi_thread_insert_lookup) {
  GenerateRandomKV();
  IndexOptions opts_;
  Ring* ring = new Ring(&opts_);
  // multiple thread for reading
  std::vector<std::thread> readers(num_reader);
  for(int i = 0; i < num_reader; ++i) {
    readers[i] = std::thread(ringbase_reader_thread, ring, i);
  }

  // one thread for writing
  std::thread writer(ringbase_writer_thread, ring);
  writer.join();

  for(int i = 0; i < num_reader; ++i) {
    readers[i].join();
  }
}

TEST(HotRingIndex, put_get_single_value_out_of_scope) {
  HotRingIndex hr_index(IndexOptions{});
  Status s;
  std::string key = "key";
  {
    std::string data = "123123123";
    hr_index.Put((key), (data));
  }

  {
    std::string data;
    s = hr_index.Get((key), data);
    ASSERT_EQ(Status::OK(), s);
    ASSERT_EQ(data, "123123123");
  }
}

TEST(HotRingIndex, DISABLED_put_get_unique_value) {
  HotRingIndex hr_index(IndexOptions{});
  Status s;
  GenerateRandomKVStringUnique();
  for(auto it = kvs_string.begin(); it != kvs_string.end(); ++it) {
    s = hr_index.Put((*it), ("value" + (*it)));
    ASSERT_EQ(Status::OK(), s);
    std::string value;
    s = hr_index.Get((*it), value);
    ASSERT_EQ(Status::OK(), s);
    ASSERT_EQ(value, "value" + (*it));
  }
}

TEST(HotRingIndex, DISABLED_put_get_duplicate_value) {
  HotRingIndex hr_index(IndexOptions{});
  Status s;
  for(int i = 0; i < (num_keys / 10); ++i) {
    for(int j = 0; j < 10; ++j) {
      s = hr_index.Put(("key" + std::to_string(i)), ("value" + std::to_string(i)));
      ASSERT_EQ(Status::OK(), s);
      std::string value;
      s = hr_index.Get(("key" + std::to_string(i)), value);
      ASSERT_EQ(Status::OK(), s);
      ASSERT_EQ(value, "value" + std::to_string(i));
    }
  }
}

TEST(HotRingIndex, DISABLED_put_get_real_workload) {
  IndexOptions options;
  options.bucket_size = 100;
  LocalStatistics* stat = new LocalStatistics();
  HotRingIndex hr_index(options, stat);
  Status s;
  GenerateRandomKVString();
  for(int i = 0; i < num_keys; ++i) {
    s = hr_index.Put((kvs_string_real[i].first), (kvs_string_real[i].second));
    ASSERT_EQ(Status::OK(), s);
    std::string value;
    s = hr_index.Get((kvs_string_real[i].first), value);
    ASSERT_EQ(Status::OK(), s);
    ASSERT_EQ(value, kvs_string_real[i].second);
  }
  ShowStat(stat);
  delete stat;
}

TEST(HotRingIndex, DISABLED_put_with_samping_shift) {
  IndexOptions options;
  options.bucket_size = 10;
  options.use_sampling_shift = false;
  options.R = 5;
  LocalStatistics* stat = new LocalStatistics();
  HotRingIndex hr_index(options, stat);
  Status s;
  GenerateRandomKVString();
  for(int i = 0; i < num_keys; ++i) {
    s = hr_index.Put((kvs_string_real[i].first), (kvs_string_real[i].second));
    ASSERT_EQ(Status::OK(), s);
  }

  // read as a zipfian fashion
  std::default_random_engine generator;
  zipfian_int_distribution<int> distribution(4, num_keys, 0.99);
  for(int i = 0; i < num_keys; ++i) {
    int idx = distribution(generator);
    std::cout << "access key: " << kvs_string_real[idx].first << std::endl;
    std::string value;
    s = hr_index.Get((kvs_string_real[idx].first), value);
    ASSERT_EQ(Status::OK(), s);
    ASSERT_EQ(value, kvs_string_real[idx].second);
  }
  ShowStat(stat);
  delete stat;
}

TEST(HotRingIndex, DISABLED_put_get_real_workload_multi_thread) {
  // LocalStatistics* stat = new LocalStatistics();
  IndexOptions options;
  HotRingIndex* hr_index = new HotRingIndex(options);
  Status s;
  GenerateRandomKVString();
  // multiple thread for reading
  std::vector<std::thread> readers(num_reader);
  for(int i = 0; i < num_reader; ++i) {
    readers[i] = std::thread(indexbase_reader_thread, hr_index, i);
  }

  // one thread for writing
  std::thread writer(indexbase_writer_thread, hr_index);
  writer.join();

  for(int i = 0; i < num_reader; ++i) {
    readers[i].join();
  }
  delete hr_index;
}

int main(int argc, char** argv) {
  LEGOKV_NAMESPACE::port::InstallStackTraceHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}