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

/* $Id: util.c,v 1.1.1.1 2005/03/29 01:27:57 moriyoshi Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "util.h"

const char *xbasename(const char *path)
{
	const char *p;

	for (p = path + strlen(path); p > path;) {
		if (*(--p) != '/') {
			break;
		}
	}

	for (;p >= path; p--) {
		if (*p == '/') {
			break;
		}
	}

	return (p + 1);
}

void xfree(void *ptr)
{
	if (ptr != NULL) {
		free(ptr);
	}
}

void *xrealloc(void *ptr, size_t nmemb, size_t size)
{
	void *retval;
	size_t total = nmemb * size;

	if (((double)size * (int)(nmemb & (((size_t)-1) >> 1))) != total
			|| NULL == (retval = (ptr == NULL ? malloc(total): realloc(ptr, total)))) {
		fprintf(stderr, "FATAL: Allocation failure.\n");
		exit(-1);
	}

	return retval;
}

void *xmalloc(size_t sz)
{
	return xrealloc(NULL, sz, 1);
}

void *xcalloc(size_t nmemb, size_t size)
{
	return xrealloc(NULL, nmemb, size);
}

char *xstrdup(const char *str)
{
	size_t len = strlen(str);
	void *ptr = xcalloc(len + 1, sizeof(char));
	memcpy(ptr, str, len + 1);
	return ptr;
}

char *xstrndup(const char *str, size_t len)
{
	size_t actual_len = strlen(str);
	char *ptr;

	if (actual_len < len) {
		len = actual_len;
	}

	ptr = xcalloc(len + 1, sizeof(char));
	memcpy(ptr, str, len);
	ptr[len] = '\0';
	return ptr;
}

