/* encoding: UTF-8 */

/** hash.h, Ivan Korunkov, 2012 **/

/**
    Здесь находится реализация функций, объявленных в hash.h
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "hash.h"

/* Макросы для проверки аргументов.
   Проверяют соответствие ключей и значений максимальным размерам,
   установленным в HASH_ELEMENT_KEY_SIZE и
   HASH_ELEMENT_VAL_SIZE. */
#define CHECK_ARG(A, B)      if((A) > (B)){errno = EINVAL; return NULL;}
#define CHECK_ARG_VOID(A, B) if((A) > (B)){errno = EINVAL; return;}

/*  Функция, производящая первичную инициализацию хэша. */
int Hash_init(HASH *new_hash, int pow_2){
    size_t hash_size; /* Размер хэш-таблицы */
    
    /* Если передан отрицательный параметр -
       используем настройки по-умолчанию.   */
    if(pow_2 < 0) pow_2 = DEFAULT_HASH_TABLE_POW2;
    
    /* Вычисляем размер хэш-таблицы... */
    hash_size = hashsize(pow_2);
    
    /* Запрашиваем, и чистим память для хэш-таблицы. */
    new_hash->hash_table =
        (HASH_ELEMENT **) calloc(hash_size, sizeof(HASH_ELEMENT *));
    
    if(!new_hash->hash_table){
        /* Не хватает памяти. errno = ENOMEM */
        return HASH_ERROR_NO_MEMORY;
    }
    
    /* Заполняем поля структуры. */
    new_hash->table_size = hash_size;
    new_hash->power2     = pow_2;
    new_hash->elem_count = 0;         /* количество элементов в массиве */
    
    return 0;
}

/*  Функция, производящая освобождение занятой хэшем памяти.
    Сбрасывает все установленные в структуре HASH поля.      */
void Hash_destroy(HASH *exists_hash){
    uint32_t     i;            /* индекс в хэш-таблице             */
    HASH_ELEMENT *element;     /* текущий элемент таблицы          */
    HASH_ELEMENT *tmp_element; /* элемент для корректного удаления */
    
    /* Если хэш-таблицы не существует, то и удалять ничего не надо. */
    if(!exists_hash->hash_table)    return;
    
    /* Перебираем всю хэш-таблицу, пока не удалим все существующие элементы */
    for(i = 0; exists_hash->elem_count && i < exists_hash->table_size; i++){
        /* Удаляемый элемент может быть началом L1 списка.
           В этом случае нужно удалить все элементы из этого списка. */
        element = exists_hash->hash_table[i];
        while(element){
            tmp_element = element->next;
            free(element);
            element     = tmp_element;
            /* Уменьшаем счётчик оставшихся элементов. */
            exists_hash->elem_count--;
        }
    }
    free(exists_hash->hash_table);
    exists_hash->hash_table = NULL;
    exists_hash->table_size = 0;
    exists_hash->power2     = 0;
    /* После цикла elem_count должен быть равен нулю.
       Если это не так значит что-то пошло не так. O_o */
}

/*  Функция, проверяющая необходимость расширения хэш-таблицы, и выполняющая
    расширение, если это нужно. При расширении размер таблицы увеличивается
    в два раза.
    Если разрешено авто-расширение, Hash_expand будет вызываться при каждом
    добавлении элемента в таблицу. Иначе, Hash_expand может быть вызван
    вручную, в любой момент времени.
    Поэтому, в случае неавтоматического контроля расширений, Hash_expand
    при своём вызове не просто увеличит размер массива в два раза, а увеличит
    его ровно настолько, чтобы коэффициент заполненности не превышал 
    MAXIMUM_HASH_TABLE_LOAD_FACTOR.
    При автоматическом расширении такой подсчёт (ближайшей нужной степени
    двойки) убран в целях оптимизации. */
int Hash_expand(HASH *exists_hash){
    /* индекс в хэш-таблице, её размер, и количество элементов */
    uint32_t     i, old_table_size, old_elem_count;
    HASH_ELEMENT **old_hash_table;  /* указатель на текущую хэш-таблицу     */
    HASH_ELEMENT *element;          /* текущий элемент таблицы              */
    HASH_ELEMENT *tmp_element;      /* элемент для корректного удаления     */
    int   power2;                   /* новая степень двойки                 */
    float load_factor;              /* коэффициент заполнения хэш-таблицы   */
    size_t hash_size;               /* новый размер хэш-таблицы             */
    #if !AUTO_EXPAND
        int near_pow2;              /* число для вычисления новой степени 2 */
    #endif
    
    load_factor = (float) (exists_hash->elem_count) / exists_hash->table_size;
    
    /* Если коэффициент заполнения слишком мал - расширение не имеет смысла.*/
    if(load_factor <= MAXIMUM_HASH_TABLE_LOAD_FACTOR)
        return HASH_ERROR_LOW_LOADFACT;
    
    /* Новая степень двойки, по которой будет подсчитан новый размер
       хэш-таблицы. По умолчанию мы просто увеличиваем размер в 2 раза */
    power2 = exists_hash->power2 + 1;
    
    #if !AUTO_EXPAND
        /* Считаем near_pow2. Оно содержит в себе ближайший
           размер массива, который удовлетворил бы условию 
           load_factor <= MAXIMUM_HASH_TABLE_LOAD_FACTOR */
        near_pow2 = (exists_hash->elem_count) / MAXIMUM_HASH_TABLE_LOAD_FACTOR;
        /* Теперь увеличиваем степень двойки до тех пор, пока размер,
           посчитанный по этой степени, не сможет покрыть near_pow2. */
        while(hashsize(power2) < near_pow2) power2++;
    #endif
    
    /* Если степень слишком велика - расширение запрещено. */
    if(power2 > MAXIMUM_HASH_TABLE_POW2) return HASH_ERROR_HIGH_POW2;
    
    /* Запоминаем указатель на текущую хэш-таблицу. */
    old_hash_table = exists_hash->hash_table;
    
    hash_size = hashsize(power2);
    
    exists_hash->hash_table =
        (HASH_ELEMENT **) calloc(hash_size, sizeof(HASH_ELEMENT *));
    
    if(!exists_hash->hash_table){
        /* Не хватает памяти. errno = ENOMEM
           Восстанавливаем указатель и возвращаем ошибку. */
        exists_hash->hash_table = old_hash_table;
        return HASH_ERROR_NO_MEMORY;
    }
    
    /* Если память для новой хэш-таблицы успешно выделена -
       запоминаем старые параметры, и записываем новые.   */
    old_table_size = exists_hash->table_size;
    old_elem_count = exists_hash->elem_count;
    exists_hash->table_size = hash_size;
    exists_hash->elem_count = 0;         /* заполняется в Hash_insert */
    exists_hash->power2     = power2;
    
    /* Перемещаем элементы из старой хэш-таблицы в новую. */
    for(i = 0; old_elem_count && i < old_table_size; i++){
        /* Перемещаемый элемент может быть началом L1 списка.
           В этом случае нужно переместить все элементы из этого списка. */
        element = old_hash_table[i];
        while(element){
            tmp_element = element->next;
            Hash_insert(exists_hash,
                        element->key, strlen(element->key),
                        element->val, strlen(element->val)
                    );
            free(element);
            element     = tmp_element;
            /* Уменьшаем счётчик оставшихся элементов. */
            old_elem_count--;
        }
    }
    
    /* Освобождаем память, занятую старой хэш-таблицей. */
    free(old_hash_table);
    
    return 0;
}

/* Вспомогательная функция, заполняющая поля структуры HASH_ELEMENT. */
static void _Hash_element_strncpy(
        HASH_ELEMENT *element,
        const char   *new_key,
        size_t        key_length,
        const char   *new_val,
        size_t        val_length
    ){
    /* Если длина ключа равна 0 - значит нужно перезаписать только значение. */
    if(key_length){
        strncpy(element->key, new_key, key_length);
        element->key[key_length] = '\0';
        element->next = NULL;
    }
    strncpy(element->val, new_val, val_length);
    element->val[val_length] = '\0';
}

/* Вспомогательная функция, проверяющая ключи на эквивалентность.
   (Может возникнуть вопрос: "Почему нельзя просто использовать strncmp?"
   Ответ: потому что результат strncmp("ice12", "ice1", 4) будет 0.
   Мы не храним размер строки old_key для экономии памяти, но знаем, что
   она гарантированно конечна (содержит '\0').
   Эта функция заменяет собой выражение:
   new_key_size <= strlen(old_key) && !strncmp(old_key, new_key, new_key_size)
   с той лишь разницей, что здесь нет двойного пробега по строке old_key.) */
static int _Hash_element_equal_key(
        const char *old_key,
        const char *new_key,
        size_t      new_key_size
    ){
    unsigned int i = 0;
    while(i < new_key_size){
        if(old_key[i] != new_key[i]) return 0;
        if(old_key[i] == '\0' || new_key[i] == '\0'){
            if(old_key[i] == '\0' && new_key[i] == '\0')
                return 1;
            return 0;
        }
        i++;
    }
    if(old_key[i] != '\0')
        return 0;
    return 1;
}

/* Функция, добавляющая в хэш новую пару [ключ -> значение].
   Если такой ключ уже существует, его значение будет перезаписано. */
HASH_ELEMENT *Hash_insert(
        HASH       *exists_hash,
        const char *new_key,
        size_t      key_length,
        const char *new_val,
        size_t      val_length
    ){
    uint32_t      index;   /* индекс в хэш-таблице */
    HASH_ELEMENT *element; /* текущий элемент (нужен для перебора) */
    
    /* Длина ключа и значения не должна превышать размера полей HASH_ELEMENT */
    errno = 0;
    CHECK_ARG(key_length, HASH_ELEMENT_KEY_SIZE);
    CHECK_ARG(val_length, HASH_ELEMENT_VAL_SIZE);
    
    /* Если разрешено автоматическое расширение - запускаем его. */
    #if AUTO_EXPAND
        Hash_expand(exists_hash);
    #endif
    
    /* Вычисляем индекс элемента в массиве с помощью хэш-функции.
       Использование hashmask позволяет взять число, возвращаемое
       хэш-функцией по модулю размера хэш-таблицы.
       (Это грубо говоря. А для подробностей см. hash_func.h) */
    index = hashfunc(new_key, key_length, (uint32_t) DEFAULT_INITVAL(new_key)) &
            hashmask(exists_hash->table_size);
    
    /* Если ячейка с таким индексом в таблице свободна,
       то мы просто добавляем новый элемент. */
    if(!exists_hash->hash_table[index]){
        exists_hash->hash_table[index] =
            (HASH_ELEMENT *) malloc(sizeof(HASH_ELEMENT));
        
        if(!exists_hash->hash_table[index]){
            /* Не хватает памяти. errno = ENOMEM */
            return NULL;
        }
        /* Заполняем поля в новой записи. */
        _Hash_element_strncpy(exists_hash->hash_table[index],
                              new_key, key_length,
                              new_val, val_length
                            );
        
        exists_hash->elem_count++;  /* Увеличиваем счётчик элементов. */
        
        return exists_hash->hash_table[index];
    }
    
    /* Иначе ячейка занята, а это значит что этот
       элемент уже добавлялся, либо произошла коллизия. */
    element = exists_hash->hash_table[index];
    /* Если элемент уже был добавлен, необходимо проверить его ключ на
       эквивалентность новому ключу. Так как элемент может являться началом
       L1 списка, необходимо проверить ключи всех элементов в списке. */
    while(1){
        /* если ключи совпадают - перезаписываем значение на новое */
        if(_Hash_element_equal_key(element->key, new_key, key_length)){
            _Hash_element_strncpy(element, NULL, 0, new_val, val_length);
            return element;
        }
        if(element->next) element = element->next;
        else break;
    }
    
    /* Если элементов с таким же ключом не найдено - добавляем новый элемент
       в конец списка. */
    element->next = (HASH_ELEMENT *) malloc(sizeof(HASH_ELEMENT));
    if(!element->next){
        /* Не хватает памяти. errno = ENOMEM */
        return NULL;
    }
    element = element->next;
    /* Заполняем поля в новой записи. */
    _Hash_element_strncpy(element, new_key, key_length, new_val, val_length);
    /* Увеличиваем счётчик элементов. */
    exists_hash->elem_count++;
    
    return element;
}

/* Функция поиска по хэшу. */
HASH_ELEMENT *Hash_find(
        HASH       *exists_hash,
        const char *key,
        size_t      key_length
    ){
    uint32_t      index;   /* индекс в хэш-таблице */
    HASH_ELEMENT *element; /* текущий элемент (нужен для перебора) */
    
    /* Длина ключа не должна превышать размера поля HASH_ELEMENT   */
    errno = 0;
    CHECK_ARG(key_length, HASH_ELEMENT_KEY_SIZE);
    
    /* Вычисляем индекс элемента в массиве с помощью хэш-функции.
       Использование hashmask позволяет взять число, возвращаемое
       хэш-функцией по модулю размера хэш-таблицы.
       (Это грубо говоря. А для подробностей см. hash_func.h) */
    index = hashfunc(key, key_length, (uint32_t) DEFAULT_INITVAL(key)) &
            hashmask(exists_hash->table_size);
    
    /* По индексу получаем элемент из хэш-таблицы. */
    element = exists_hash->hash_table[index];
    
    /* Так как элемент может являться началом L1 списка,
       необходимо проверить ключи всех элементов в списке. */
    while(element){
        if(_Hash_element_equal_key(element->key, key, key_length))
            return element;
        element = element->next;
    }
    
    return NULL;
}

void Hash_remove_element(
        HASH       *exists_hash,
        const char *key,
        size_t      key_length
    ){
    uint32_t      index;        /* индекс в хэш-таблице */
    HASH_ELEMENT *element;      /* текущий элемент (нужен для перебора) */
    HASH_ELEMENT *prev_element; /* указатели на следующий и предыдущий\ */
    HASH_ELEMENT *next_element; /* элементы, для корректного удаления.  */
    
    /* Длина ключа не должна превышать размера поля HASH_ELEMENT */
    errno = 0;
    CHECK_ARG_VOID(key_length, HASH_ELEMENT_KEY_SIZE);
    
    /* Вычисляем индекс элемента в массиве с помощью хэш-функции.
       Использование hashmask позволяет взять число, возвращаемое
       хэш-функцией по модулю размера хэш-таблицы.
       (Это грубо говоря. А для подробностей см. hash_func.h) */
    index = hashfunc(key, key_length, (uint32_t) DEFAULT_INITVAL(new_key)) &
            hashmask(exists_hash->table_size);
    
    /* По индексу получаем элемент из хэш-таблицы. */
    element      = exists_hash->hash_table[index];
    prev_element = NULL; /* У начала списка нет предыдущего элемента. */
    
    /* Так как элемент может являться началом L1 списка,
       необходимо проверить ключи всех элементов в списке. */
    while(element){
        /* Если элемент найден, нужно исключить элемент из списка
           и правильно перевязать соседние элементы между собой. */
        if(_Hash_element_equal_key(element->key, key, key_length)){
            next_element = element->next;
            
            if(prev_element) prev_element->next = next_element;
            else exists_hash->hash_table[index] = next_element;
            
            free(element);
            exists_hash->elem_count--;
            break;
        }
        prev_element = element;
        element      = prev_element->next;
    }
}
