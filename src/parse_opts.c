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

/* $Id: parse_opts.c,v 1.1.1.1 2005/03/29 01:27:57 moriyoshi Exp $ */

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

#include "util.h"
#include "parse_opts.h"

int pamtester_parse_opts(int argc, const char **argv, pamtester_opt_spec_t *options, void *param, int *last_comp, char **err_msg)
{
	int i;
	int state = 0;
	int retval = 0;
	pamtester_opt_spec_t *opt;
	size_t opt_idx = 0;
	size_t nopts = 0;
	unsigned int *occ_map = NULL;

	for (opt = options; opt->hdlr != NULL; opt++, nopts++);

	occ_map = xcalloc(nopts, sizeof(unsigned int));
	memset(occ_map, 0, sizeof(unsigned int) * nopts);

	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];

		if (!state) {
			if (arg[0] == '-') {
				if (arg[1] == '-') {
					for (opt_idx = 0; opt_idx < nopts; opt_idx++) {
						opt = &options[opt_idx];

						if (opt->long_name != NULL &&
								strcmp(opt->long_name, &arg[2]) == 0) {
							size_t occ = ++occ_map[opt_idx];

							switch (opt->occ_spec) {
								case '1':
								case '?':
									if (occ > 1) {
										*err_msg = xmalloc(sizeof("Option \"\" may not be specified twice or more") + strlen(opt->long_name));
										sprintf(*err_msg, "Option \"%s\" may not be specified twice or more", opt->long_name);
										retval = 1;
										goto out;
									}
									break;
							}							

							if (opt->nxt) {
								state = 1;
							} else {
								if ((retval = opt->hdlr(param, NULL))) {
									goto out;
								}
								opt = NULL;
							}
							break;
						}
					}
				} else {
					for (opt_idx = 0; opt_idx < nopts; opt_idx++) {
						opt = &options[opt_idx];

						if (opt->short_name != NULL &&
								opt->short_name[0] == arg[1]) {
							if (strcmp(opt->short_name, &arg[1]) == 0) {
								size_t occ = ++occ_map[opt_idx];

								switch (opt->occ_spec) {
									case '1':
									case '?':
										if (occ > 1) {
											*err_msg = xmalloc(sizeof("Option \"\" may not be specified twice or more") + strlen(opt->short_name));
											sprintf(*err_msg, "Option \"%s\" may not be specified twice or more", opt->short_name);
											retval = 1;
											goto out;
										}
										break;
								}
							
								if (opt->nxt) {
									state = 1;
								} else {
									if ((retval = opt->hdlr(param, NULL))) {
										goto out;
									}
									opt = NULL;
								}
								break;
							} else if (opt->short_name[1] == '\0' && opt->inl) {
								size_t occ = ++occ_map[opt_idx];

								switch (opt->occ_spec) {
									case '1':
									case '?':
										if (occ > 1) {
											*err_msg = xmalloc(sizeof("Option \"\" may not be specified twice or more") + strlen(arg));
											sprintf(*err_msg, "Option \"%s\" may not be specified twice or more", arg);
											retval = 1;
											goto out;
										}
										break;
								}

								if ((retval = opt->hdlr(param, &arg[2]))) {
									goto out;
								}
								opt = NULL;
								break;
							}
						}
					}
				}

				if (opt_idx >= nopts) {
					if (err_msg != NULL) {
						*err_msg = xmalloc(sizeof("Unknown option \"\"") + strlen(arg));
						sprintf(*err_msg, "Unknown option \"%s\"", arg);
					}
				}
			} else {
				break;
			}
		} else {
			if ((retval = opt->hdlr(param, arg))) {
				goto out;
			}
			state = 0;
		}
	}

	for (opt_idx = 0; opt_idx < nopts; opt_idx++) {
		size_t occ = occ_map[opt_idx];
		opt = &options[opt_idx];

		switch (opt->occ_spec) {
			case '1':
			case '+':
				if (occ == 0) {
					*err_msg = xmalloc(sizeof("Option \"\" must be specified") + strlen(opt->short_name));
					sprintf(*err_msg, "Option \"%s\" must be specified", opt->short_name);
					retval = 1;
					goto out;
				}
				break;
		}							
	}

out:
	xfree(occ_map);
	*last_comp = i;
	return retval;
}

