## TLS Cache
------------------------

This repository contains the starter code for the CS165 TLS Cache project. The file structure is as follows:
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
$ cd build

Run the server:
$ ./build/server 9999

Run the client (in another terminal):
$ ./build/client 127.0.0.1 9999
```

### How to build and run code
--------------------------
1. Add your code in src/client or src/server. 
2. Go to build/
3. `$ make`

### Scripts included
--------------------------
1. setup.sh should be run exactly once after you have downloaded code, and never again. It extracts and builds the dependencies in extern/, and builds and links the code in src/ with LibreSSL.
2. reset.sh reverts the directory to its initial state.


### References
--------------------------
1. LibTLS tutorial: https://github.com/bob-beck/libtls/blob/master/TUTORIAL.md
2. On Certificate Authorities: https://jamielinux.com/docs/openssl-certificate-authority/introduction.html
