/* encoding: UTF-8 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
//~ #include <sys/mman.h>       /* For shm */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <zmq.h>

#include "gethwdata.h"
#include "../dbdef.h"
#include "../client_server.h"
#include "../md5/md5.h"
#include "../md5/message_digest.h"

#define SELF_NAME               "MSIU Device Control System Client"
#define MAX_DOMAIN_SIZE         COMPUTER_DOMAIN_NAME_SIZE
#define SETSID_ATEMPTS_COUNT    5

#ifndef DEV_NULL_PATH
    #define DEV_NULL_PATH       "/dev/null"
#endif

#define store_uint32_to(DEST, UINT32)                                           \
    do {                                                                        \
        uint32_t ordered_int32 = htonl((uint32_t) (UINT32));                    \
        memcpy((void *) (DEST), (void *) &(ordered_int32), sizeof(uint32_t));   \
    } while(0)

#define store_uint16_to(DEST, UINT16)                                           \
    do {                                                                        \
        uint16_t ordered_int16 = htons((uint16_t) (UINT16));                    \
        memcpy((void *) (DEST), (void *) &(ordered_int16), sizeof(uint16_t));   \
    } while(0)

#define store_uint8_to(DEST, UINT8) (*(DEST) = (uint8_t) (UINT8))

#define SHIFT_AND_INC(POINTER, SIZE, VAL)   \
    do {                                    \
        (POINTER) += (VAL);                 \
        (SIZE)    += (VAL);                 \
    } while(0)

/* Функция, закрывающая стандартный файловый дескриптор, и
   создающая его копию, перенаправленную в /dev/null.      */
static void close_and_dup_stdfd(int stdfd, int newfd){
    /* Если дескриптор не открыт - ничего страшного.       */
    /* Но если происходит что-то странное (EINTR|EIO) -    */
    /* завершаем работу.                                   */
    errno = 0;
    if(close(stdfd) < 0 && errno != EBADF){
        syslog(LOG_EMERG, "close(%d): %s.", stdfd, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(dup2(newfd, stdfd) < 0){
        syslog(LOG_EMERG, "dup2(%d to %d): %s.",
                stdfd, newfd, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/*
   digest(md5)  0 hostname |    data(md5)   0
/-------^------\^/----^---\^/-------^------\^
000000000011111111112222222222333333333344444 4444455555555556
012345678901234567890123456789012345678901234 5678901234567890
*/
static int fill_message(
        unsigned char *message,
        size_t      message_size,
        char        ctrl_byte,
        const char *hostname,
        size_t      hostname_size,
        const unsigned char *data,
        size_t      data_size
    ){
    unsigned char digest[MSG_DIGEST_SIZE];
    
    message[MSG_CTRL_BYTE] = ctrl_byte;
    memcpy(message + MSG_DIGEST_SIZE + 1, hostname, hostname_size);
    message[MSG_DIGEST_SIZE + 1 + hostname_size] = MSG_SEPARATOR;
    memcpy(message + MSG_DIGEST_SIZE + 1 + hostname_size + 1, data, data_size);
    message[message_size] = '\0';
    memset(digest, '\0', MSG_DIGEST_SIZE);
    int rc = msg_digest((char *) (message + MSG_DIGEST_SIZE),
                    MSG_SALT_PATH, message_size - MSG_DIGEST_SIZE, digest);
    if(rc) return 1;
    memcpy(message, digest, MSG_DIGEST_SIZE);
    return 0;
}

int main (int argc, char **argv) {
    int   be_verbose = 0;   /* Логический флаг "болтливости" */
    int   opt;              /* Буфер для распознания опций argv через getopt */
    char  server_addr[MAX_SERVER_ADDR_SIZE];    /* Адрес сервера */
    char  hostname[MAX_DOMAIN_SIZE];
    int   hostname_size;
    int   message_size;
    int   null_fd;
    
    /* Для ZeroMQ */
    void     *context;
    void     *requester;
    zmq_msg_t request;
    unsigned char *message;
    
    #ifdef NDEBUG
        pid_t pid, sid;
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
    #endif
    
    /* Открытие журнала на запись */
    openlog(SELF_NAME, LOG_ODELAY|LOG_PERROR|LOG_PID, LOG_DAEMON);
    
    #ifdef NDEBUG
        /* Создание нового SID для дочернего процесса */
        sid = setsid();
        /* Если получить sid не удалось - пытаемся сделать это ещё SETSID_ATEMPTS_COUNT раз */
        TRY_N_TIMES(SETSID_ATEMPTS_COUNT, (sid = setsid()), (sid < 0), "setsid()", LOG_CRIT);
        /* Если после вышеописанных попыток sid всё равно не получен - завершаем работу. */
        if (sid < 0) {
            syslog(LOG_EMERG, "setsid(): %s.", strerror(errno));
            exit(EXIT_FAILURE);
        }
    #endif
    
    /* Изменяем файловую маску - создаваемые файлы (etc) будут доступны
     * только для самого создавшего. 0666 & ~077 = 0600 */
    umask(S_IRWXG|S_IRWXO);
    
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
    #ifdef NDEBUG
        close_and_dup_stdfd(STDOUT_FILENO, null_fd);
        close_and_dup_stdfd(STDERR_FILENO, null_fd);
    #endif
    
    /* Получаем список всех деталей */
    CL_Detail *details;
    size_t     details_size;
    size_t     hw_list_size = 0;
    unsigned char  hw_list[10240];
    unsigned char *tmp_p = hw_list;
    details = gethwdata(&details_size);
    
    for(int i = 0; i < details_size; i++){
        unsigned char *print_pointer = tmp_p; /* debug only */
        store_uint16_to(tmp_p, details[i].vendor_id);
            SHIFT_AND_INC(tmp_p, hw_list_size, sizeof(details[i].vendor_id));
        store_uint16_to(tmp_p, details[i].device_id);
            SHIFT_AND_INC(tmp_p, hw_list_size, sizeof(details[i].device_id));
        store_uint32_to(tmp_p, details[i].subsystem_id);
            SHIFT_AND_INC(tmp_p, hw_list_size, sizeof(details[i].subsystem_id));
        store_uint32_to(tmp_p, details[i].class_code);
            SHIFT_AND_INC(tmp_p, hw_list_size, sizeof(details[i].class_code));
        store_uint8_to(tmp_p, details[i].revision);
            SHIFT_AND_INC(tmp_p, hw_list_size, sizeof(details[i].revision));
        memcpy(tmp_p, &details[i].bus_addr, sizeof(details[i].bus_addr));
            SHIFT_AND_INC(tmp_p, hw_list_size, sizeof(details[i].bus_addr));
        store_uint32_to(tmp_p, details[i].serial_length);
            SHIFT_AND_INC(tmp_p, hw_list_size, sizeof(details[i].serial_length));
        memcpy(tmp_p, details[i].serial, details[i].serial_length);
            SHIFT_AND_INC(tmp_p, hw_list_size, details[i].serial_length);
        store_uint32_to(tmp_p, details[i].params_length);
            SHIFT_AND_INC(tmp_p, hw_list_size, sizeof(details[i].params_length));
        memcpy(tmp_p, details[i].params, details[i].params_length);
            SHIFT_AND_INC(tmp_p, hw_list_size, details[i].params_length);
        
        printf("ID's:\t%.4x:%.4x:%.8x\n",
            details[i].vendor_id,
            details[i].device_id,
            details[i].subsystem_id);
        printf("    :\t%.4x:%.4x:%.8x\n",
            get_uint16_from(print_pointer),
            get_uint16_from(print_pointer + sizeof(uint16_t)),
            get_uint32_from(print_pointer + sizeof(uint16_t) * 2)
        );
        int shift = sizeof(uint16_t) * 2;
        printf("Class:\t%.6x (rev: %.2x)\n",
            details[i].class_code, details[i].revision);
        printf("     :\t%.6x (   : %.2x)\n",
            get_uint32_from(print_pointer + shift + sizeof(uint32_t)),
            get_uint8_from(print_pointer + shift + sizeof(uint32_t) * 2)
        );
        shift += sizeof(uint32_t) * 2;
        printf("Bus:\t\t'%s'\nSerial[%u]:\t'%s'\n",
            details[i].bus_addr, details[i].serial_length, details[i].serial);
        printf("   :\t\t'%s'\n      [%u]:\t'%s'\n",
            (print_pointer + shift + sizeof(uint8_t)),
            get_uint32_from(print_pointer + shift + sizeof(uint8_t) + sizeof(details[i].bus_addr)),
            (print_pointer + shift + sizeof(uint8_t) + sizeof(details[i].bus_addr) + sizeof(uint32_t)));
        shift += sizeof(uint8_t) + sizeof(details[i].bus_addr) + sizeof(uint32_t);
        printf("Params[%d]:\t%s\n", details[i].params_length,
            details[i].params);
        printf("      [%d]:\t%s\n", get_uint32_from(print_pointer + shift + details[i].serial_length),
            (print_pointer + shift + details[i].serial_length + sizeof(uint32_t)));
        printf("tmp_p(int): %.8x\n", (unsigned int) (tmp_p));
        printf("hw_list_size: %d\n", (unsigned int) (hw_list_size));
        printf("------------------------------------\n");
        
        free(details[i].params);
    }
    
    free(details);
    //~ return 0;
    
    /* Проверим все переданные аргументы */
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':   /* verbose */
                be_verbose = 1;
                break;
            default:    /* '?' */
                /* Все неизвестные аргументы игнорируем */
                break;
        }
    }
    /* Получаем адрес сервера. */
    if (optind >= argc) {
        if(be_verbose){
            syslog(LOG_NOTICE, "Server addres not specified. "
                "Use default: '" DEFAULT_SERVER_ADDR_FOR_CLIENT "'");
        }
        strncpy(server_addr, DEFAULT_SERVER_ADDR_FOR_CLIENT, MAX_SERVER_ADDR_SIZE);
    }
    else{
        strncpy(server_addr, argv[optind], MAX_SERVER_ADDR_SIZE);
    }
    
    if(be_verbose){
        syslog(LOG_INFO, "Initialize complete. Getting data...");
    }
    
    /* Получаем имя хоста */
    gethostname(hostname, MAX_DOMAIN_SIZE);
    
    /* Подсчёт md5-суммы */
    MD5_CTX mdcontext;
    unsigned char hw_digest[MSG_DIGEST_SIZE];
    
    MD5Init(&mdcontext);
    MD5Update(&mdcontext, hw_list, hw_list_size);
    MD5Final(hw_digest, &mdcontext);
    
    /* Подключаемся... */
    context   = zmq_ctx_new();
    requester = zmq_socket(context, ZMQ_REQ);
    zmq_connect(requester, server_addr);
    
    /* Формирование сообщения для передачи серверу */
    hostname_size = strnlen(hostname, MAX_DOMAIN_SIZE);
    message_size = MSG_DIGEST_SIZE + 1 + hostname_size + 1 + 16 + 1;
    
    zmq_msg_init_size(&request, message_size);
    
    message = (unsigned char *) zmq_msg_data(&request);
    
    /*int rc = */fill_message(message, message_size, DCS_CLIENT_REQ_MD5,
        hostname, hostname_size, hw_digest, 16);
    //~ assert(rc == 0);
    
    printf("Digest  = '");
    for(int i = 0; i < MSG_DIGEST_SIZE; i++) printf("%02x", message[i]);
    printf("'\n");
    printf("Control byte = '%02x'\n", *(message + MSG_DIGEST_SIZE));
    printf("Hostname = '");
    for(int i = 0; i < hostname_size; i++) printf("%c", *(message + MSG_DIGEST_SIZE + 1 + i));
    printf("'\n|\n");
    printf("Message = '");
    for(int i = 0; i < MSG_DIGEST_SIZE; i++) printf("%02x", *(message + MSG_DIGEST_SIZE + hostname_size + 1 + i));
    printf("'\n");
    
    zmq_msg_send(&request, requester, 0);
    zmq_msg_close(&request);
    
    zmq_msg_init(&request);
    zmq_msg_recv(&request, requester, 0);
    printf("\nAnswer: '%.2x'\n", *((char *) zmq_msg_data(&request)));
    printf("True? %d\n", *((char *) zmq_msg_data(&request)) == DCS_SERVER_REPLY_FULL);
    
    for(int i = 0; i < 128; i++) printf("-"); printf("\n");
    
    //~ if(strncmp((char *) zmq_msg_data(&request), DCS_SERVER_REPLY_FULL, DCS_SERVER_REPLY_SIZE) == 0){
    if(*((char *) zmq_msg_data(&request)) == DCS_SERVER_REPLY_FULL){
        /* Нужно прислать полную конфигурацию */
        zmq_msg_close(&request);
        message_size = MSG_DIGEST_SIZE + 1 + strnlen(hostname, MAX_DOMAIN_SIZE) + 1 + hw_list_size;
        zmq_msg_init_size(&request, message_size);
        message = (unsigned char *) zmq_msg_data(&request);
        
        int rc = fill_message(message, message_size, DCS_CLIENT_REQ_FULL,
            hostname, hostname_size, hw_list, hw_list_size);
        printf("rc = %d\n", rc);
        //~ assert(rc == 0);
        
        printf("Digest  = '");
        for(int i = 0; i < MSG_DIGEST_SIZE; i++) printf("%02x", message[i]);
        printf("'\n");
        printf("Control byte = '%02x'\n", *(message + MSG_DIGEST_SIZE));
        //~ printf("Message = '%s'\n", message + MSG_DIGEST_SIZE);
        printf("Hostname = '");
        for(int i = 0; i < hostname_size; i++) printf("%c", *(message + MSG_DIGEST_SIZE + 1 + i));
        printf("'\n|\n");
        printf("Message = '");
        for(int i = 0; i < hw_list_size; i++) printf("%c", *(message + MSG_DIGEST_SIZE + hostname_size + 1 + i));
        printf("'\n");
        
        zmq_msg_send(&request, requester, 0);
    }
    zmq_msg_close(&request);
    
    zmq_close(requester);
    zmq_ctx_destroy(context);
    
    return EXIT_SUCCESS;
}
