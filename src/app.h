/*
 * pamtester - PAM testing program.
 * 
 * Copyright (c) 2004-2005, Moriyoshi Koizumi. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *   - Neither the name of the "pamtester" nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id: app.h,v 1.1.1.1 2005/03/29 01:27:57 moriyoshi Exp $ */

#ifndef _PAMTESTER_APP_H
#define _PAMTESTER_APP_H

typedef struct _pamtester_op_t pamtester_op_t;

struct _pamtester_op_t {
	pamtester_op_t *next;
	char *name;
	char *param;
};

typedef struct _pamtester_pam_item_t pamtester_pam_item_t;

struct _pamtester_pam_item_t {
	pamtester_pam_item_t *next;
	char *name;
	char *value;
};

typedef struct _pamtester_app_t {
	char *app_name;
	char *service;
	char *user;
        char *password;
	int verbose;
	pamtester_pam_item_t *items;
	pamtester_pam_item_t *last_item;
	pamtester_pam_item_t *envs;
	pamtester_pam_item_t *last_env;
	pamtester_op_t *operations;
} pamtester_app_t;

void pamtester_app_init(pamtester_app_t *params, const char *app_name);
void pamtester_app_cleanup(pamtester_app_t *params);
int pamtester_app_run(pamtester_app_t *params);

#endif /* _PAMTESTER_APP_H */

