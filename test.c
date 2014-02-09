#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include"dict.h"

#define DEFAULT_KEY_SIZE 16
#define DEFAULT_VALUE_SIZE 32

//get ms of local time
long long now(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

void add(dict *d, char *key, size_t key_len, char *value, size_t value_len) {
	char *k = (char *)malloc(key_len + 1);
	char *v = (char *)malloc(value_len + 1);
	snprintf(k, key_len + 1, "%s", key);
    snprintf(v, value_len + 1, "%s", value);
	int rs = dictAdd(d, k, v);
	if (!rs) {
		printf("add error--key: %s  value: %s\n", k, v);
	}
}

void printDictStatus(dict *d) {
	printf("----------------------------------------\n");
	printf("rehashId: %d\n", d->rehashId);
	if (d->rehashId == -1) {
		printf("size: %d\n", d->ht[0].size);
		printf("used: %d\n", d->ht[0].used);
	} else {
		printf("size: %d\n", d->ht[0].size);
		printf("used: %d\n", d->ht[0].used);
	}
	
	printf("----------------------------------------\n");
}

int main() {
	int i, rs;
    char *getRs;
    hashNode *hd;
    dict *d = dictNew();
    
    char *k = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	char *v = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
	char *key = malloc(32 + 1);
	char *value = malloc(32 + 1);
    
	printf("\n\n*******************add************************\n");
	int rd_k, rd_v, rd_k_len, rd_v_len;
	srand(time(NULL));
	for (i = 0; i < 100; i++) {
		snprintf(key, 32, "%s", k);
		snprintf(value, 32, "%s", v);
		
		rd_k = rand() % 25;
		rd_v = rand() % 24;
		rd_k_len = rand() % 32;
		rd_v_len = rand() % 32;
		
		key[rd_k_len] += rd_k;
		value[rd_v_len] += rd_v;
		add(d, key, rd_k_len + 1, value, rd_v_len + 1);
	}
	printDictStatus(d);
	
	printf("\n\n*******************get************************\n");
	for (i = 0; i < 100; i++) {
		snprintf(key, 32, "%s", k);
		snprintf(value, 32, "%s", v);
		
		rd_k = rand() % 25;
		rd_k_len = rand() % 32;
		
		key[rd_k_len] += rd_k;
		key[rd_k_len + 1] = '\0';
		getRs = dictGet(d, key);
		printf("get result: %s\n", getRs);
	}
	printDictStatus(d);
	
    printf("\n\n******************delete**********************\n");
	int deleted = 0;
	for (i = 0; i < 100; i++) {
		snprintf(key, 32, "%s", k);
		snprintf(value, 32, "%s", v);
		
		rd_k = rand() % 25;
		rd_k_len = rand() % 32;
		
		key[rd_k_len] += rd_k;
		key[rd_k_len + 1] = '\0';
		rs = dictDelete(d, key);
		if(rs) {
			deleted++;
		}
	}
	printf("you have deleted %d nodes\n", deleted);
	printDictStatus(d);
	
	
	printf("\n\n*****************key list*********************\n");
	dictKeys(d);
	
	
	printf("\n\n*****************dictSize*********************\n");
	int dSize = dictSize(d);
    printf("dictSize: %d\n", dSize);
	printDictStatus(d);
	
	printf("\n\n*****************dictFlush********************\n");
	int dFlush = dictFlush(d);
    printf("dictFlush: %d\n", dFlush);
	printDictStatus(d);

    free(key);
    free(value);
	
    return 0;
}
