// Implementation of Get(), Put() and Delete()
#include "hotring_index.h"
#include "util/hashing/xxhash.h"
namespace HOTRING_NAMESPACE {

HotRingIndex::HotRingIndex(IndexOptions options, LocalStatistics* stat) {
  opts_ = options;
  stat_ = stat;
  m_hashtable_ = (Ring**)malloc(sizeof(Ring*) * opts_.bucket_size);
  for(uint64_t i = 0; i < opts_.bucket_size; ++i) {
    m_hashtable_[i] = new Ring(&opts_, stat);
  } 
}

HotRingIndex::~HotRingIndex() {
  for(uint64_t i = 0; i < opts_.bucket_size; ++i) {
    delete m_hashtable_[i];
  }
  if(m_hashtable_ != nullptr) free(m_hashtable_);
}

Status HotRingIndex::Put(const std::string key, const std::string value) {
  XXH32_hash_t hash = XXH32(key.c_str(), key.size(), 0);
  uint32_t hash_index = (hash & (((1UL << 16) - 1) << 16)) >> 16;
  uint32_t tag = ((1UL << 16) - 1) & hash;
  tag_key_t tk(tag, key);
  m_hashtable_[hash_index % opts_.bucket_size]->Insert(tk, value);
  return Status::OK();
}

Status HotRingIndex::Get(const std::string& key, std::string& value){
  XXH32_hash_t hash = XXH32(key.c_str(), key.size(), 0);
  uint32_t hash_index = (hash & (((1UL << 16) - 1) << 16)) >> 16;
  uint32_t tag = ((1UL << 16) - 1) & hash;
  tag_key_t tk(tag, key);
  if(!m_hashtable_[hash_index % opts_.bucket_size]->LookUp(tk, value) ){
    return Status::NotFound();
  } 
  return Status::OK();
}

Status HotRingIndex::Delete(const std::string& key) {
    (void)key;
    return Status::OK();
}
}