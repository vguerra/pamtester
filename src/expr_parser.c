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

/* $Id: expr_parser.c,v 1.1.1.1 2005/03/29 01:27:57 moriyoshi Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "util.h"
#include "expr_parser.h"

static char *build_error_message(const char *msg, const char *token, size_t token_val_len)
{
	char *retval;
	size_t msg_len = strlen(msg);
	size_t offset = 0;

	retval = xcalloc(msg_len + 2 + token_val_len + 2 + 1, sizeof(char));
	memcpy(&retval[offset], msg, msg_len);
	offset += msg_len;
	retval[offset++] = ' ';
	retval[offset++] = '\"';
	memcpy(&retval[offset], token, token_val_len);
	offset += token_val_len;
	retval[offset++] = '\"';
	retval[offset] = '\0';

	return retval;
}

static int pamtester_expr_parser_hdlr(pamtester_expr_parser_t *parser, pamtester_token_t token, const char *token_val, size_t token_val_len)
{
	switch (token) {
		case PAMTESTER_TOKEN_NUMERIC: {
			int val;
			char *next_ptr;

			if (parser->last_token == PAMTESTER_TOKEN_NUMERIC) {
				parser->last_error = build_error_message("unexpected token", token_val, token_val_len);
				return 1;
			}

			val = strtol(token_val, &next_ptr, 10);

			if (next_ptr - token_val != token_val_len
					|| errno == ERANGE || errno == EINVAL) {
				if (parser->const_resolve_hdlr == NULL
							|| parser->const_resolve_hdlr(parser->hdlr_param, &val, token_val, token_val_len)) {
					parser->last_error = build_error_message("undefined constant", token_val, token_val_len);
					return 1;
				}
			}

			if (parser->last_token == PAMTESTER_TOKEN_NOT) {
				val = ~val;
			}

			parser->val_filo[parser->val_filo_idx] = val;
			parser->val_filo_idx++;
		} break;

		case PAMTESTER_TOKEN_NOT:
			break;

		case PAMTESTER_TOKEN_OR: {
			if (parser->last_token != PAMTESTER_TOKEN_NUMERIC) {
				parser->last_error = build_error_message("unexpected token", token_val, token_val_len);
				return 1;
			}

			if (parser->val_filo_idx >= 2 && parser->op_filo_idx > 0) {
				switch (parser->op_filo[parser->op_filo_idx - 1]) {
					case PAMTESTER_TOKEN_NULL:
						break;

					case PAMTESTER_TOKEN_OR:
						parser->op_filo_idx--;
						parser->val_filo[parser->val_filo_idx - 2] |= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;

					case PAMTESTER_TOKEN_XOR:
						parser->op_filo_idx--;
						parser->val_filo[parser->val_filo_idx - 2] ^= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;

					case PAMTESTER_TOKEN_AND:
						parser->op_filo_idx--;
						parser->val_filo[parser->val_filo_idx - 2] &= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;

					default:
						break;
				}
			}
			parser->op_filo[parser->op_filo_idx++] = token;
		} break;

		case PAMTESTER_TOKEN_XOR: {
			if (parser->last_token != PAMTESTER_TOKEN_NUMERIC) {
				parser->last_error = build_error_message("unexpected token", token_val, token_val_len);
				return 1;
			}

			if (parser->val_filo_idx >= 2 && parser->op_filo_idx > 0) {
				switch (parser->op_filo[parser->op_filo_idx - 1]) {
					case PAMTESTER_TOKEN_NULL:
					case PAMTESTER_TOKEN_OR:
						break;

					case PAMTESTER_TOKEN_AND:
						parser->op_filo_idx--;
						parser->val_filo[parser->val_filo_idx - 2] &= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;
	
					case PAMTESTER_TOKEN_XOR:
						parser->op_filo_idx--;
						parser->val_filo[parser->val_filo_idx - 2] ^= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;

					default:
						break;
				}
			}

			parser->op_filo[parser->op_filo_idx++] = token;
		} break;

		case PAMTESTER_TOKEN_AND: {
			if (parser->last_token != PAMTESTER_TOKEN_NUMERIC) {
				parser->last_error = build_error_message("unexpected token", token_val, token_val_len);
				return 1;
			}

			if (parser->val_filo_idx >= 2 && parser->op_filo_idx > 0) {
				switch (parser->op_filo[parser->op_filo_idx - 1]) {
					case PAMTESTER_TOKEN_NULL:
					case PAMTESTER_TOKEN_OR:
					case PAMTESTER_TOKEN_XOR:
						break;

					case PAMTESTER_TOKEN_AND:
						parser->op_filo_idx--;
						parser->val_filo[parser->val_filo_idx - 2] &= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;

					default:
						break;
				}
			}

			parser->op_filo[parser->op_filo_idx++] = token;
		} break;

		case PAMTESTER_TOKEN_NULL: {
			while (parser->val_filo_idx >= 2 && parser->op_filo_idx > 0) {
				switch (parser->op_filo[--parser->op_filo_idx]) {
					case PAMTESTER_TOKEN_OR:
						parser->val_filo[parser->val_filo_idx - 2] |= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;

					case PAMTESTER_TOKEN_XOR:
						parser->val_filo[parser->val_filo_idx - 2] ^= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;

					case PAMTESTER_TOKEN_AND:
						parser->val_filo[parser->val_filo_idx - 2] &= parser->val_filo[parser->val_filo_idx - 1];
						parser->val_filo_idx--;
						break;

					default:
						break;
				}
			}

			if (parser->last_token == PAMTESTER_TOKEN_NULL) {
				parser->empty_result = 1;
			} else {
				parser->result = parser->val_filo[0];
			}
		} break;	

		default:
			break;
	}

	parser->last_token = token;

	return 0;
}

void pamtester_expr_parser_set_const_resolve_handler(pamtester_expr_parser_t *parser, pamtester_expr_parser_hdlr_fn_t hdlr, void *param)
{
	parser->const_resolve_hdlr = hdlr;
	parser->hdlr_param = param;
}

int pamtester_expr_parser_init(pamtester_expr_parser_t *parser)
{
	parser->result = 0;
	parser->val_filo_idx = 0;
	parser->op_filo_idx = 0;
	parser->const_resolve_hdlr = NULL;
	parser->last_error = NULL;
	parser->last_token = PAMTESTER_TOKEN_NULL;
	parser->empty_result = 0;
	return 0;
}

void pamtester_expr_parser_cleanup(pamtester_expr_parser_t *parser)
{
	xfree(parser->last_error);
}

int pamtester_expr_parser_parse(pamtester_expr_parser_t *parser, const char *expr)
{
	int err;
	const char *p;
	const char *token;
	int state = 0;

	p = token = expr;

	for (;;) {
		switch (state) {
			case 0:
				switch (*p) {
					case ' ':
					case '\t':
						p++;
						break;

					default:
						token = p;
						state = 1;
						break;

					case '\0':
						token = p;
						goto quit_loop;
				}
				break;

			case 1:
				switch (*p) {
					case '|':
					case '&':
					case '^':
					case '~':
						if (p - token > 0) {
							if ((err = pamtester_expr_parser_hdlr(parser, PAMTESTER_TOKEN_NUMERIC, token, (size_t)(p - token)))) {
								return err;
							}
						}
						state = 2;
						break;

					case ' ':
					case '\t':
						if (p - token > 0) {
							if ((err = pamtester_expr_parser_hdlr(parser, PAMTESTER_TOKEN_NUMERIC, token, (size_t)(p - token)))) {
								return err;
							}
						}
						state = 0;
						p++;
						break;

					case '\0':
						goto quit_loop;

					default:
						p++;
						break;
				}
				break;

			case 2:
				switch (*p) {
					case '|':
						if ((err = pamtester_expr_parser_hdlr(parser, PAMTESTER_TOKEN_OR, token, (size_t)(p - token) + 1))) {
							return err;
						}
						break;
					case '&':
						if ((err = pamtester_expr_parser_hdlr(parser, PAMTESTER_TOKEN_AND, token, (size_t)(p - token) + 1))) {
							return err;
						}
						break;
					case '^':
						if ((err = pamtester_expr_parser_hdlr(parser, PAMTESTER_TOKEN_XOR, token, (size_t)(p - token) + 1))) {
							return err;
						}
						break;
					case '~':
						if ((err = pamtester_expr_parser_hdlr(parser, PAMTESTER_TOKEN_NOT, token, (size_t)(p - token) + 1))) {
							return err;
						}
						break;
				}
				state = 0;
				p++;
				break;
		}
	}

quit_loop:
	if (p - token > 0) {
		if ((err = pamtester_expr_parser_hdlr(parser, PAMTESTER_TOKEN_NUMERIC, token, (size_t)(p - token)))) {
			return err;
		}
	}

	if ((err = pamtester_expr_parser_hdlr(parser, PAMTESTER_TOKEN_NULL, NULL, 0))) {
		return err;
	}

	return 0;
}
