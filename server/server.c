/* encoding: UTF-8 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <zmq.h>

#include <errno.h>

#include "../dbdef.h"
#include "../client_server.h"
#include "../cache/cache.h"
#include "../hash/hash.h"
#include "./sync/sync.h"
#include "../md5/md5.h"
#include "../md5/message_digest.h"

#define _XOPEN_SOURCE
#include <signal.h>

#define SELF_NAME               "MSIU Device Control System Server"
#define SETSID_ATEMPTS_COUNT    5

#ifndef CATCH_SIGNAL
    #define CATCH_SIGNAL        1
#endif
#ifndef DEV_NULL_PATH
    #define DEV_NULL_PATH       "/dev/null"
#endif
#define DEFAULT_CACHE_PATH      "/home/kia84/Documents/Device_Control_System/server/"
#define DEFAULT_THREADS_COUNT   2
#define DEFAULT_THREADS_COUNT_S "2"

#define ZMQ_INPROC_ADDR         "inproc://workers"

#ifdef NDEBUG
    #define DEBUGMSG(X)
#else
    #define DEBUGMSG(X) (X)
#endif

typedef struct dcs_server_server_pool{
    /* global */
    int        be_verbose;           /* Логический флаг "болтливости" */
    char       server_addr[MAX_SERVER_ADDR_SIZE];    /* Адрес сервера */
    
    /* store */
    HASH       hash;
    LocalCache cache;
    /*Database   db;*/
    
    /* threads */
    pthread_t *tids;
    size_t     threads_count;
    pthread_t  sync_tid;
    pthread_barrier_t proxy_barr;
    int        no_barr;             /* Работать без барьера? */
    
    /* Для ZeroMQ */
    void       *context;
    void       *clients;
    void       *workers;
} DCS_SERVER_POOL;

static DCS_SERVER_POOL server_pool;

static const char server_replys[DCS_SERVER_REPLY_COUNT] = {
    DCS_SERVER_REPLY_OK,
    DCS_SERVER_REPLY_FULL
};

/* Функция корректного выхода. */
void correct_exit(){
    for(int i = 0; i < server_pool.threads_count; i++){
        pthread_cancel(server_pool.tids[i]);
    }
    zmq_close(server_pool.workers);
    zmq_close(server_pool.clients);
    zmq_ctx_destroy(server_pool.context);
    free(server_pool.tids);
    Cache_destroy(&server_pool.cache);
    Hash_destroy(&server_pool.hash);
    if(server_pool.be_verbose) syslog(LOG_INFO, "Server exit.");
    closelog();
    exit(EXIT_SUCCESS);
}

void sync_comp(){
    
}

void *thread_synchronizer(void *attr){
    int ret;
    /* Инициализируем кэш */
    ret = Cache_init(DEFAULT_CACHE_PATH"server_cache.db", &server_pool.cache);
    if(ret == LC_CODE_BADF_ERR)
        syslog(LOG_ERR, "Sync: произошла ошибка создания / открытия файла.");
    if(ret == LC_CODE_TABLE_ERR)
        syslog(LOG_ERR, "Sync: невозможно создать стандартную таблицу");
    /* Инициализируем хэш */
    Hash_init(&server_pool.hash, -1);
    ret = sync_cache_and_db(&server_pool.cache);
    if(ret != 0){
        syslog(LOG_ERR, "Sync with db failed.");
    }
    ret = sync_cache_and_hash(&server_pool.cache, &server_pool.hash);
    /* Wait conditions cycle here... */
    pthread_exit(NULL);
}

void  thread_operator_msg_clean(void *msgs){
    zmq_msg_t *reply_msgs = (zmq_msg_t *) msgs;
    for(int i = 0; i < DCS_SERVER_REPLY_COUNT; i++){
        zmq_msg_close(&reply_msgs[i]);
    }
}

void *thread_operator(void *attr){
    int rc;
    int my_id = (int) attr;
    
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    void *socket = zmq_socket(server_pool.context, ZMQ_REP);
    zmq_connect(socket, ZMQ_INPROC_ADDR);
    pthread_cleanup_push((void (*)(void *)) zmq_close, socket);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        
        zmq_msg_t reply_msgs[DCS_SERVER_REPLY_COUNT];
        
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        for(int i = 0; i < DCS_SERVER_REPLY_COUNT; i++){
            zmq_msg_init_size(&reply_msgs[i], DCS_SERVER_REPLY_SIZE);
            memcpy(zmq_msg_data(&reply_msgs[i]), &server_replys[i], DCS_SERVER_REPLY_SIZE);
        }
        pthread_cleanup_push(thread_operator_msg_clean, reply_msgs);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        
            if(!server_pool.no_barr){
                rc = pthread_barrier_wait(&server_pool.proxy_barr);
                if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
                    syslog(LOG_ERR, "Thread #%d cannot wait on barrier.", my_id);
            }
            
            while(1){
                int        reply_id = DCS_SERVER_REPLY_OK;
                zmq_msg_t  client_msg;
                char      *message;
                size_t     msg_size;
                unsigned char digest[MSG_DIGEST_SIZE];
                char      *domain;
                char      *md5sum;
                char      *sep;
                HASH_ELEMENT *comp;
                
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                zmq_msg_init(&client_msg);
                pthread_cleanup_push((void (*)(void *)) zmq_msg_close, &client_msg);
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                    
                    zmq_msg_recv(&client_msg, socket, 0);
                    message  = (char *) zmq_msg_data(&client_msg);
                    msg_size = zmq_msg_size(&client_msg);
                    DEBUGMSG(syslog(LOG_DEBUG, "msg_size = %d\n", msg_size));
                    if(msg_size >= MSG_DIGEST_SIZE + 1 + 1 + 2){
                        //~ проверка размера сообщения здесь!!!
                        //~ decrypt and verify message here!
                        //~ message = func(message)
                        memset(digest, '\0', MSG_DIGEST_SIZE);
                        rc = msg_digest(message + MSG_DIGEST_SIZE, MSG_SALT_PATH,
                                        msg_size - MSG_DIGEST_SIZE, digest);
                        if(rc){
                            DEBUGMSG(syslog(LOG_DEBUG, "msg_digest failed!!!"));
                        }
                        if(memcmp(message, digest, MSG_DIGEST_SIZE) == 0){
                            message += MSG_DIGEST_SIZE;
                            DEBUGMSG(syslog(LOG_DEBUG, "Thread #%d catch message: '%s'\n", my_id, message + 1));
                            
                            switch(*message){
                                case DCS_CLIENT_REQ_MD5:
                                    message++;
                                    sep = strchr(message, MSG_SEPARATOR);
                                    if(sep){
                                        *sep   = '\0';
                                        domain = message;
                                        md5sum = sep + 1;
                                        /* Проверки на длину md5-сум!!! */
                                        comp = Hash_find(&server_pool.hash, domain,
                                            (size_t) (unsigned int) sep - (unsigned int) message);
                                        if(comp){
                                            if(memcmp(md5sum, comp->val, HASH_ELEMENT_VAL_SIZE) != 0){
                                                /* Суммы различны, подать сюда полный список деталей! */
                                                reply_id = DCS_SERVER_REPLY_FULL;
                                                DEBUGMSG(syslog(LOG_DEBUG, "Суммы различны\n"));
                                            }
                                            /* else{
                                                //~ Суммы совпали, всё хорошо.
                                                reply_id = DCS_SERVER_REPLY_OK;
                                            }*/
                                        }
                                        else{ /* Компьютера в хэше нет. */
                                            reply_id = DCS_SERVER_REPLY_FULL;
                                            DEBUGMSG(syslog(LOG_DEBUG, "Компьютера в хэше нет\n"));
                                        }
                                    }
                                    break;
                                case DCS_CLIENT_REQ_FULL:
                                    message++;
                                    sep = strchr(message, MSG_SEPARATOR);
                                    if(sep){
                                        *sep   = '\0';
                                        domain = message;
                                        CL_Detail *details;
                                        size_t     details_count;
                                        unsigned char *hwdata = (unsigned char *) sep + 1;
                                        msg_size -= (MSG_DIGEST_SIZE + 2 + (
                                                (size_t) (unsigned int) sep - (unsigned int) message));
                                        
                                        comp = Hash_find(&server_pool.hash, domain,
                                            (size_t) (unsigned int) sep - (unsigned int) message);
                                        if(comp){
                                            //~ sync_comp();
                                        }
                                        else{ /* Компьютера в хэше нет - новый компьютер. */
                                            DEBUGMSG(syslog(LOG_DEBUG, "Новая машина!"));
                                            //~ details = (CL_Detail *) calloc(sizeof(CL_Detail), 40);
                                            details = (CL_Detail *) malloc(sizeof(CL_Detail) * 40);
                                            
                                            unsigned char *hwdata_p = hwdata;
                                            for(int i = 0; i < 40; i++){
                                                int tmp_size = sizeof(unsigned int) * 5 + 
                                                    sizeof(details[i].bus_addr) + sizeof(details[i].serial_length);
                                                DEBUGMSG(syslog(LOG_DEBUG, "HERE1! tmp_size = %d, serial_s = %d",
                                                            tmp_size, sizeof(details[i].serial_length)));
                                                memcpy(&(details[i].vendor_id), hwdata_p, tmp_size);
                                                //~ details[i].vendor_id = (unsigned int) hwdata;
                                                hwdata_p += tmp_size;
                                                DEBUGMSG(syslog(LOG_DEBUG, "Detail: %.4x:%.4x:%.8x (%.2x) [%.6x]: '%s', '%d'",
                                                                    details[i].vendor_id,
                                                                    details[i].device_id,
                                                                    details[i].subsystem_id,
                                                                    details[i].revision,
                                                                    details[i].class_code,
                                                                    details[i].bus_addr,
                                                                    details[i].serial_length
                                                ));
                                                DEBUGMSG(syslog(LOG_DEBUG, "HERE2! %d", details[i].serial_length));
                                                memcpy(details[i].serial, hwdata_p, details[i].serial_length);
                                                details[i].serial[details[i].serial_length] = '\0';
                                                hwdata_p += details[i].serial_length;
                                                DEBUGMSG(syslog(LOG_DEBUG, "HERE3!"));
                                                memcpy(&(details[i].params_length), hwdata_p, sizeof(details[i].params_length));
                                                hwdata_p += sizeof(details[i].params_length);
                                                DEBUGMSG(syslog(LOG_DEBUG, "HERE4! params_length: %d", details[i].params_length));
                                                memcpy(&(details[i].params), hwdata_p, details[i].params_length);
                                                
                                                DEBUGMSG(syslog(LOG_DEBUG, "Detail: %.4x:%.4x:%.8x (%.2x) [%.6x]: '%s', '%s'",
                                                                    details[i].vendor_id,
                                                                    details[i].device_id,
                                                                    details[i].subsystem_id,
                                                                    details[i].revision,
                                                                    details[i].class_code,
                                                                    details[i].serial,
                                                                    details[i].params
                                                ));
                                                if((unsigned int) (hwdata_p - hwdata) >= msg_size){
                                                    details_count = i + 1;
                                                    details = (CL_Detail *) realloc(details, sizeof(CL_Detail) * details_count);
                                                    break;
                                                }
                                            }
                                            
                                        }
                                    }
                                    break;
                                default:
                                    DEBUGMSG(syslog(LOG_DEBUG, "default = %02x\n", *message));
                                    break;
                            }
                        }
                        else{
                            DEBUGMSG(syslog(LOG_DEBUG, "memcmp failed!"));
                        }
                    }
                    
                pthread_cleanup_pop(1); /* zmq_msg_close() */
                DEBUGMSG(syslog(LOG_DEBUG, "Reply %d: '%.2x'\n", reply_id, *((unsigned int *) zmq_msg_data(&reply_msgs[reply_id]))));
                zmq_msg_send(&reply_msgs[reply_id], socket, 0);
            }
        pthread_cleanup_pop(0); /* thread_operator_msg_clean */
    pthread_cleanup_pop(0); /* zmq_close */
    pthread_exit(NULL);
}

/* Функция, закрывающая стандартный файловый дескриптор, и
   создающая его копию, перенаправленную в /dev/null.      */
static void close_and_dup_stdfd(int stdfd, int nullfd){
    /* Если дескриптор не открыт - ничего страшного.       */
    /* Но если происходит что-то странное (EINTR|EIO) -    */
    /* завершаем работу.                                   */
    errno = 0;
    if(close(stdfd) < 0 && errno != EBADF){
        syslog(LOG_EMERG, "close(%d): %s.", stdfd, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(dup2(nullfd, stdfd) < 0){
        syslog(LOG_EMERG, "dup2(%d to "DEV_NULL_PATH"): %s.",
                stdfd, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    pid_t pid, sid;
    pthread_attr_t pthread_attr;
    int   opt,               /* Буфер для распознания опций argv через getopt */
          opt_t = 0;         /* Указан ли ключ '-t' */
    int null_fd;
    int rc;                 /* return code */
    
    #if CATCH_SIGNAL
        struct sigaction sa;
        sigset_t         sa_set;
    #endif
    
    /* Отделяемся от родительского процесса */
    pid = fork();
    
    /* Если не проходит даже форк - значит дела совсем плохи -
     * завершаем работу тут же. */
    if (pid < 0) {
        perror("fork()");
        exit(EXIT_FAILURE);
    }
    
    /* Если дочерний процесс порождён успешно, то родительский процесс можно завершить. */
    if (pid > 0) { exit(EXIT_SUCCESS); }
    
    /* Открытие журнала на запись */
    openlog(SELF_NAME, LOG_ODELAY|LOG_PERROR|LOG_PID, LOG_DAEMON);
    
    /* Создание нового SID для дочернего процесса */
    sid = setsid();
    /* Если получить sid не удалось - пытаемся сделать это ещё SETSID_ATEMPTS_COUNT раз */
    TRY_N_TIMES(SETSID_ATEMPTS_COUNT, (sid = setsid()), (sid < 0), "setsid()", LOG_CRIT);
    /* Если после вышеописанных попыток sid всё равно не получен - завершаем работу. */
    if (sid < 0) {
        syslog(LOG_EMERG, "setsid(): %s.", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    #if CATCH_SIGNAL
        sigemptyset(&sa_set);
        sigaddset(&sa_set, SIGHUP);
        sigprocmask(SIG_BLOCK, &sa_set, 0);

        sa.sa_mask    = sa_set;
        sa.sa_flags   = SA_NOMASK;
        sa.sa_handler = correct_exit;

        sigaction(SIGTSTP, &sa, 0);
        sigaction(SIGINT,  &sa, 0);
        sigaction(SIGTERM, &sa, 0);
        sigaction(SIGQUIT, &sa, 0);
    #endif
    
    /* Изменяем файловую маску */
    umask(0);
    
    /* Изменяем текущий рабочий каталог */
    if (chdir("/") < 0) {
        syslog(LOG_WARNING, "chdir(): %s.", strerror(errno));
    }
    
    /* Закрываем стандартные файловые дескрипторы - 
     * теперь вместо них будет /dev/null            */
    null_fd = open(DEV_NULL_PATH, O_RDWR | O_NONBLOCK);
    if(null_fd < 0){
        syslog(LOG_EMERG, "open(\""DEV_NULL_PATH"\"): %s.", strerror(errno));
        exit(EXIT_FAILURE);
    }
    close_and_dup_stdfd(STDIN_FILENO,  null_fd);
    //~ close_and_dup_stdfd(STDOUT_FILENO, null_fd);
    //~ close_and_dup_stdfd(STDERR_FILENO, null_fd);
    
    /* Заполним страктуру значениями по-умолчанию */
    /*  server_pool.be_verbose    = 0;  */
        server_pool.threads_count = DEFAULT_THREADS_COUNT;
    
    /* Проверим все переданные аргументы */
    while ((opt = getopt(argc, argv, "vt:")) != -1) {
        switch (opt) {
            case 'v':    /* verbose */
                server_pool.be_verbose = 1;
                break;
            /*case 'c':     cache file path
                break;*/
            case 't':    /* thread count */
                server_pool.threads_count = atoi(optarg);
                opt_t = 1;
                if(server_pool.threads_count < 1){
                    syslog(LOG_WARNING, "Threads count incorrect."
                        "Use default: " DEFAULT_THREADS_COUNT_S);
                }
                break;
            default:    /* '?' */
                break;
        }
    }
    if(opt_t == 0 && server_pool.be_verbose){
        syslog(LOG_NOTICE, "Threads count not specified. "
            "Use default: " DEFAULT_THREADS_COUNT_S);
    }
    /* Получаем адрес сервера. */
    if (optind >= argc) {
        if(server_pool.be_verbose){
            syslog(LOG_NOTICE, "Server addres not specified. "
                "Use default: '" DEFAULT_SERVER_ADDR "'");
        }
        strncpy(server_pool.server_addr, DEFAULT_SERVER_ADDR, MAX_SERVER_ADDR_SIZE);
    }
    else{
        strncpy(server_pool.server_addr, argv[optind], MAX_SERVER_ADDR_SIZE);
    }
    
    /* Инициализируем барьер. */
    if(pthread_barrier_init(&server_pool.proxy_barr, NULL,
                          server_pool.threads_count + 1)
    ){
        syslog(LOG_ERR, "Error in barrier creating.");
        server_pool.no_barr = 1;
    }
    else{
        server_pool.no_barr = 0;
    }
    
    /* Резервируем место под нити */
    server_pool.tids = (pthread_t *) malloc(sizeof(pthread_t) * server_pool.threads_count);
    
    server_pool.context = zmq_ctx_new();
    server_pool.clients = zmq_socket(server_pool.context, ZMQ_ROUTER);
    server_pool.workers = zmq_socket(server_pool.context, ZMQ_DEALER);
    zmq_bind(server_pool.clients, server_pool.server_addr);
    zmq_bind(server_pool.workers, ZMQ_INPROC_ADDR);
    
    pthread_attr_init(&pthread_attr);
    pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
        /* sync pthread init */
        if(pthread_create(&server_pool.sync_tid, &pthread_attr, &thread_synchronizer, NULL) != 0){
            syslog(LOG_EMERG, "Error in sync thread creating.");
            correct_exit();
        }
        
        /* cmd pthread init */
        
        /* client pthreads init */
        for(int i = 0; i < server_pool.threads_count; i++){
            if(pthread_create(&server_pool.tids[i], &pthread_attr, &thread_operator, (void *) i) != 0){
                syslog(LOG_EMERG, "Error in thread creating.");
                correct_exit();
            }
        }
    pthread_attr_destroy(&pthread_attr);
    
    if(server_pool.be_verbose){
        syslog(LOG_INFO, "Initialize complete. Start main cycle.");
    }
    
    /* Перед запуском zmq_proxy необходимо открыть все нужные сокеты.
     * Ожидаем, когда все клиентские нити сделают это. */
    if(!server_pool.no_barr){
        rc = pthread_barrier_wait(&server_pool.proxy_barr);
        if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
            syslog(LOG_ERR, "Cannot wait on barrier.");
        else{
            pthread_barrier_destroy(&server_pool.proxy_barr);
            #ifndef NDEBUG
                syslog(LOG_DEBUG, "Barrier destroy. Start proxy.");
            #endif
        }
    }
    
    zmq_proxy(server_pool.clients, server_pool.workers, NULL);
    
    return EXIT_SUCCESS;
}
