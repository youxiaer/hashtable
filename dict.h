#ifndef _DICT_H_
#define _DICT_H_

#define DICT_OK 1
#define DICT_ERR 0

typedef struct hashNode {
    void *key;
    void *value;
    struct hashNode *next;
} hashNode;

typedef struct hashTable {
    hashNode **table;
    //the size of hashtable
    unsigned long size;
    //the used node of hashtable
    unsigned long used;
}hashTable;

typedef struct dict {
    hashTable ht[2];
    int rehashId;
    unsigned long (*key_hash)(const char *key, unsigned long len);
}dict;

//#define LONG_MAX 0x0fffffffffffffff
#define DICT_DEFAULT_SIZE 8
#define dictIsRehashing(ht) ((ht)->rehashId != -1)

/* API */
dict *dictNew(void);
void dictFree(dict *d);
int dictAdd(dict *d, void *key, void *value);
void *dictGet(dict *d, void *key);
int dictDelete(dict *d, void *key);
unsigned long dictSize(dict *d);
void dictKeys(dict *d);
int dictFlush(dict *d);


#endif
