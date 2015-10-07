/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef MGMT_THREAD_H_
#define MGMT_THREAD_H_

#include <config.h>
#include <vector>
#include "../dl/download.h"
#include <cfgfile/cfgfile.h>
#include <netpptk/netpptk.h>
#include "connection_manager.h"


void mgmt_thread_main();

void connection_handler(client *connection);
void target_dl(std::string &data, tkSock *sock);
void target_pkg(std::string &data, tkSock *sock);
void target_var(std::string &data, tkSock *sock);
void target_file(std::string &data, tkSock *sock);
void target_router(std::string &data, tkSock *sock);
void target_premium(std::string &data, tkSock *sock);
void target_subscription(std::string &data, client *cl);
void target_captcha(std::string &data, tkSock *sock);

void target_dl_list(std::string &data, tkSock *sock);
void target_dl_add(std::string &data, tkSock *sock);
void target_dl_del(std::string &data, tkSock *sock);
void target_dl_stop(std::string &data, tkSock *sock);
void target_dl_up(std::string &data, tkSock *sock);
void target_dl_down(std::string &data, tkSock *sock);
void target_dl_top(std::string &data, tkSock *sock);
void target_dl_bottom(std::string &data, tkSock *sock);
void target_dl_activate(std::string &data, tkSock *sock);
void target_dl_deactivate(std::string &data, tkSock *sock);
void target_dl_set(std::string &data, tkSock *sock);
void target_dl_get(std::string &data, tkSock *sock);

void target_pkg_add(std::string &data, tkSock *sock);
void target_pkg_del(std::string &data, tkSock *sock);
void target_pkg_up(std::string &data, tkSock *sock);
void target_pkg_down(std::string &data, tkSock *sock);
void target_pkg_top(std::string &data, tkSock *sock);
void target_pkg_bottom(std::string &data, tkSock *sock);
void target_pkg_exists(std::string &data, tkSock *sock);
void target_pkg_set(std::string &data, tkSock *sock);
void target_pkg_get(std::string &data, tkSock *sock);
void target_pkg_container(std::string &data, tkSock *sock);
void target_pkg_activate(std::string &data, tkSock *sock);
void target_pkg_deactivate(std::string &data, tkSock *sock);

void target_var_get(std::string &data, tkSock *sock);
void target_var_set(std::string &data, tkSock *sock);
void target_var_list(std::string &data, tkSock *sock);

void target_file_del(std::string &data, tkSock *sock);
void target_file_getpath(std::string &data, tkSock *sock);
void target_file_getsize(std::string &data, tkSock *sock);

void target_router_list(std::string &data, tkSock *sock);
void target_router_setmodel(std::string &data, tkSock *sock);
void target_router_set(std::string &data, tkSock *sock);
void target_router_get(std::string &data, tkSock *sock);

void target_premium_list(std::string &data, tkSock *sock);
void target_premium_set(std::string &data, tkSock *sock);
void target_premium_get(std::string &data, tkSock *sock);

void target_subscription_add(std::string &data, client *cl);
void target_subscription_del(std::string &data, client *cl);
void target_subscription_list(std::string &data, client *cl);

void target_captcha_request(std::string &data, tkSock *sock);
void target_captcha_solve(std::string &data, tkSock *sock);

#endif /*MGMT_THREAD_H_*/
