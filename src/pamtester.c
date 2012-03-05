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

/* $Id: pamtester.c,v 1.1.1.1 2005/03/29 01:27:57 moriyoshi Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif

#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif

#include "parse_opts.h"
#include "util.h"
#include "app.h"

static int opt_hdlr_verbose(void *param, const char *val)
{
	pamtester_app_t *x = (pamtester_app_t *)param;

	x->verbose = (val == NULL ? 1: strtoul(val, NULL, 10));

	return 0;
}

static int opt_hdlr_item(void *param, const char *val)
{
	pamtester_app_t *x = (pamtester_app_t *)param;
	char *item_name, *item_val;
	pamtester_pam_item_t *next_item;

	item_name = xstrdup(val);

	item_val = strchr(item_name, '=');

	if (item_val != NULL) {
		*item_val = '\0';
		item_val++;
	}

	next_item = xmalloc(sizeof(pamtester_pam_item_t));
	next_item->next = NULL;
	next_item->name = item_name;
	next_item->value = item_val;

	if (x->last_item == NULL) {
		x->items = x->last_item = next_item;
	} else {
		x->last_item->next = next_item;
		x->last_item = next_item;
	}

	return 0;
}

static int opt_hdlr_env(void *param, const char *val)
{
	pamtester_app_t *x = (pamtester_app_t *)param;
	char *env_name, *env_val;
	pamtester_pam_item_t *next_env;

	env_name = xstrdup(val);

	env_val = strchr(env_name, '=');

	if (env_val != NULL) {
		env_val++;
	}

	next_env = xmalloc(sizeof(pamtester_pam_item_t));
	next_env->next = NULL;
	next_env->name = env_name;
	next_env->value = NULL;

	if (x->last_env == NULL) {
		x->envs = x->last_env = next_env;
	} else {
		x->last_env->next = next_env;
		x->last_env = next_env;
	}

	return 0;
}

static int opt_hdlr_password(void *param, const char *val)
{

	pamtester_app_t *x = (pamtester_app_t *)param;
	x->password = xstrdup(val);
	return 0;
}

pamtester_opt_spec_t options[] = {
	{ "I", "item", 1, 1, '*', opt_hdlr_item },
	{ "E", "env", 1, 1, '*', opt_hdlr_env },
	{ "v", "verbose", 1, 0, '?', opt_hdlr_verbose },
	{ "p", "password", 1, 1, '*', opt_hdlr_password },
	{ NULL, NULL, 0, 0, 0, NULL }
};

int main(int argc, const char **argv)
{
	int err;
	char *err_msg;
	pamtester_app_t params;
	int op_idx;
	pamtester_op_t *operations, *next_op, *prev_op;
	const char *prog_name = xbasename(argv[0]);
	int i;
	if (argc < 2) {
		fprintf(stderr, "usage: %s [-Eenv=value] [-Iparam=value] service user op_name ...\n", prog_name);
		exit(-1);
	}

	pamtester_app_init(&params, prog_name);

	if (pamtester_parse_opts(argc, argv, options, &params, &op_idx, &err_msg)) {
		fprintf(stderr, "%s: %s\n", prog_name, err_msg);
		xfree(err_msg);
		err = -1;
		goto out;
	}

	if (op_idx >= argc) {
		fprintf(stderr, "%s: please specify service name.\n", prog_name);
		err = -1;
		goto out;
	}

	params.service = xstrdup(argv[op_idx++]);

	if (op_idx >= argc) {
		fprintf(stderr, "%s: please specify user name.\n", prog_name);
		err = -1;
		goto out;
	}

	params.user = xstrdup(argv[op_idx++]);

	if (op_idx >= argc) {
		fprintf(stderr, "%s: specify one or more operations.\n", prog_name);
		err = -1;
		goto out;
	}

	operations = prev_op = NULL;

	for (; op_idx < argc; op_idx++) {
		char *op_spec = (char *)argv[op_idx];
		char *op_name;
		char *op_param;

		if (NULL != (op_param = strchr(op_spec, '('))) {
			char *op_name_end = op_param;
			char *op_param_end;

			op_name = op_spec;

			if (op_param == op_spec
					|| *(op_param_end = op_param + strlen(op_param) - 1) != ')') {
				fprintf(stderr, "%s: incorrect op_name spec \"%s\".", prog_name, op_spec);
				err = -1;
				goto out;
			}

			while (op_param_end-- > op_spec) {
				if (*op_param_end != ' ' && *op_param_end != '\t')
					break;
			}

			while (++op_param < op_param_end) {
				if (*op_param != ' ' && *op_param != '\t')
					break;
			}

			while (op_name_end-- > op_spec) {
				if (*op_name_end != ' ' && *op_name_end != '\t')
					break;
			}

			op_name = xstrndup(op_name, (size_t)(op_name_end - op_name) + 1);
			op_param = xstrndup(op_param, (size_t)(op_param_end - op_param) + 1);
		} else {
			op_name = xstrdup(op_spec);
		}

		next_op = xmalloc(sizeof(pamtester_op_t));
		next_op->next = NULL;
		next_op->name = op_name;
		next_op->param = op_param;

		if (prev_op != NULL) {
			prev_op->next = next_op;
		} else {
			operations = next_op;
		}

		prev_op = next_op;
	}

	params.operations = operations;

	err = pamtester_app_run(&params);

out:
	pamtester_app_cleanup(&params);

	return err;
}
