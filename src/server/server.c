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


const char SERVER_DIR[] = "./server_files/";

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port portnumber\n", __progname);
	exit(1);
}

static void kidhandler(int signum) {
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}


int main(int argc,  char *argv[])
{
	if (argc != 3 || strcmp(argv[1], "-port") != 0)				// Check if executable is used properly
                usage();
	
	/**** Configure TLS connection to proxy server ****/
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

	if ((mem = tls_load_file("../../certificates/root.pem", &mem_len, NULL)) == NULL)
		err(1, "tls_load_file(ca):");
	if (tls_config_set_ca_mem(cfg, mem, mem_len) != 0)
		err(1, "tls_config_set_ca_mem:");
	printf("Set root certificate\n");

	if ((mem = tls_load_file("../../certificates/server.crt", &mem_len, NULL)) == NULL)
		err(1, "tls_load_file(server):");
	if (tls_config_set_cert_mem(cfg, mem, mem_len) != 0)
		err(1, "tls_config_set_cert_mem:");
	printf("Set server certificate\n");	

	if ((mem = tls_load_file("../../certificates/server.key", &mem_len, NULL)) == NULL)
		err(1, "tls_load_file(serverkey):");
	if (tls_config_set_key_mem(cfg, mem, mem_len) != 0)
		err(1, "tls_config_set_key_mem:");
	printf("Set server private key\n");	

	if ((ctx = tls_server()) == NULL)
		err(1, "tls_server:");
	printf("Got TLS server\n");

	if (tls_configure(ctx, cfg) != 0)
		err(1, "tls_configure: %s", tls_error(ctx));
	printf("Configured TLS server with TLS config\n");
	/**** End configure TLS connection to proxy server ****/

	/**** Configure TCP connection with proxy server ****/
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

	printf("Server up and listening for connections on port %u\n", port);
	/**** End configure TCP connection with proxy server ****/	

	for(;;) {
		/**** TCP connection with proxy server ****/
		int clientsd;
		clientlen = sizeof(&client);
		clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		/**** End TCP connection with proxy server ****/

		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time.
		 */

		pid = fork();
		if (pid == -1)
		     err(1, "fork failed");

		if(pid == 0) {
			/**** TLS connection with proxy server ****/
			if (tls_accept_socket(ctx, &cctx, clientsd) != 0)
				err(1, "tls_accept_socket: %s", tls_error(ctx));
			printf("Accepted TLS socket\n");
			printf("\n");
			/**** TLS connection with proxy server ****/
			
			/**** Receive request for object from proxy server ****/
			char request[255];
			memset(request, 0, sizeof(request));
			
			if (tls_read(cctx, request, sizeof(request)) < 0)
				err(1, "tls_read: %s", tls_error(cctx));
			
			printf("Received request for server for %s\n", request); 
			/**** End receive request for object from proxy server ****/

			/**** Send requested object to client ****/
			FILE *fp;
			char filename[255];
			char content[255];
			memset(filename, 0, sizeof(filename));
			memset(content, 0, sizeof(content));
			strcpy(filename, SERVER_DIR);
			strcat(filename, request);
	 
			if ((fp = fopen(filename, "r")) == NULL)
				err(1, "File not found!");

			printf("Sent file content:\n");
			while (fread(content, sizeof(char), sizeof(content), fp) > 0) {
				if (tls_write(cctx, content, strlen(content)) < 0)
					err(1, "tls_write: %s", tls_error(ctx));
				printf("%s", content);
				memset(content, 0, sizeof(content));
			}
			fclose(fp);
			printf("\n");	
			/**** End send requested object to client ****/

			/**** Close TLS connection to proxy server ****/
			if (tls_close(cctx) != 0)
				err(1, "tls_close: %s", tls_error(cctx));
			printf("Closed TLS client\n");
			
			tls_free(cctx);
			printf("Freed TLS client\n"); 

			tls_free(ctx);
			printf("Freed TLS server\n");

			tls_config_free(cfg);
			printf("Freed TLS config\n");
			printf("\n");

			close(clientsd);
			/**** End close TLS connection to proxy server ****/

			exit(0);
		}

		close(clientsd);
	}
}
