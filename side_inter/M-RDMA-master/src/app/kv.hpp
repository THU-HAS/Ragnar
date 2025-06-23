#if !defined(KV_HPP)
#define KV_HPP

#include <cstdint>
#include <cstring>
#include <type_traits>

// #define USE_VAR_STR

const int max_value_size = 8;
// const int max_value_size = 64; 

struct KV {
    uint64_t version;
    uint64_t key;
#ifdef USE_VAR_STR
    char value[0];
#else
    char value[max_value_size];
#endif

    KV(uint64_t key_, char *value_): key(key_) {
        if (value_ != nullptr) {

            std::memcpy(value, value_, max_value_size);
        }
    }
    // variable array

    forceinline void reinit(uint64_t key_, char *value_) {
        key = key_;
        if (value_ != nullptr) {

            std::memcpy(value, value_, max_value_size);
        }
    } 
};

forceinline int transfer_port(int src, int dst) {
    if (src > dst)
        return src * 100 + dst;
    if (dst > src)
        return dst * 100 + src;
    assert(dst != src);
    return -1;
}

forceinline void update_KV(KV *kv, uint64_t key_, char *value_) {

    kv->key = key_;
    if (value_ != nullptr) {
        
        std::memcpy(kv->value, value_, max_value_size);
    }
} 

struct KV_DATA {
    uint64_t key;
    char value[0];
};
#ifdef USE_VAR_STR
const int kv_size = sizeof(KV) + max_value_size;
#else
const int kv_size = sizeof(KV);
#endif

using KV_PTR = KV*;
using char_ptr = char*;

#endif // KV_HPP
