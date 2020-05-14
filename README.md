## TLS Cache
------------------------

This repository contains the starter code for the CS165 project. The directory structure is as follows:
```
certificates/	// Contains CA and server certificates.
scripts/	// Helper scripts.
src/		// Client and Server code. Add your code here.
cmake/		// CMake find script. 
extern/		// Required third party tools and libraries- LibreSSL & CMake.
licenses/	// Open source licenses for code used.
```


### Steps
-------------------------
1. Download and extract the code.
2. Run the following commands:
```
$ cd TLSCache
$ source scripts/setup.sh

Generate the server and client certificates
$ cd certificates
$ make
```
3. The plaintext server and client can be used as follows:
```
$ cd TLSCache

Run the server:
$ ./build/src/server 9999

Run the client (in another terminal):
$ ./build/src/client 127.0.0.1 9999
```

### How to build and run code
--------------------------
1. Add your code in `src/client` or `src/server`. 
2. Go to `build/`
3. Run `make`


### Scripts included
--------------------------
1. `setup.sh` should be run exactly once after you have downloaded code, and never again. It extracts and builds the dependencies in extern/, and builds and links the code in src/ with LibreSSL.
2. `reset.sh` reverts the directory to its initial state. It does not touch `src/` or `certificates/`. Run `make clean` in `certificates/` to delete the generated certificates.


### FAQ
--------------------------
1. How to generate CA, server and client certificates?

Go to `certificates/` and run `make` to generate all three certificates. 
```
root.pem	// Root CA certificate, the root of trust
server.crt	// Server's certificate, signed by the root CA using an intermediate CA certificate 
server.key	// Server's private key
```

2. The given starter code has only two files, `server/server.c` and `client/client.c`. I want to add another file to implement the proxy. How do I do it?

This project uses CMake to build code, and therefore has a `CMakeLists.txt` file located in `src/`. You can read the file as follows:
```
set(CLIENT_SRC	client/client.c)	# The CLIENT_SRC variable holds the names of all files that are a part of client's implementation. This is a client that has only one file in its implementation.
add_executable(client ${CLIENT_SRC})    # Tells CMake to compile all files listed in CLIENT_SRC into a binary named 'client'
target_link_libraries(client LibreSSL::TLS) # Asks CMake to link our executable to libtls
```
If you want to split your client's code into multiple files, you can modify `src/CMakeLists.txt` as follows:
```
set(CLIENT_SRC client/client_1.c client/client_2.c)
```
Your code can be split into any number of files as necessary, but remember that they are all compiled to a single runnable binary. 
If you want to create more binaries, you can copy the three lines explained above and change the variable and file names as necesary.


### Useful(!) Resources 
--------------------------
1. libTLS tutorial: https://github.com/bob-beck/libtls/blob/master/TUTORIAL.md
2. Official libTLS documentation: https://man.openbsd.org/tls\_init.3
3. LinuxConf AU 2017 slides: http://www.openbsd.org/papers/linuxconfau2017-libtls/
4. On Certificate Authorities: https://jamielinux.com/docs/openssl-certificate-authority/introduction.html


