#ifndef HASH_H
#define HASH_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "alloc.h"

template<typename Key, typename Value>
class Hash
{
public:
    typedef ui32(*hash_key_id)(const Key& key);
    typedef bool(*hash_key_cmp)(const Key& key1, const Key& key2);
    typedef struct Bucket {
        Key key_;
        Value value_;

        Bucket* next_;
    }Bucket;
public:
    Hash() :
        bucket_(0),
        hash_size_(0),
        hash_id_(0) {
    }
    ~Hash() {
        destroy();
    }
    void create(int size, hash_key_id id, hash_key_cmp cmp) {
        hash_id_ = id;
        hash_cmp_ = cmp;
        bucket_ = new Bucket*[size];

        for (int i = 0; i < size; ++i){
            bucket_[i] = 0;
        }

        hash_size_ = size;
    }
    void destroy() {
        for (ui32 i = 0; i < hash_size_; ++i) {
            Bucket* b = bucket_[i];
            while (b) {
                Bucket* n = b->next_;
                alloc_delete_object(b);
                b = n;
            }
        }
        delete[] bucket_;
        hash_size_ = 0;
        bucket_ = 0;
        hash_id_ = 0;
    }
    ui32 size(){
        return hash_size_;
    }
    bool find(const Key& key, Value& value) {
        ui32 id = hash_id_(key);
        Bucket* b = bucket_[id % hash_size_];

        while (b) {
            if (hash_cmp_(key, b->key_)){
                value = b->value_;
                return true;
            }
            b = b->next_;
        }

        return false;
    }
    Bucket* get(const Key& key){
        ui32 id = hash_id_(key);
        return bucket_[id % hash_size_];
    }
    Bucket* get_at(ui32 id){
        if (id >= hash_size_){
            return 0;
        }
        return bucket_[id % hash_size_];
    }
    void insert(const Key& key, Value value) {
        ui32 id = hash_id_(key);
        Bucket* b = bucket_[id % hash_size_];

        Bucket* n = alloc_new_object<Bucket>();
        n->key_ = key;
        n->value_ = value;
        n->next_ = b;

        bucket_[id % hash_size_] = n;
    }

    bool remove(const Key& key, Value& value) {
        ui32 id = hash_id_(key);
        Bucket* b = bucket_[id % hash_size_];
        Bucket* p = 0;

        while (b) {
            if (hash_cmp_(key , b->key_)) {
                if (!p){
                    bucket_[id % hash_size_] = b->next_;
                }
                else{
                    p->next_ = b->next_;
                }
                value = b->value_;
                alloc_delete_object(b);

                return true;
            }

            p = b;
            b = b->next_;
        }
        return false;
    }
    bool remove(const Key& key) {
        ui32 id = hash_id_(key);
        Bucket* b = bucket_[id % hash_size_];
        Bucket* p = 0;

        while (b) {
            if (hash_cmp_(key, b->key_)) {
                if (!p){
                    bucket_[id % hash_size_] = b->next_;
                }
                else{
                    p->next_ = b->next_;
                }

                alloc_delete_object(b);

                return true;
            }

            p = b;
            b = b->next_;
        }
        return false;
    }
protected:
    Bucket     **bucket_;
    ui32         hash_size_;
    hash_key_id  hash_id_;
    hash_key_cmp hash_cmp_;
private:
    Hash(const Hash&);
    Hash& operator=(const Hash&);
};

#endif
