#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <sys/time.h>

#include "dict.h"

static unsigned int dict_force_resize_ratio = 5;
static unsigned int dict_can_resize = 1;
static int _getHashKey(dict *d, const void *key);
static int _dictExpandIfNeeded(dict *d);
static int _dictExpand(dict *d, unsigned long size);
static int _dictRehash(dict *d, int n);
static unsigned long _dictNextPower(unsigned long size);
static void _dictRelease(dict *d);

static unsigned long djb_hash(const char *str, unsigned long len) {

        unsigned long hash = 5381;
        /*
		char *p;
        for(p = str; p < str + len; ++p) {
                hash = ((hash << 5) + hash) + (unsigned char)(*p); 
        }
		*/
		while (len--) {
			hash = ((hash << 5) + hash) + (unsigned char)(*str++); /* hash * 33 + c */
		}

        return hash;
}

dict *dictNew() {
    dict *d = malloc(sizeof(*d));

    _dictReset(&d->ht[0]);
    _dictReset(&d->ht[1]);
    d->key_hash = djb_hash;
    d->rehashId = -1;
    
    return d;
}

int _dictReset(hashTable *ht)
{
    ht->table = NULL;
    ht->size = 0;
    ht->used = 0;
    
    return DICT_OK;
}

void freeNode(hashNode *hd) {
    if (hd) {
        if (hd->key) {
            free(hd->key);
        }
        if (hd->value) {
            free(hd->value);
        }
        free(hd);
    }
}

void dictFree (dict *d) {
    _dictRelease(d);
    free(d);
}

static void _dictRelease(dict *d) {
    unsigned long i, j;
	
    for (i = 0; i <= 1; i++) {
        for (j = 0; j < d->ht[i].size && d->ht[i].used > 0; j++) {
            hashNode *hd, *nextHd;

            if ((hd = d->ht[i].table[j]) == NULL) {
                continue;
            }
            while(hd) {
                nextHd = hd->next;
                freeNode(hd);
                d->ht[i].used--;

                hd = nextHd;
            }
			
        }
		if (d->ht[i].table != NULL) {
			free(d->ht[i].table);
        }
        _dictReset(&d->ht[i]);
    }
}


hashNode *dictFind(dict *d, void *key)
{
    hashNode *hd;
    unsigned int hk, index, table;

    if (d->ht[0].size == 0) {
        return NULL; 
    }
    
    hk = _getHashKey(d, key);
    for (table = 0; table <= 1; table++) {
        index = hk & (d->ht[table].size - 1);
        hd = d->ht[table].table[index];
        while(hd) {
            // 找到并返回
            if (strcmp(key, hd->key) == 0)
                return hd;

            hd = hd->next;
        }
        
        if (!dictIsRehashing(d)) return NULL;
    }
    
    return NULL;
}

int dictAdd(dict *d, void *key, void *value)
{
    unsigned int hk, index;
    hashNode *hd ,*fd;
    hashTable *ht;
    if ((hk = _getHashKey(d, key)) == -1) {
        return DICT_ERR;
    }

    ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
    index = hk & (ht->size - 1);
    
    fd = dictFind(d, key);
    if (fd) {
		free(fd->value);
        fd->value = value;
    } else {
        hd = malloc(sizeof(*hd));
        hd->next = ht->table[index];
        ht->table[index] = hd;
        ht->used++;
        hd->key = key;
        hd->value = value;
    }
    
    return DICT_OK;
}

void *dictGet(dict *d, void *key) {
    hashNode *hd;

    hd = dictFind(d,key);

    return hd ? hd->value : NULL;
}

int dictDelete(dict *d, void *key)
{
    unsigned int hk, index, table;
    hashNode *hd, *preHd;
    
    if (d->ht[0].size == 0) {
        return DICT_ERR; 
    }
    
    hk = _getHashKey(d, key);
    for (table = 0; table <= 1; table++) {
        index = hk & (d->ht[table].size - 1);
        hd = d->ht[table].table[index];
        preHd = NULL;
        while(hd) {
            if (strcmp(key, hd->key) == 0) {
                if (preHd) {
                    preHd->next = hd->next;
                } else {
                    d->ht[table].table[index] = hd->next;
                }
				freeNode(hd);
                
                d->ht[table].used--;

                return DICT_OK;
            }
            preHd = hd;
            hd = hd->next;
        }
        
        if (!dictIsRehashing(d)) break;
    }

    return DICT_ERR;
}

void dictKeys(dict *d) {
	unsigned long i, j;
    
    for (i = 0; i <= 1; i++) {
        for (j = 0; j < d->ht[i].size && d->ht[i].used > 0; j++) {
            hashNode *hd, *nextHd;

            if ((hd = d->ht[i].table[j]) == NULL) {
                continue;
            }
            while(hd) {
                nextHd = hd->next;
                printf("k: %s  v: %s\n", hd->key, hd->value);

                hd = nextHd;
            }
        }
    }
}

unsigned long dictSize(dict *d) {
    if (dictIsRehashing(d)) {
        return d->ht[0].used + d->ht[1].used;
    } else {
        return d->ht[0].used;
    }
}

int dictFlush(dict *d) {
    _dictRelease(d);
    return DICT_OK;
}

static int _getHashKey(dict *d, const void *key)
{
    unsigned int hk, table;

    if (_dictExpandIfNeeded(d) == DICT_ERR)
        return -1;
    hk = d->key_hash(key, strlen(key));
    
    return hk;
}

static int _dictExpandIfNeeded(dict *d)
{
    if (dictIsRehashing(d)) return DICT_OK;

    if (d->ht[0].size == 0) return _dictExpand(d, DICT_DEFAULT_SIZE);

    if (d->ht[0].used >= d->ht[0].size &&
        (dict_can_resize ||
         d->ht[0].used/d->ht[0].size > dict_force_resize_ratio))
    {
        return _dictExpand(d, d->ht[0].used*2);
    }

    return DICT_OK;
}

static int _dictExpand(dict *d, unsigned long size)
{
    hashTable n;
    unsigned long realsize = _dictNextPower(size);

    if (dictIsRehashing(d) || d->ht[0].used > size)
        return DICT_ERR;

    n.size = realsize;
    n.table = malloc(realsize*sizeof(hashNode*));
    n.used = 0;

    if (d->ht[0].table == NULL) {
        d->ht[0] = n;
        return DICT_OK;
    }

    d->ht[1] = n;
    d->rehashId = 0;
    return _dictRehash(d, size);
}

static int _dictRehash(dict *d, int n) {
    if (!dictIsRehashing(d)) return 0;

    while(n--) {
        hashNode *hd, *nexthd;

        if (d->ht[0].used == 0) {
            free(d->ht[0].table);
            d->ht[0] = d->ht[1];
            _dictReset(&d->ht[1]);
            d->rehashId = -1;
            return DICT_OK;
        }
        assert(d->ht[0].size > (unsigned)d->rehashId);
        while(d->ht[0].table[d->rehashId] == NULL) d->rehashId++;
        
        hd = d->ht[0].table[d->rehashId];
		
        while(hd) {
            unsigned int hk, index;

            nexthd = hd->next;
			hk = _getHashKey(d, hd->key);
			index = hk & (d->ht[1].size - 1);
            hd->next = d->ht[1].table[index];
            d->ht[1].table[index] = hd;

            // 更新计数器
            d->ht[0].used--;
            d->ht[1].used++;

            hd = nexthd;
        }
        d->ht[0].table[d->rehashId] = NULL;

        d->rehashId++;
    }

    return DICT_OK;
}

static unsigned long _dictNextPower(unsigned long size)
{
    unsigned long i = DICT_DEFAULT_SIZE;

    if (size >= LONG_MAX) return LONG_MAX;
    while(1) {
        if (i >= size)
            return i;
        i *= 2;
    }
}

