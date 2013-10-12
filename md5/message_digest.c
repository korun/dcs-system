/* encoding: UTF-8 */

/*
 * message_digest.c
 * 
 * Copyright 2013 Ivan Korunkov <kia84@mail.msiu.ru>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
#include <stdio.h>

#include "md5.h"
#include "../client_server.h"

int msg_digest(char *msg, const char *salt_fd_path,
        size_t msg_size, unsigned char digest[16]){
    int len;
    FILE *file;
    MD5_CTX context;
    unsigned char buffer[1024];
    
    if((file = fopen(salt_fd_path, "r")) == NULL)
        return 1;
    
    MD5Init(&context);
    MD5Update(&context, (unsigned char *) msg, msg_size);
    while((len = fread(buffer, 1, 1024, file)))
        MD5Update(&context, buffer, len);
    MD5Final(digest, &context);
    fclose(file);
    
    return 0;
}
