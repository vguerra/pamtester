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

/* $Id: compat.c,v 1.2 2005/06/12 09:37:15 moriyoshi Exp $ */

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

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "compat.h"
#include "util.h"

#ifndef HAVE_MISC_CONV
static char *pamtester_compat_readline(FILE *fp)
{
	char *buf = NULL;
	size_t buf_alloc = 0;
	size_t offset = 0;

	for (;;) {
		char *p;
		size_t nbytes_to_read;
		size_t nbytes_read;

		if (offset >= buf_alloc) {
			size_t new_buf_alloc = buf_alloc + 32;

			if (new_buf_alloc < buf_alloc) {
				fprintf(stderr, "Unable to allocate memory %f bytes",
						((double)buf_alloc) + 32);
			}

			buf = xrealloc(buf, new_buf_alloc, sizeof(char));
			buf_alloc = new_buf_alloc;
		}

		nbytes_to_read = buf_alloc - offset;
		if ((p = fgets(&buf[offset], nbytes_to_read, fp)) == NULL) {
			break;
		}

		nbytes_read = strlen(p);
		offset += nbytes_read;

		if (nbytes_read < nbytes_to_read) {
			break;
		}
	}

	/* strip trailing newlines */
	if (buf) {
		while (offset > 0) {
			--offset;

			if (buf[offset] == '\n' || buf[offset] == '\r') {
				buf[offset] = '\0';
			}
		}
	}

	return buf;
}

static void pamtester_compat_put_message(const char *msg, FILE *fp)
{
	fwrite(msg, sizeof(msg[0]), strlen(msg), fp);
	fflush(fp);
}

static char *pamtester_compat_prompt(const char *msg, int echo)
{
	char *retval;

	pamtester_compat_put_message(msg, stdout);
	setvbuf(stdin, NULL, _IONBF, 0);

	if (echo) {
		retval =  pamtester_compat_readline(stdin);
	} else {
		struct termios tstat;

		tcgetattr(fileno(stdin), &tstat);

		tstat.c_lflag &= ~ECHO;
		tcsetattr(fileno(stdin), TCSAFLUSH, &tstat);

		retval = pamtester_compat_readline(stdin);

		tstat.c_lflag |= ECHO;
		tcsetattr(fileno(stdin), TCSADRAIN, &tstat);
	}

	pamtester_compat_put_message("\n", stdout);

	return retval;
}

int pamtester_compat_misc_conv(int nmsgs,
		PAM_CONV_CONST struct pam_message **pmsgs,
		struct pam_response **resp, void *appdata)
{
	int i;
	fprintf(stderr, "entering pamtester_compat_misc_conv\n");
	*resp = xcalloc(nmsgs, sizeof(struct pam_response));

	for (i = 0; i < nmsgs; i++) {
		const struct pam_message *msg = &(*pmsgs)[i];

		(*resp)[i].resp_retcode = 0;

		switch (msg->msg_style) {
			case PAM_PROMPT_ECHO_OFF:
				(*resp)[i].resp = pamtester_compat_prompt(msg->msg, 0);
				break;

			case PAM_PROMPT_ECHO_ON:
				(*resp)[i].resp = pamtester_compat_prompt(msg->msg, 1);
				break;

			case PAM_ERROR_MSG:
				(*resp)[i].resp = NULL;
				pamtester_compat_put_message(msg->msg, stderr);
				break;

			case PAM_TEXT_INFO:
				(*resp)[i].resp = NULL;
				pamtester_compat_put_message(msg->msg, stdout);
				break;

			default:
				break;
		}
	}

	return PAM_SUCCESS;
}
#endif /* !HAVE_MISC_CONV */
