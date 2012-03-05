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

/* $Id: app.c,v 1.3 2005/09/20 05:55:34 moriyoshi Exp $ */

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif

#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif

#ifdef HAVE_MISC_CONV

#ifdef HAVE_SECURITY_PAM_MISC_H
#include <security/pam_misc.h>
#endif

#ifdef HAVE_PAM_PAM_MISC_H
#include <pam/pam_misc.h>
#endif

#else /* HAVE_MISC_CONV */

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#endif /* HAVE_MISC_CONV */

#include "expr_parser.h"
#include "util.h"
#include "app.h"
#include "compat.h"

//vguerra
struct pam_cred {
  char *username;
  char *password;
};

static int resolve_item_type(int *retval, const char *name)
{
	if (!strcasecmp(name, "service")) {
		*retval = PAM_SERVICE;
		return 0;
	}

	if (!strcasecmp(name, "user")) {
		*retval = PAM_USER;
		return 0;
	}

	if (!strcasecmp(name, "prompt")) {
		*retval = PAM_USER_PROMPT;
		return 0;
	}

	if (!strcasecmp(name, "tty")) {
		*retval = PAM_TTY;
		return 0;
	}

	if (!strcasecmp(name, "ruser")) {
		*retval = PAM_RUSER;
		return 0;
	}

	if (!strcasecmp(name, "rhost")) {
		*retval = PAM_RHOST;
		return 0;
	}
	
	return 1;
}


static int resolve_flag_value(void *param, int *retval, const char *name, size_t name_len)
{
	if (!strncasecmp(name, "PAM_SILENT", name_len)) {
		*retval = PAM_SILENT;
		return 0;	
	}

	if (!strncasecmp(name, "PAM_DISALLOW_NULL_AUTHTOK", name_len)) {
		*retval = PAM_DISALLOW_NULL_AUTHTOK;
		return 0;	
	}

	if (!strncasecmp(name, "PAM_ESTABLISH_CRED", name_len)) {
		*retval = PAM_ESTABLISH_CRED;
		return 0;	
	}

	if (!strncasecmp(name, "PAM_REINITIALIZE_CRED", name_len)) {
		*retval = PAM_REINITIALIZE_CRED;
		return 0;	
	}

	if (!strncasecmp(name, "PAM_REFRESH_CRED", name_len)) {
		*retval = PAM_REFRESH_CRED;
		return 0;	
	}

	if (!strncasecmp(name, "PAM_CHANGE_EXPIRED_AUTHTOK", name_len)) {
		*retval = PAM_CHANGE_EXPIRED_AUTHTOK;
		return 0;	
	}

	return 1;
}

void pamtester_app_init(pamtester_app_t *params, const char *app_name)
{
	params->app_name = xstrdup(app_name);
	params->service = NULL;
	params->user = NULL;
	params->items = params->last_item = NULL;
	params->envs = params->last_env = NULL;
	params->verbose = 0;
	params->operations = NULL;
}

void pamtester_app_cleanup(pamtester_app_t *params)
{
	pamtester_op_t *op, *next_op = NULL;
	pamtester_pam_item_t *item, *next_item = NULL;

	xfree(params->app_name);
	xfree(params->service);
	xfree(params->user);

	for (item = params->items; item != NULL; item = next_item) {
		next_item = item->next;

		xfree(item->name);
		xfree(item);
	}

	for (item = params->envs; item != NULL; item = next_item) {
		next_item = item->next;

		xfree(item->name);
		xfree(item);
	}

	for (op = params->operations; op != NULL; op = next_op) {
		next_op = op->next;

		xfree(op->name);
		xfree(op->param);
		xfree(op);
	}
}

//vguerra 
/*
 *----------------------------------------------------------------------
 *
 * pam_conv --
 *
 * PAM conversation function
 * Accepts: number of messages
 *	    vector of messages
 *	    pointer to response return
 *	    application data
 *
 * Results:
 *      PAM_SUCCESS if OK, response vector filled in, else PAM_CONV_ERR
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static int
pam_conv(int msgs, const struct pam_message **msg, struct pam_response **resp, void *appdata)
{
    int i;
    struct pam_cred *cred = (struct pam_cred *) appdata;
    struct pam_response *reply = malloc(sizeof (struct pam_response) * msgs);

    for (i = 0; i < msgs; i++) {
        switch (msg[i]->msg_style) {
        case PAM_PROMPT_ECHO_ON:	/* assume want user name */
            reply[i].resp_retcode = PAM_SUCCESS;
            reply[i].resp = strdup(cred->username);
            break;

        case PAM_PROMPT_ECHO_OFF:	/* assume want password */
            reply[i].resp_retcode = PAM_SUCCESS;
            reply[i].resp = strdup(cred->password);
            break;

        case PAM_TEXT_INFO:
        case PAM_ERROR_MSG:
            reply[i].resp_retcode = PAM_SUCCESS;
            reply[i].resp = NULL;
            break;

        default:			/* unknown message style */
            free(reply);
            return PAM_CONV_ERR;
        }
    }
    *resp = reply;
    return PAM_SUCCESS;
}

int pamtester_app_run(pamtester_app_t *params)
{

	int err;
	char *err_msg = NULL;
	//struct pam_conv conv = { misc_conv, NULL };
	struct pam_conv conv; 
	pamtester_pam_item_t *item;
	pam_handle_t *pamh = NULL;
	pamtester_op_t *op;

	//vguerra 
	struct pam_cred cred;
	cred.username = (params->user == NULL ? "": params->user);
	cred.password = (params->password == NULL ? "" : params->password);
	conv.appdata_ptr = &cred;
	conv.conv = &pam_conv;

	if (params->verbose > 0) {
		fprintf(stderr, "%s: invoking pam_start(%s, %s, ...)\n", params->app_name, params->service, params->user);
	}

	if ((err = pam_start((params->service == NULL ? "": params->service),
			(params->user == NULL ? "": params->user), &conv, &pamh))) {
		err_msg = xstrdup("Initialization failure");
		pamh = NULL;
		goto out;
	}

	for (item = params->items; item != NULL; item = item->next) {
		int item_type;

		if (resolve_item_type(&item_type, item->name)) {
			err_msg = xmalloc(sizeof("Unknown item type \"\"") + strlen(item->name));
			sprintf(err_msg, "Unknown item type \"%s\"", item->name);
			goto out;
		}

		if ((err = pam_set_item(pamh, item_type, item->value))) {
			err_msg = xstrdup(pam_strerror(pamh, err));
			goto out;
		}
	}

	for (item = params->envs; item != NULL; item = item->next) {
		if ((err = pam_putenv(pamh, item->name))) {
			err_msg = xstrdup(pam_strerror(pamh, err));
			goto out;
		}
	}

	for (op = params->operations; op != NULL; op = op->next) {
		if (params->verbose > 0) {
			fprintf(stderr, "%s: performing operation - %s\n", params->app_name, op->name);
		}

		if (!strcasecmp(op->name, "authenticate")) {
			int flag = 0;

			if (op->param != NULL) {
				pamtester_expr_parser_t ep;

				if (pamtester_expr_parser_init(&ep)) {
					err_msg = xstrdup("failed to initialize expression parser");
					goto out;
				}

				pamtester_expr_parser_set_const_resolve_handler(&ep, resolve_flag_value, NULL);

				if (pamtester_expr_parser_parse(&ep, op->param)) {
					err_msg = ep.last_error;
					ep.last_error = NULL;
					pamtester_expr_parser_cleanup(&ep);
					goto out;
				}

				if (!ep.empty_result) {
					flag = ep.result;
				}

				pamtester_expr_parser_cleanup(&ep);
			}
			if ((err = pam_authenticate(pamh, flag))) {
				err_msg = xstrdup(pam_strerror(pamh, err));
				goto out;
			} else {
				printf("%s: successfully authenticated\n", params->app_name);
			}
		} else if (!strcasecmp(op->name, "setcred")) {
			int flag = 0;

			if (op->param != NULL) {
				pamtester_expr_parser_t ep;

				if (pamtester_expr_parser_init(&ep)) {
					err_msg = xstrdup("failed to initialize expression parser");
					goto out;
				}

				pamtester_expr_parser_set_const_resolve_handler(&ep, resolve_flag_value, NULL);

				if (pamtester_expr_parser_parse(&ep, op->param)) {
					err_msg = ep.last_error;
					ep.last_error = NULL;
					pamtester_expr_parser_cleanup(&ep);
					goto out;
				}

				if (!ep.empty_result) {
					flag = ep.result;
				}

				pamtester_expr_parser_cleanup(&ep);
			}

			if ((err = pam_setcred(pamh, flag))) {
				err_msg = xstrdup(pam_strerror(pamh, err));
				goto out;
			} else {
				printf("%s: credential info has successfully been set.\n", params->app_name);
			}
		} else if (!strcasecmp(op->name, "acct_mgmt")) {
			int flag = 0;

			if (op->param != NULL) {
				pamtester_expr_parser_t ep;

				if (pamtester_expr_parser_init(&ep)) {
					err_msg = xstrdup("failed to initialize expression parser");
					goto out;
				}

				pamtester_expr_parser_set_const_resolve_handler(&ep, resolve_flag_value, NULL);

				if (pamtester_expr_parser_parse(&ep, op->param)) {
					err_msg = ep.last_error;
					ep.last_error = NULL;
					pamtester_expr_parser_cleanup(&ep);
					goto out;
				}

				if (!ep.empty_result) {
					flag = ep.result;
				}

				pamtester_expr_parser_cleanup(&ep);
			}

			if ((err = pam_acct_mgmt(pamh, flag))) {
				err_msg = xstrdup(pam_strerror(pamh, err));
				goto out;
			} else {
				printf("%s: account management done.\n", params->app_name);
			}
		} else if (!strcasecmp(op->name, "open_session")) {
			int flag = 0;

			if (op->param != NULL) {
				pamtester_expr_parser_t ep;

				if (pamtester_expr_parser_init(&ep)) {
					err_msg = xstrdup("failed to initialize expression parser");
					goto out;
				}

				pamtester_expr_parser_set_const_resolve_handler(&ep, resolve_flag_value, NULL);

				if (pamtester_expr_parser_parse(&ep, op->param)) {
					err_msg = ep.last_error;
					ep.last_error = NULL;
					pamtester_expr_parser_cleanup(&ep);
					goto out;
				}

				if (!ep.empty_result) {
					flag = ep.result;
				}

				pamtester_expr_parser_cleanup(&ep);
			}

			if ((err = pam_open_session(pamh, flag))) {
				err_msg = xstrdup(pam_strerror(pamh, err));
				goto out;
			} else {
				printf("%s: sucessfully opened a session\n", params->app_name);
			}
		} else if (!strcasecmp(op->name, "close_session")) {
			int flag = 0;

			if (op->param) {
				pamtester_expr_parser_t ep;

				if (pamtester_expr_parser_init(&ep)) {
					err_msg = xstrdup("failed to initialize expression parser");
					goto out;
				}

				pamtester_expr_parser_set_const_resolve_handler(&ep, resolve_flag_value, NULL);

				if (pamtester_expr_parser_parse(&ep, op->param)) {
					err_msg = ep.last_error;
					ep.last_error = NULL;
					pamtester_expr_parser_cleanup(&ep);
					goto out;
				}

				if (!ep.empty_result) {
					flag = ep.result;
				}

				pamtester_expr_parser_cleanup(&ep);
			}

			if ((err = pam_close_session(pamh, flag))) {
				err_msg = xstrdup(pam_strerror(pamh, err));
				goto out;
			} else {
				printf("%s: session has successfully been closed.\n", params->app_name);
			}
		} else if (!strcasecmp(op->name, "chauthtok")) {
			int flag = 0;

			if (op->param != NULL) {
				pamtester_expr_parser_t ep;

				if (pamtester_expr_parser_init(&ep)) {
					err_msg = xstrdup("failed to initialize expression parser");
					goto out;
				}

				pamtester_expr_parser_set_const_resolve_handler(&ep, resolve_flag_value, NULL);

				if (pamtester_expr_parser_parse(&ep, op->param)) {
					err_msg = ep.last_error;
					ep.last_error = NULL;
					pamtester_expr_parser_cleanup(&ep);
					goto out;
				}

				if (!ep.empty_result) {
					flag = ep.result;
				}

				pamtester_expr_parser_cleanup(&ep);
			}

			if ((err = pam_chauthtok(pamh, flag))) {
				err_msg = xstrdup(pam_strerror(pamh, err));
				goto out;
			} else {
				printf("%s: authentication token altered successfully.\n", params->app_name);
			}

		} else {
			err_msg = xmalloc(sizeof("Unsupported operation \"\"") + strlen(op->name));
			sprintf(err_msg, "Unsupported operation \"%s\"", op->name);
			goto out;
		}
	}

out:
	if (err_msg != NULL) {
		fprintf(stderr, "%s: %s\n", params->app_name, err_msg);
		xfree(err_msg);
	}

	if (pamh != NULL) {
		pam_end(pamh, PAM_SUCCESS);
	}
	return (err_msg == NULL ? 0: 1);
}

