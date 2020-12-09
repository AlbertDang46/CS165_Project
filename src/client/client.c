/*
 * Copyright (c) 2008 Bob Beck <beck@obtuse.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* client.c  - the "classic" example of a socket client */
#include <arpa/inet.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tls.h>
#include "murmur3.h"


const unsigned int NUM_PROXIES = 6;
const char *PROXY_NAMES[] = {"one", "two", "three", "four", "five", "six"};

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port proxyportnumber filename\n", __progname);
	exit(1);
}

int main(int argc, char *argv[])
{
	if (argc != 4 || strcmp(argv[1], "-port") != 0)
        	usage();

	FILE *fp;
  	uint32_t hashes[NUM_PROXIES];
        char object_name[255];
        char request[255];
        char response[255];
        memset(object_name, 0, sizeof(object_name));
        memset(request, 0, sizeof(request));
        memset(response, 0, sizeof(response));

        if((fp = fopen(argv[3], "r")) == NULL)
                err(1, "File not found!");

        while (fscanf(fp, "%s", object_name) > 0) {			// New TLS connection for each object requested in file	
		struct tls_config *cfg = NULL;
		struct tls *ctx = NULL;
	
		if (tls_init() != 0)
			err(1, "tls_init:");
		printf("Initialized TLS\n");
	
		if ((cfg = tls_config_new()) == NULL)
			err(1, "tls_config_new:");
		printf("Got TLS config\n");
	
		if (tls_config_set_ca_file(cfg, "root.pem") != 0)
			err(1, "tls_config_set_ca_file:");
		printf("Set root certificate\n");
	
		if ((ctx = tls_client()) == NULL)
			err(1, "tls_client:");
		printf("Got TLS client\n");
	
		if (tls_configure(ctx, cfg) != 0)
			err(1, "tls_configure: %s", tls_error(ctx));
		printf("Configured TLS client with TLS config\n");
	
		if (tls_connect(ctx, "localhost", argv[2]) != 0)
			err(1, "tls_connect: %s", tls_error(ctx));
		printf("Connected to proxy server\n");
		printf("\n");

		printf("Computing hashes for each objectname|proxyname\n");
		unsigned int i;
		for (i = 0; i < NUM_PROXIES; i++) {			// Calculate hashes for object_name.PROXY_NAMES[i]
			char str[255];
			memset(str, 0, sizeof(str));
			strcpy(str, object_name);
			strcat(str, PROXY_NAMES[i]);
			MurmurHash3_x86_32(str, strlen(str), 42, &hashes[i]);
			printf("%s|%s: %x\n", object_name, PROXY_NAMES[i], hashes[i]); 
		}				
		printf("\n");		

		unsigned int max_index = 0;
		for (i = 0; i < NUM_PROXIES; i++) {			// Get index of proxy that produced the highest hash value
			if (hashes[i] > hashes[max_index])
				max_index = i;
		}

		strcpy(request, PROXY_NAMES[max_index]);		// Create request with form "PROXY_NAME OBJECT_NAME"
		strcat(request, " ");
		strcat(request, object_name); 

		if (tls_write(ctx, request, strlen(request)) < 0)
			err(1, "tls_write: %s", tls_error(ctx));
		printf("Sent request to proxy server %s for %s\n", PROXY_NAMES[max_index], object_name);

		printf("Proxy server response:\n");
		while (tls_read(ctx, response, sizeof(response)) > 0){
			printf("%s", response);
			memset(response, 0, sizeof(response));
		}
		printf("\n");

		if (tls_close(ctx) != 0)
			err(1, "tls_close: %s", tls_error(ctx));
		printf("Closed TLS client\n");
	
		tls_free(ctx);
		printf("Freed TLS client\n");
	
		tls_config_free(cfg);
		printf("Freed TLS config\n");
		printf("\n");

		memset(object_name, 0, sizeof(object_name));
       		memset(request, 0, sizeof(request));
                memset(response, 0, sizeof(response));
        }

	return(0);
}
