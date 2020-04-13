/**
 * @file callbacks.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es); 
 * @brief Definicion de los callbacks asociados a los comandos ftp
 * @version 1.0
 * @date 12-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef CALLBACKS_H
#define CALLBACKS_H
#include "utils.h"
#include "ftp_session.h"

void *cdup_cb(void *info, void* info_out);

void *cwd_cb(void *info, void* info_out);

void *help_cb(void *info, void* info_out);

void *mkd_cb(void *info, void* info_out);

void *pass_cb(void *info, void* info_out);

void *rnto_cb(void *info, void* info_out);

void *list_cb(void *info, void* info_out);

void *pasv_cb(void *info, void* info_out);

void *dele_cb(void *info, void* info_out);

void *port_cb(void *info, void* info_out);

void *pwd_cb(void *info, void* info_out);

void *quit_cb(void *info, void* info_out);

void *retr_cb(void *info, void* info_out);

void *rmd_cb(void *info, void* info_out);

void *rmda_cb(void *info, void* info_out);

void *stor_cb(void *info, void* info_out);

void *rnfr_cb(void *info, void* info_out);

void *size_cb(void *info, void* info_out);

void *type_cb(void *info, void* info_out);

void *user_cb(void *info, void* info_out);

#endif