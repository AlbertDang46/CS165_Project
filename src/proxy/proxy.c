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

/* server.c  - the "classic" example of a socket server */

/*
 * compile with gcc -o server server.c
 * or if you are on a crappy version of linux without strlcpy
 * thanks to the bozos who do glibc, do
 * gcc -c strlcpy.c
 * gcc -o server server.c strlcpy.o
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

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
const char PROXY_DIR[] = "./proxy_files/";
const char BLACKLIST_FILENAME[] = "Blacklisted_Objects";
const unsigned int NUM_BLOOM_BITS = 303658;
const unsigned int NUM_BLOOM_INTS = 10000;
const unsigned int NUM_BLOOM_HASHES = 5;

/****
 * Insert a string into a bloom filter
 * bloom_filter: An array of ints. Each bit in each int is a slot in the bloom filter
 * num_hashes: Number of hash functions
 * num_bits: Number of slots in the bloom filter
 * return: Nothing
 ****/
static void insert_bloom_filter(unsigned int bloom_filter[], unsigned int num_hashes, unsigned int num_bits, char str[]) {
	unsigned int int_bit_size = sizeof(unsigned int) * 8;
	
	unsigned int i;
	for (i = 0; i < num_hashes; i++) {
		uint32_t hash[4];
		MurmurHash3_x86_32(str, strlen(str), i + 46, hash);
		unsigned int index = (*hash % num_bits) / int_bit_size;
		bloom_filter[index] |= (1 << (*hash % int_bit_size));	
	}
}

/****
 * Check if a string has been inserted into a bloom filter
 * bloom_filter: An array of ints. Each bit in each int is a slot in the bloom filter
 * num_hashes: Number of hash functions
 * num_bits: Number of slots in the bloom filter
 * return: 1 if string has been inserted. 0 if string has not been inserted
 ****/
static unsigned int search_bloom_filter(unsigned int bloom_filter[], unsigned int num_hashes, unsigned int num_bits, char str[]) {
	unsigned int int_bit_size = sizeof(unsigned int) * 8;
	
	unsigned int i;
	for (i = 0; i < num_hashes; i++) {
		uint32_t hash[4];
		MurmurHash3_x86_32(str, strlen(str), i + 46, hash);
		unsigned int index = (*hash % num_bits) / int_bit_size;
		if ((bloom_filter[index] & (1 << (*hash % int_bit_size))) == 0)
			return 0;
	}

	return 1;
}

/****
 * Configure TLS client for connection with a server, but does not connect to server yet
 * return: A struct tls* that has been configured for a TLS connection with a server
 ****/
static struct tls* setupTLSClient() {
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

	return ctx;
}

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port portnumber -servername:serverportnumber\n", __progname);
	exit(1);
}

static void kidhandler(int signum) {
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}


int main(int argc, char *argv[])
{
	if (argc != 4 || strcmp(argv[1], "-port") != 0)			// Check if executable is used properly
                usage();

	/**** Create bloom filters for each proxy  ****/
	unsigned int bloom_filters[NUM_PROXIES][NUM_BLOOM_INTS];
	memset(bloom_filters, 0, sizeof(bloom_filters));
	printf("Created bloom filters for blacklisted objects for each proxy server\n");
	printf("\n");
	/**** End create bloom filters for each proxy ****/

	/**** Insert blacklisted objects into bloom filters for their respective proxy servers ****/
	char blacklist_filename[255];
	strcpy(blacklist_filename, PROXY_DIR);
	strcat(blacklist_filename, BLACKLIST_FILENAME);
	
	FILE *blacklist;
	blacklist = fopen(blacklist_filename, "r");

	printf("Entering blacklisted objects into bloom filter:\n");
	char blacklisted_object[255];
	memset(blacklisted_object, 0, sizeof(blacklisted_object));
	
	while (fscanf(blacklist, "%s", blacklisted_object) > 0) {
		/**** Rendezvous hashing to select which proxy's bloom filter the object will be entered into ****/
	        uint32_t hashes[NUM_PROXIES];
		unsigned int i;
		for (i = 0; i < NUM_PROXIES; i++) {                     // Calculate hashes for object_name.PROXY_NAMES[i]
                        char str[255];
                        memset(str, 0, sizeof(str));
                        strcpy(str, blacklisted_object);
                        strcat(str, PROXY_NAMES[i]);
                        MurmurHash3_x86_32(str, strlen(str), 42, &hashes[i]);
                        printf("%s|%s: %x\n", blacklisted_object, PROXY_NAMES[i], hashes[i]);
                }

                unsigned int max_index = 0;
                for (i = 0; i < NUM_PROXIES; i++) {                     // Get index of proxy that produced the highest hash value
                        if (hashes[i] > hashes[max_index])
                                max_index = i;
                }
		/**** End rendezvous hashing to select which proxy's bloom filter the object will be entered into ****/

		insert_bloom_filter(&bloom_filters[max_index][0], NUM_BLOOM_HASHES, NUM_BLOOM_BITS, blacklisted_object);		
		printf("Entered %s into proxy %s's bloom filter\n", blacklisted_object, PROXY_NAMES[max_index]);
	        printf("\n");
		memset(blacklisted_object, 0, sizeof(blacklisted_object));
	}
	fclose(blacklist);
	/**** End insert blacklisted objects into bloom filters for their respective proxy servers ****/

	/**** Configure TLS connection to client ****/
	struct tls_config *cfg = NULL;
	struct tls *ctx = NULL, *cctx = NULL;
	uint8_t *mem;
	size_t mem_len;

	if (tls_init() != 0)
		err(1, "tls_init:");
	printf("Initialized TLS\n");

	if ((cfg = tls_config_new()) == NULL)
		err(1, "tls_config_new:");
	printf("Got TLS config\n");

	if ((mem = tls_load_file("root.pem", &mem_len, NULL)) == NULL)
		err(1, "tls_load_file(ca):");
	if (tls_config_set_ca_mem(cfg, mem, mem_len) != 0)
		err(1, "tls_config_set_ca_mem:");
	printf("Set root certificate\n");

	if ((mem = tls_load_file("server.crt", &mem_len, NULL)) == NULL)
		err(1, "tls_load_file(server):");
	if (tls_config_set_cert_mem(cfg, mem, mem_len) != 0)
		err(1, "tls_config_set_cert_mem:");
	printf("Set proxy server certificate\n");	

	if ((mem = tls_load_file("server.key", &mem_len, NULL)) == NULL)
		err(1, "tls_load_file(serverkey):");
	if (tls_config_set_key_mem(cfg, mem, mem_len) != 0)
		err(1, "tls_config_set_key_mem:");
	printf("Set proxy server private key\n");	

	if ((ctx = tls_server()) == NULL)
		err(1, "tls_server:");
	printf("Got TLS proxy server\n");

	if (tls_configure(ctx, cfg) != 0)
		err(1, "tls_configure: %s", tls_error(ctx));
	printf("Configured TLS proxy server with TLS config\n");
	/**** End configure TLS connection to client ****/

	/**** Configure TCP connection with client ****/
	struct sockaddr_in sockname, client;
	char *ep;
	struct sigaction sa;
	int sd;
	socklen_t clientlen;
	u_short port;
	pid_t pid;
	u_long p;

	/*
	 * first, figure out what port we will listen on - it should
	 * be our first parameter.
	 */
	errno = 0;
        p = strtoul(argv[2], &ep, 10);
        if (*argv[2] == '\0' || *ep != '\0') {
		/* parameter wasn't a number, or was empty */
		fprintf(stderr, "%s - not a number\n", argv[2]);
		usage();
	}
        if ((errno == ERANGE && p == ULONG_MAX) || (p > USHRT_MAX)) {
		/* It's a number, but it either can't fit in an unsigned
		 * long, or is too big for an unsigned short
		 */
		fprintf(stderr, "%s - value out of range\n", argv[2]);
		usage();
	}
	/* now safe to do this */
	port = p;

	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd = socket(AF_INET,SOCK_STREAM,0);
	if (sd == -1)
		err(1, "socket failed");

	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");

	if (listen(sd, 3) == -1)
		err(1, "listen failed");

	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 */


	/*
	 * first, let's make sure we can have children without leaving
	 * zombies around when they die - we can do this by catching
	 * SIGCHLD.
	 */
	sa.sa_handler = kidhandler;
        sigemptyset(&sa.sa_mask);
	/*
	 * we want to allow system calls like accept to be restarted if they
	 * get interrupted by a SIGCHLD
	 */
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1)
                err(1, "sigaction failed");

	printf("Proxy server up and listening for connections on port %u\n", port);
	/**** End configure TCP connection with client ****/

	for(;;) {
		/**** TCP connection with client ****/
		int clientsd;
		clientlen = sizeof(&client);
		clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		/**** TCP connection with client ****/
	
		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time.
		 */

		pid = fork();
		if (pid == -1)
		     err(1, "fork failed");

		if(pid == 0) {
			/**** TLS connection with client ****/
			if (tls_accept_socket(ctx, &cctx, clientsd) != 0)
				err(1, "tls_accept_socket: %s", tls_error(ctx));
			printf("Accepted TLS socket\n");
			printf("\n");	
			/**** End TLS connection with client ****/

			/**** Receive request for object from client ****/
			char request[255];
			char *proxy_name;
			char *object_name;
			memset(request, 0, sizeof(request));
			
			if (tls_read(cctx, request, sizeof(request)) < 0)
				err(1, "tls_read: %s", tls_error(cctx));
			
			proxy_name = strtok(request, " ");
			object_name = strtok(NULL, " ");
			printf("Received request for proxy server %s for %s\n", proxy_name, object_name); 
			/**** End receive request for object from client ****/

			/**** Check respective proxy's blacklist for object ****/
			unsigned int filter_index;
			for (filter_index = 0; filter_index < NUM_PROXIES; filter_index++) {		// Check if requested proxy server is in list
				if (strcmp(PROXY_NAMES[filter_index], proxy_name) == 0)
					break;
			}
			
			if (search_bloom_filter(&bloom_filters[filter_index][0], NUM_BLOOM_HASHES, NUM_BLOOM_BITS, object_name) == 1) {
				char deny[] = "****black-listed****\n";					// Requested object was blacklisted
				if (tls_write(cctx, deny, strlen(deny)) < 0)
					err(1, "tls_write: %s", tls_error(cctx));
				printf("Request was for black-listed object %s. Denied request\n", object_name);
				printf("\n");
			/**** End check respective proxy's blacklist for object ****/
			} else {
				/**** Send requested object to client ****/
				FILE *fp;
				char filename[255];
				char content[255];
				memset(filename, 0, sizeof(filename));
				memset(content, 0, sizeof(content));
				strcpy(filename, PROXY_DIR);
				strcat(filename, object_name);

				/**** Get requested object from server if object is not in proxy server's cache ****/
				if ((fp = fopen(filename, "r")) == NULL) {
					printf("Requested object is not in proxy server cache. Requesting object from server\n");
					printf("\n");

					char *server_name;
					char *server_port;

					server_name = strtok(argv[3], ":");
					server_name++;
					server_port = strtok(NULL, ":");

					struct tls *server_ctx = setupTLSClient();
					if (tls_connect(server_ctx, server_name, server_port) != 0)
			                        err(1, "tls_connect: %s", tls_error(ctx));
                			printf("Connected to server\n");
                			printf("\n"); 

					if (tls_write(server_ctx, object_name, strlen(object_name)) < 0)
			                        err(1, "tls_write: %s", tls_error(ctx));
			                printf("Sent request to server %s for %s\n", server_name, object_name);
			
					/**** Put requested object into proxy server's cache  ****/
					fp = fopen(filename, "a+");
					char response[255];
					memset(response, 0, sizeof(response));
			                printf("Server response:\n");
			                while (tls_read(server_ctx, response, sizeof(response)) > 0){
						fwrite(response, sizeof(char), sizeof(response), fp);
			                        printf("%s", response);
			                        memset(response, 0, sizeof(response));
			                }
					printf("\n");
					printf("Put %s in proxy %s's cache\n", object_name, proxy_name);
					/**** End put requested object into proxy server's cache  ****/

			                fseek(fp, 0, SEEK_SET);
					printf("\n");
				}
				/**** End get requested object from server if object is not in proxy server's cache ****/
	
				printf("Sent file content:\n");
				while (fread(content, sizeof(char), sizeof(content), fp) > 0) {
					if (tls_write(cctx, content, strlen(content)) < 0)
						err(1, "tls_write: %s", tls_error(cctx));
					printf("%s", content);
					memset(content, 0, sizeof(content));
				}
				fclose(fp);
				printf("\n");
				/**** End send requested object to client ****/
			}

			/**** Close TLS connection to client ****/
			if (tls_close(cctx) != 0)
				err(1, "tls_close: %s", tls_error(cctx));
			printf("Closed TLS client\n");
			
			tls_free(cctx);
			printf("Freed TLS client\n"); 

			tls_free(ctx);
			printf("Freed TLS proxy server\n");

			tls_config_free(cfg);
			printf("Freed TLS config\n");
			printf("\n");

			close(clientsd);
			/**** End close TLS connection to client ****/

			exit(0);
		}

		close(clientsd);
	}
}
