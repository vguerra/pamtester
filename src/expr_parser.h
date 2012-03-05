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

/* $Id: expr_parser.h,v 1.1.1.1 2005/03/29 01:27:57 moriyoshi Exp $ */

#ifndef _PAMTESTER_EXPR_PARSER_H
#define _PAMTESTER_EXPR_PARSER_H

typedef enum _pamtester_token_t {
	PAMTESTER_TOKEN_NULL,
	PAMTESTER_TOKEN_NUMERIC,
	PAMTESTER_TOKEN_OR,
	PAMTESTER_TOKEN_AND,
	PAMTESTER_TOKEN_XOR,
	PAMTESTER_TOKEN_NOT
} pamtester_token_t;

typedef int (*pamtester_expr_parser_hdlr_fn_t)(void *param, int *retval, const char *token, size_t token_len);

typedef struct _pamtester_expr_parser_t {
	int result;
	void *hdlr_param;
	pamtester_expr_parser_hdlr_fn_t const_resolve_hdlr;
	int val_filo[16];
	int val_filo_idx;
	pamtester_token_t op_filo[16];
	int op_filo_idx;
	pamtester_token_t last_token;
	char *last_error;
	int empty_result;
} pamtester_expr_parser_t;

void pamtester_expr_parser_set_const_resolve_handler(pamtester_expr_parser_t *parser, pamtester_expr_parser_hdlr_fn_t hdlr, void *param);
int pamtester_expr_parser_init(pamtester_expr_parser_t *parser);
void pamtester_expr_parser_cleanup(pamtester_expr_parser_t *parser);
int pamtester_expr_parser_parse(pamtester_expr_parser_t *parser, const char *expr);

#endif /* _PAMTESTER_EXPR_PARSER_H */
