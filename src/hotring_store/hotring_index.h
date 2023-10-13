#include "legokv/status.h"
#include "legokv/slice.h"
#include "legokv/options.h"
#include "port/stack_trace.h"
#include "local_statistics.h"
#include <iostream>
#include <bitset>
#include <inttypes.h>
#include <atomic>
#include <assert.h>
#include <memory>
#include "index_options.h"
#include "hotring_namespace.h"

using LEGOKV_NAMESPACE::Status;
using LEGOKV_NAMESPACE::Slice;
using LEGOKV_NAMESPACE::Options;

// HotRing is a pure-memory KV store from Alibaba Tair Team
// No persistency needed, could add it later on.

namespace HOTRING_NAMESPACE{

struct tag_key_t {
  uint32_t tag;
  std::string key;
  tag_key_t(uint16_t t, std::string k) : tag(t), key(k) {}
  bool operator<(const tag_key_t& other) const {
    if(tag < other.tag || ((tag == other.tag) && (key < other.key))) return true;
    return false;
  }
  bool operator==(const tag_key_t& other) const {
    return (tag == other.tag) && (key == other.key);
  }
  bool operator>(const tag_key_t& other) const {
    return !(*this <= other);
  }
  bool operator<=(const tag_key_t& other) const {
    return ((*this) < other) || ((*this) == other);
  }
  friend std::ostream& operator<<(std::ostream & os,const tag_key_t& tag_key);
};

std::ostream& operator<<(std::ostream & os,const tag_key_t& tag_key) {
  os << "[" << tag_key.tag << "," << tag_key.key << "]";
  return os;
}

struct item_t {
  uint16_t tag;
  std::atomic<item_t*> next_item_ptr;
  // item_ptr_t next_item_ptr;
  std::string key;
  std::string value;
  item_t(const std::string& k, const std::string& v) : 
    tag(0), next_item_ptr(nullptr), key(k), value(v) {} 
  item_t(const tag_key_t k, const std::string& v) : 
    tag(k.tag), next_item_ptr(nullptr), key(k.key), value(v) {} 
  tag_key_t Key() {
    tag_key_t tag_key(tag, key);
    return tag_key;
  }
  item_t& operator*() const {
    std::cout << "access *" << std::endl;
    return *(reinterpret_cast<item_t*>(reinterpret_cast<uintptr_t>(this) & 0xffffffffffff));
  }
  item_t* operator->() const {
    std::cout << "access ->" << std::endl;
    return reinterpret_cast<item_t*>(reinterpret_cast<uintptr_t>(this) & 0xffffffffffff);
  }
  item_t* GetRaw() const {
    return reinterpret_cast<item_t*>(reinterpret_cast<uintptr_t>(this) & 0xffffffffffff);
  }
};

inline item_t* GetRaw(std::atomic<item_t*>& ptr) {
  return reinterpret_cast<item_t*>(reinterpret_cast<uintptr_t>(ptr.load()) & 0xffffffffffff);
}

// inline void SetRaw(std::atomic<item_t*>& ptr, item_t* new_ptr) {
//   ptr.store((ptr.load() & ((uint64_t)65535 << 48)) | (reinterpret_cast<uintptr_t>(raw_ptr) & 0xffffffffffff));
// }

inline void SetRehash(std::atomic<item_t*>& ptr) {
  auto raw_ptr = ptr.load();
  uintptr_t new_ptr = reinterpret_cast<uintptr_t>(raw_ptr) | ((uintptr_t)1 << 63);
  while(!ptr.compare_exchange_strong(raw_ptr, reinterpret_cast<item_t*>(new_ptr))) {
    raw_ptr = ptr.load();
    new_ptr = reinterpret_cast<uintptr_t>(raw_ptr) | ((uintptr_t)1 << 63);
  }
}
inline void ResetRehash(std::atomic<item_t*>& ptr) {
  auto raw_ptr = ptr.load();
  uintptr_t new_ptr = reinterpret_cast<uintptr_t>(raw_ptr) & (~((uintptr_t)1 << 63));
  while(!ptr.compare_exchange_strong(raw_ptr, reinterpret_cast<item_t*>(new_ptr))) {
    raw_ptr = ptr.load();
    new_ptr = reinterpret_cast<uintptr_t>(raw_ptr) & (~((uintptr_t)1 << 63));
  }
}
void SetOccupied(std::atomic<item_t*>& ptr) {
  auto raw_ptr = ptr.load();
  uintptr_t new_ptr = reinterpret_cast<uintptr_t>(raw_ptr) | ((uintptr_t)1 << 62);
  while(!ptr.compare_exchange_strong(raw_ptr, reinterpret_cast<item_t*>(new_ptr))) {
    raw_ptr = ptr.load();
    new_ptr = reinterpret_cast<uintptr_t>(raw_ptr) | ((uintptr_t)1 << 62);
  }
}
void ResetOccupied(std::atomic<item_t*>& ptr) {
  auto raw_ptr = ptr.load();
  uintptr_t new_ptr = reinterpret_cast<uintptr_t>(raw_ptr) & (~((uintptr_t)1 << 62));
  while(!ptr.compare_exchange_strong(raw_ptr, reinterpret_cast<item_t*>(new_ptr))) {
    raw_ptr = ptr.load();
    new_ptr = reinterpret_cast<uintptr_t>(raw_ptr) & (~((uintptr_t)1 << 62));
  }
}
void IncCounter(std::atomic<item_t*>& ptr) {
  uint64_t bit_mask = (uint64_t)16383 << 48;
  item_t* raw_ptr;
  item_t* new_ptr;
  do{
    raw_ptr = ptr.load();
    uint16_t cnt = (((1 << 14) - 1) & (reinterpret_cast<uintptr_t>(raw_ptr) >> 48));
    ++cnt;
    new_ptr = reinterpret_cast<item_t*>((reinterpret_cast<uintptr_t>(raw_ptr) & (~bit_mask)) | ((uint64_t)cnt << 48));
  } while(!ptr.compare_exchange_strong(raw_ptr, new_ptr));
}
void ResetCounter(std::atomic<item_t*>& ptr) {
  uintptr_t range = 0xffffffffffffffff;
  range &= ~((((uintptr_t)1 << 48) - 1) ^ (((uintptr_t)1 << 62) - 1));
  item_t* raw_ptr;
  item_t* new_ptr;
  do{
      raw_ptr = ptr.load();
      new_ptr = reinterpret_cast<item_t*>(reinterpret_cast<uintptr_t>(raw_ptr) & range);
  } while (!ptr.compare_exchange_strong(raw_ptr, new_ptr));
}
inline uint16_t GetCounter(std::atomic<item_t*>& ptr) {
  return static_cast<uint16_t>((((1 << 14) - 1) & (reinterpret_cast<uintptr_t>(ptr.load()) >> 48)));
}
inline uint16_t GetCounterWeak(item_t* ptr) {
  return static_cast<uint16_t>((((1 << 14) - 1) & (reinterpret_cast<uintptr_t>(ptr) >> 48)));
}

struct head_ptr_t {
  std::atomic<uintptr_t> ptr;
  head_ptr_t() : ptr(0) {}
  head_ptr_t(uintptr_t raw_ptr) : ptr(raw_ptr){}

  bool Empty() { return ptr.load() == 0; }

  inline void SetRaw(item_t* raw_ptr) {
    ptr.store((ptr.load() & ((uint64_t)65535 << 48)) | (reinterpret_cast<uintptr_t>(raw_ptr) & 0xffffffffffff));
  }
  inline item_t* GetRaw() {
    return reinterpret_cast<item_t*>(ptr.load() & 0xffffffffffff);
  }

  inline void SetActive() { ptr |= ((uintptr_t)1 << 63); }
  inline void UnsetActive() { ptr &= ~((uintptr_t)1 << 63); }
  
  void UpdateCounter() {
    int cnt = (((1 << 15) - 1) & (ptr >> 48));
    ++cnt;
    uint64_t bit_mask = (uint64_t)32767 << 48;
    ptr.store((ptr.load() & (~bit_mask)) | ((uint64_t)cnt << 48));
  }
  void ResetCounter() {
    uintptr_t range = 0xffffffffffffffff;
    range &= ~((((uintptr_t)1 << 48) - 1) ^ (((uintptr_t)1 << 63) - 1));
    ptr.store(ptr.load() & range);
  }
  uint16_t GetCounter() {
    return static_cast<uint16_t>(((1 << 15) - 1) & (ptr.load() >> 48));
  }
};

class Ring {
public:
  Ring(IndexOptions* options, LocalStatistics* stat = nullptr) : opts_(options), stat_(stat) {
    head_ptr = new head_ptr_t();
  }

  ~Ring() {
    if(head_ptr->Empty()) {
      delete head_ptr->GetRaw();
      return;
    }
    item_t* dummy = head_ptr->GetRaw();
    dummy = dummy->next_item_ptr.load()->GetRaw();
    while(dummy != head_ptr->GetRaw()) {
      item_t* tmp = dummy->next_item_ptr.load()->GetRaw();
      delete dummy;
      dummy = tmp;
    }
    delete head_ptr->GetRaw();
  }

  // Requires lock-free guard.
  // class RingIterator {
  //   public:
  //     RingIterator() : cur_item_ptr(nullptr), prev_item_ptr(nullptr) {}
  //     RingIterator(item_t* ptr) : prev_item_ptr(nullptr) {
  //       cur_item_ptr = ptr;
  //     }
  //     tag_key_t Key() {
  //       return cur_item_ptr->Key();
  //     }
  //     bool Valid() {
  //       return cur_item_ptr != nullptr;
  //     }
  //     void Next() {
  //       if(prev_item_ptr == nullptr) prev_item_ptr = cur_item_ptr;
  //       else prev_item_ptr = prev_item_ptr->next_item_ptr;
  //       cur_item_ptr = cur_item_ptr->next_item_ptr;
  //     }
  //     item_t* GetPrev() {
  //       return prev_item_ptr;
  //     }
  //     item_t* GetCur() {
  //       return cur_item_ptr;
  //     }
  //   private:
  //     item_t* cur_item_ptr;
  //     item_t* prev_item_ptr;
  // };

  class RingIterator {
    public:
      RingIterator() : cur_item_ptr(0), prev_item_ptr(0) {}
      RingIterator(item_t* ptr){
        cur_item_ptr = ptr;
        prev_item_ptr = ptr;
        while (prev_item_ptr->GetRaw()->next_item_ptr.load()->GetRaw() != cur_item_ptr->GetRaw()) {
          prev_item_ptr = prev_item_ptr->GetRaw()->next_item_ptr.load();
        }
        cur_item_ptr = prev_item_ptr->GetRaw()->next_item_ptr.load();
      }
      tag_key_t Key() {
        return cur_item_ptr->GetRaw()->Key();
      }
      bool Valid() {
        return cur_item_ptr != nullptr;
      }
      void Next() {
        // if(prev_item_ptr == nullptr) prev_item_ptr = cur_item_ptr;
        // else prev_item_ptr = prev_item_ptr->GetRaw()->next_item_ptr.load();
        prev_item_ptr = prev_item_ptr->GetRaw()->next_item_ptr.load();
        cur_item_ptr = cur_item_ptr->GetRaw()->next_item_ptr.load();
      }
      item_t* GetPrev() {
        return prev_item_ptr;
      }
      item_t* GetCur() {
        return cur_item_ptr;
      }
    private:
      item_t* cur_item_ptr;
      item_t* prev_item_ptr;
  };

  bool LookUp(const tag_key_t key, std::string& value) {
    if(head_ptr->Empty()) {
      return false;
    }
    RecordTick(stat_, HOTRING_LOOKUP_TIME);
    assert(head_ptr->GetRaw() != nullptr);

    // if during a sampling process, track record
    if(opts_->use_sampling_shift) {
      head_ptr->UpdateCounter();
    }

    RingIterator iter(head_ptr->GetRaw());
    for(;;iter.Next()) {
      if(iter.Key() == key) {
        value = iter.GetCur()->GetRaw()->value;
        if(opts_->use_random_shift) {
          if(request_count_.fetch_add(1) == opts_->R && head_ptr->GetRaw() != iter.GetCur()) {
            // trigger a random head_ptr shift
            RecordTick(stat_, HOTRING_RANDOM_HEAD_SHIFT);
            head_ptr->SetRaw(iter.GetCur());
            request_count_.store(0);
          }
        } else if(opts_->use_sampling_shift) {
        #ifdef DEBUG
          std::cout << "trigger sampling" << std::endl;
        #endif
          IncCounter(iter.GetPrev()->GetRaw()->next_item_ptr);
          if(request_count_.fetch_add(1) == opts_->R) {
            RecordTick(stat_, HOTRING_SAMPLING_HEAD_SHIFT);
            // trigger a sampling shift
            SamplingShift();    
            request_count_.store(0);
          }
        }
        return true;
      }
      auto prev = iter.GetPrev()->GetRaw();
      auto cur = iter.GetCur()->GetRaw();
      if(prev == nullptr) { // list only has one item
        continue;
      }
      if((prev->Key() < key) && (key < cur->Key())) return false;
      if((key < cur->Key()) && (cur->Key() < prev->Key())) return false;
      if((cur->Key() < prev->Key()) && (prev->Key() < key)) return false;
      RecordTick(stat_, HOTRING_NEXT_TIME);
    }
    return false;
  }
  
  // The insertion has to maintain the ordered property of the ring.
  void Insert(const tag_key_t key, const std::string& value) {
    bool success = false;

    if(head_ptr->Empty()) {
      item_t* item = new item_t(key, value);
      head_ptr->SetRaw(item);
      assert(head_ptr->GetRaw() != nullptr);
      item->next_item_ptr = item;
      return;
    }

    while(!success) {
      item_t* target = FindLessThan(key);
      // simply return on duplicated key, will do update in the future.
      if(target->Key() == key) {
        return;
      }
      item_t* item = new item_t(key, value);
      assert(item != nullptr);
      item_t* next = target->next_item_ptr.load();
      assert(target != nullptr);
      item->next_item_ptr.store(next);
      if(!target->next_item_ptr.compare_exchange_strong(next, item)) {
        delete item;
      } else {
        success = true;
      }
    }
    // if(head_ptr->Empty()) {
    //   head_ptr->SetRaw(item);
    //   assert(head_ptr->GetRaw() != nullptr);
    //   item->next_item_ptr = item;
    // } else {
    //   item_t* target = FindLessThan(key);
    //   assert(target != nullptr);
    //   item->next_item_ptr = target->next_item_ptr;
    //   target->next_item_ptr = item;
    // }
  }

  void Show() {
    item_t* dummy = head_ptr->GetRaw();
    assert(dummy != nullptr);
    do {
      std::cout << "[" << dummy->tag << "," << dummy->key << "]" << std::endl;
      dummy = dummy->next_item_ptr.load()->GetRaw();
    } while(dummy != head_ptr->GetRaw());
  }
  
  void Remove() {

  }
  
  void MoveHead();
private:
  head_ptr_t* head_ptr;
  IndexOptions* opts_;
  std::atomic<uint32_t> request_count_ = {0};
  std::atomic<uint32_t> size_ = {0};
  LocalStatistics* stat_;
  // Find the last key that's less than or equal to key.
  item_t* FindLessThan(const tag_key_t key) {
    if(head_ptr->Empty()) {
      return nullptr;
    }
    item_t* tmp_ptr = head_ptr->GetRaw();
    // return the only data in the list
    if(tmp_ptr->next_item_ptr.load()->GetRaw() == tmp_ptr) {
      return tmp_ptr;
    }
    item_t* prev_ptr = tmp_ptr;
    tmp_ptr = tmp_ptr->next_item_ptr.load()->GetRaw();

    while(true) {
      if((prev_ptr->Key() <= key) && (tmp_ptr->Key() > key)) {
        return prev_ptr;
      }
      if(prev_ptr->Key() > tmp_ptr->Key()) {  // on the edge
        if(prev_ptr->Key() == key) return prev_ptr;
        if(tmp_ptr->Key() == key) return tmp_ptr;
        if((prev_ptr->Key() <= key) || (key < tmp_ptr->Key())) { // if key is largest or smallest, insert here
          return prev_ptr;
        }
      }
      tmp_ptr = tmp_ptr->next_item_ptr.load()->GetRaw();
      prev_ptr = prev_ptr->next_item_ptr.load()->GetRaw();
    }
    return nullptr;
  }
  void SamplingShift() {
  #ifdef DEBUG
    std::cout << "start sampling shift" << std::endl;
  #endif
    // count total item in a ring
    int total = 0;
    item_t* dummy = head_ptr->GetRaw();
    assert(dummy != nullptr);
    do {
      ++total;
      dummy = dummy->next_item_ptr.load()->GetRaw();
    } while(dummy != head_ptr->GetRaw());
  #ifdef DEBUG
    std::cout << "total: " << total << std::endl;
  #endif

    std::vector<double> incomes(total, 0.0);
    RingIterator iter(head_ptr->GetRaw());
    
    for(int i = 1; i <= total; ++i) {
      for(int iter_loc = 1; iter_loc <= total; iter.Next(), iter_loc++) {
      #ifdef DEBUG
        if(i == 1) std::cout << "key: " << iter.GetCur()->GetRaw()->Key() << ", counter: " << GetCounterWeak(iter.GetCur()) << "," << head_ptr->GetCounter() << std::endl;
      #endif
        double ratio = (double)GetCounterWeak(iter.GetCur()) / head_ptr->GetCounter();
        double local_income = ratio * (abs(iter_loc - i) % total);
        incomes[i - 1] += local_income;
      }
    }
    int shift_idx = 0;
    double min_income = incomes[0];
  #ifdef DEBUG
    std::cout << "show incomes" << std::endl;
  #endif
    for(size_t i = 0; i < incomes.size(); ++i) {
    #ifdef DEBUG
      std::cout << incomes[i] << std::endl;
    #endif
      if(incomes[i] < min_income) {
        min_income = incomes[i];
        shift_idx = i;
      }
    }
  #ifdef DEBUG
    std::cout << "head moved " << shift_idx << std::endl;; 
  #endif
    head_ptr->ResetCounter();
    RingIterator head_iter(head_ptr->GetRaw());
    while(shift_idx > 0) {
      ResetCounter(head_iter.GetPrev()->GetRaw()->next_item_ptr);
      head_iter.Next();
      --shift_idx;
      --total;
    }
    head_ptr->SetRaw(head_iter.GetCur()->GetRaw());
  #ifdef DEBUG
    std::cout << " to " << head_ptr->GetRaw()->Key() << std::endl;
  #endif
    while(total > 0) {
      ResetCounter(head_iter.GetPrev()->GetRaw()->next_item_ptr);
      head_iter.Next();
      --total;
    }
  }
};

class HotRingIndex {
public:
  explicit HotRingIndex(IndexOptions options, LocalStatistics* stat = nullptr);
  ~HotRingIndex();
  Status Put(const std::string key, const std::string value);
  Status Get(const std::string& key, std::string& value);
  Status Delete(const std::string& key);
  LocalStatistics* stat_;
private:
  // in-memory structure
  IndexOptions opts_;
  Ring** m_hashtable_;
  uint64_t hash_bit_length = 32;
};
}