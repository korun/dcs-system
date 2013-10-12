/* encoding: UTF-8 */

#ifndef	_MESSAGE_DIGEST_H
#define _MESSAGE_DIGEST_H

int msg_digest(char *msg, const char *salt_fd_path,
        size_t msg_size, unsigned char digest[16]);

#endif /* message_digest.h */
