/* encoding: UTF-8 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(_WIN32) && !defined(WIN32)
	#include <sys/time.h>
	#define TIME_TEST
#endif

#include "hash.h"

void Hash_display(HASH * exists_hash) {
	uint32_t       i;
	HASH_ELEMENT * element;
	
	float load_factor;
	
	if(!exists_hash->hash_table)	return;
	printf("\n\t _Hash__%.8d________%.8x_________%.8d_______________________________________\n", (uint32_t) exists_hash->table_size, (uint32_t) exists_hash->hash_table, (uint32_t) exists_hash->elem_count);
	for(i = 0; i < exists_hash->table_size; i++){
		if(exists_hash->hash_table[i]){
			printf("\t|\ttable[%d]  \t%.8x  \t%s   \t%s\n", i, (uint32_t) exists_hash->hash_table[i], exists_hash->hash_table[i]->key, exists_hash->hash_table[i]->val);
			element = exists_hash->hash_table[i]->next;
			while(element){
				printf("\t|\t\tlist  \t%.8x  \t%s   \t%s\n", (uint32_t) element, element->key, element->val);
				element     = element->next;
			}
		}
	}
	printf("\t|_______________________________________________________________________________________\n\n");
	
	load_factor = (float) exists_hash->elem_count / exists_hash->table_size;
	printf("Load factor:\t%f\n", load_factor);
}

HASH_ELEMENT * Array_find(HASH * exists_hash, const char * key, size_t key_length) {
	uint32_t       index;
	size_t         tmp_key_size;
	HASH_ELEMENT * element;
	#ifdef TIME_TEST
		/*time_t	start_time;
		start_time = time(NULL);*/
	#endif
	
	key_length = (key_length > DEFAULT_HASH_ELEMENT_KEY_SIZE ? DEFAULT_HASH_ELEMENT_KEY_SIZE : key_length);
	
	for(index = 0; index < exists_hash->table_size; index++){
		element = exists_hash->hash_table[index];
		
		if (element == NULL){
			continue;
		}
		
		for(; element->next != NULL; element = element->next){		/* Going to list... */
			tmp_key_size = strlen(element->key);
			if (key_length <= tmp_key_size && !strncmp(element->key, key, key_length)) {
				#if defined(TIME_TEST)
					/*printf("\tSLOW FIND    %ld\t%s   \tinlist => %s\n", start_time, key, element->val);*/
				#endif
				return element;
			}
		}
		
		tmp_key_size = strlen(element->key);
		if (key_length <= tmp_key_size && !strncmp(element->key, key, key_length)) {
			#if defined(TIME_TEST)
				/*printf("\tSLOW FIND    %ld\t%s   \t=> %s\n", start_time, key, element->val);*/
			#endif
			return element;
		}
	}
	#if defined(TIME_TEST)
		/*printf("\tSLOW FIND    %ld\t%s   \t=> NULL\n", start_time, key);*/
	#endif
	return NULL;
}

int main(void) {
	int i, j;
	char * keys[] = {
		"monster01",	"monster02",	"monster03",	"monster04",
		"monster05",	"monster06",	"monster07",	"monster08",
		"monster09",	"monster10",	"monster11",	"monster12",
		"monster13",	"monster14",	"monster15",	"monster16",
		"monster17",	"monster18",	"monster19",	"monster20",
		"ice01",	"ice02",	"ice03",	"ice04",
		"ice05",	"ice06",	"ice07",	"ice08",
		"ice09",	"ice10",	"ice11",	"ice12",
		"ice13",	"ice14",	"ice15",	"ice16",
		"ice17",	"ice18",	"ice19",	"ice20",
		"fire01",	"fire02",	"fire03",	"fire04",
		"fire05",	"fire06",	"fire07",	"fire08",
		"fire09",	"fire10",	"fire11",	"fire12",
		"fire13",	"fire14",	"fire15",	"fire16",
		"fire17",	"fire18",	"fire19",	"fire20"
	};
	
	char * values[] = {
		"63985036671a91cb4406498fda70027f",	"6cb7acf6f5f1cdb6c9bf7b7851762aca",	"04130b1df34bfa1943403e83f247b752",	"c186962cfd9f0bc9a732aaa90fac3142",
		"34d066155b981d7d97314c302d6c7a7c",	"ebca9f93b632bb728b2aa68e3b877696",	"82d2c1f0b78e90c10dd88c6e87c01a72",	"d19a63b399e0e265b346542b6995defb",
		"1542e943c02b7aa2e4c2935854c2e27f",	"49e54001a1dd726381dcd67a8e9542a3",	"80c55051ae3e39e3c874f6564e9d219e",	"dc157a0bb5e66e016801f6f7761f5c39",
		"61ab54aced8289716674689ac0765583",	"3a1a5fa361fbd0f8e109be3c92d13d4d",	"a986855a643de4a96a8683fd95479df4",	"bd562e5f6963ffd1e0ef4c0ace90eedd",
		"fbc2dc3d86c37652d340d770fd9158e7",	"8c425d8e674cd13aac41305829b168ad",	"197d5d7544ca03087040fe6f5538e81f",	"2c5ab6cb0980cc7e9b79c0b035e75dd6",
		"b647f51f0e07cc3f6a3ebc9d28852bc0",	"40fd121e066bf6d252c6b5d1cdff6078",	"e4a49fc8815094649a2d88c709facf8f",	"76102d13d9a9036f1592a9252e9c044d",
		"00035e090c5dc1d9d31ce28425f3e18a",	"627db931ad7a1d0b32427dea6d3fe4ea",	"eb453566c89a10f2d4ab169b1f5e8e97",	"c1aafed37a9a029865b635822a17d265",
		"3153578688b2668383920eec5d997a61",	"3990d7fe1574557a8877a84910a57178",	"8ab33e427f89570f6677310d8d16683e",	"eb8a4373b61a333d2db706439bf10e1d",
		"4f471304555d8a73ec0b78edcccb8899",	"ed634f11a43e9553865eeec37e499315",	"7fe416623f3ee9ecbb9d189c5a539aab",	"d7d684bb97eacce0b716d5e74147cb40",
		"8088d4188caa9af36d1927cc412961af",	"1c17478eb899979ea072cb9ef04560bf",	"d4559e54036ff7052ba39297d85d6448",	"30c13d89d4937d70d2fce494c987b695",
		"fd9d94d71c0e5f6a055c44ed1dd60c16",	"114b112d375fb421c2262999d68c07b2",	"2b40b6d8badf887bebe28d230d0b03e1",	"8563357566223928b9965100bd40a1bf",
		"1d20f3d3ebe71eb2e47b897a5e5ffa59",	"f97525aee60cfbc0634b4cd9fb1c3e3e",	"2bae9e896a0e0929e83935161582508a",	"729664e5abbe356c141f4d2d36d1c777",
		"91aef7d674df4f2554a323711817d65d",	"70748a049350cad7e6c816fab05a0848",	"4ec733f19c728248e3c382aaf2951d63",	"ad6d24854487c31786be125b3eedcedc",
		"1a1ddf491ad3d7b901f82e6e5a47d4be",	"400b8018e915cd523680e71cfde9864d",	"2c43811452a55e61adfd265c9d641978",	"1324a1747bed35d37dd314f5fc4b4394",
		"bc51625eefea1e797f113cd25deb6b1d",	"043def562290e1ea2ed3982997109fd8",	"afed775e106e0470ed25d9167f048fbe",	"59921100b24010643d3186a156d276be"
	};
	
	HASH md5_sum_hash;
	
	char buffer[1];
	
	#ifdef TIME_TEST
		struct timeval tv;
		suseconds_t start_time;
		suseconds_t start_time_sec;
	#endif
	
	printf("Hash_init...\n");
	Hash_init(&md5_sum_hash, 4);
	for(i = 0; i < 60; i++){
		Hash_insert(&md5_sum_hash, keys[i], strlen(keys[i]), values[i], 32);
	}
	Hash_display(&md5_sum_hash);
	
	printf("\n");
	
	for(i = 0; i < 60; i += 2){
		Hash_insert(&md5_sum_hash, keys[i], strlen(keys[i]), "qq", 2);
	}
	#if !AUTO_EXPAND
		Hash_expand(&md5_sum_hash);
	#endif
	Hash_display(&md5_sum_hash);
	
	printf("\n");
	
	printf("Hash_find: \"monster21\": %.8x\n", (unsigned int) Hash_find(&md5_sum_hash, "monster21", 9));
	Hash_insert( &md5_sum_hash, "monster21", 9, "Hello!", 6);
	printf("Hash_find: \"monster21\": %.8x\n", (unsigned int) Hash_find(&md5_sum_hash, "monster21", 9));
	
	printf("\n");
	
	for(i = 0; i < 60; i += 3){
		Hash_find(&md5_sum_hash, keys[i], strlen(keys[i]));
	}
	
	printf("\n");
	
	for(i = 0; i < 60; i += 2){
		Hash_remove_element(&md5_sum_hash, keys[i], strlen(keys[i]));
	}
	
	printf("\n");
	
	Hash_display(&md5_sum_hash);
	printf("Hash_destroy: <#HASH table_size: %.5d, elem_count: %.3d, power2: %.2d>\n", md5_sum_hash.table_size, md5_sum_hash.elem_count, md5_sum_hash.power2);
	Hash_destroy(&md5_sum_hash);
	printf("              <#HASH table_size: %.5d, elem_count: %.3d, power2: %.2d>\n", md5_sum_hash.table_size, md5_sum_hash.elem_count, md5_sum_hash.power2);
	
	/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/
	
	printf("\n\n\n\t\t\tAnd now... START EPIC TEST!!!\n\n\n");
	
	i = (int) fread(buffer, 1, 1, stdin);
    
	#if AUTO_EXPAND
		Hash_init(&md5_sum_hash, 22);
	#else
		Hash_init(&md5_sum_hash, 8);
	#endif
	for(i = 0; i < 60; i++){
		for(j = 0; j < 5; j++){
			Hash_insert(&md5_sum_hash, keys[i] + j, strlen(keys[i] + j), values[i] + j, 32 - j);
			Hash_insert(&md5_sum_hash, values[i] + 8 + j, 9, "val be here", 11);
		}
	}
	/*Hash_display(&md5_sum_hash);*/
	#if !AUTO_EXPAND
		Hash_expand(&md5_sum_hash);
		Hash_display(&md5_sum_hash);
	#endif
	
	printf("Start hash find cicle:\n");
	#ifdef TIME_TEST
		gettimeofday(&tv, NULL);
		start_time = tv.tv_usec;
		start_time_sec = tv.tv_sec;
	#endif
	for(i = 0; i < 60; i += 3){
		for(j = 0; j < 5; j++){
			Hash_find(&md5_sum_hash, keys[i] + j, strlen(keys[i] + j));
			Hash_find(&md5_sum_hash, values[i] + 8 + j, 9);
		}
	}
	
	#ifdef TIME_TEST
		gettimeofday(&tv, NULL);
		printf("%ld.%06ld\t", tv.tv_sec - start_time_sec, (tv.tv_usec > start_time ? tv.tv_usec - start_time : start_time - tv.tv_usec));
	#endif
	
	printf("Start sloooow find cicle:\n");
	#ifdef TIME_TEST
		gettimeofday(&tv, NULL);
		start_time = tv.tv_usec;
		start_time_sec = tv.tv_sec;
	#endif
	for(i = 0; i < 60; i += 3){
		for(j = 0; j < 5; j++){
			Array_find(&md5_sum_hash, keys[i] + j, strlen(keys[i] + j));
			Array_find(&md5_sum_hash, values[i] + 8 + j, 9);
		}
	}
	
	#ifdef TIME_TEST
		gettimeofday(&tv, NULL);
		printf("%ld.%06ld\n", tv.tv_sec - start_time_sec, (tv.tv_usec > start_time ? tv.tv_usec - start_time : start_time - tv.tv_usec));
	#endif
	Hash_destroy(&md5_sum_hash);
	
	return EXIT_SUCCESS;
}
