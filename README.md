# CS165 Project

## To compile:
* Replace /src/ in original TCPSocket_iii with the /src/ in this folder
* Run make in /build/ 
* Executables "server", "proxy", and "client" will appear in /build/src/

## To run:
* You must make a folder called "server_files" in the same directory the executable "server" is in. "server_files" is the server cache.
	* an example "server_files" folder will be provided
* You must make a folder called "proxy_files" in the same directory the executable "proxy" is in. "proxy_files" is a shared cache for the proxy servers.
	* You must make a file called "Blacklisted_Objects" in "proxy_files". "Blacklisted_Objects" contains all the blacklisted objects separated by new lines
	* an example "proxy_files" folder will be provided
* You must have "root.pem", "server.crt", and "server.key" in /certificates/
	* an example "root.pem", "server.crt", and "server.key" will be provided 
* Run "server" with the command ./server -port portnumber
	* portnumber is the port "server" listens on
* Run "proxy" with the command ./proxy -port portnumber -servername:serverportnumber
	* portnumber is the port "proxy" listens on
	* servername is the name/IP address of the server. Use "localhost" for servername
	* serverportnumber is the port "server" listens on
* Run "client" with the command ./client -port proxyportnumber filename
	* proxyportnumber is the port "proxy" listens on
	* filename is the name of the file that contains all the objects that "client" will be requesting from "proxy"
		* objects must be separated by new lines
		* an example "object_list.txt" will be provided
* All provided files are in /build/src/ including the compiled executables

## Example compile and run:
* Start from root of project folder
1. source scripts/setup.sh
2. run "make clean" in /certificates/
3. run "make" in /certificates/
4. run "make" in /build/
5. cp -r resources/server_files /build/src/
6. cp -r resources/proxy_files /build/src/
7. cp resources/object_list.txt /build/src/
8. In /build/src/ in three different terminal windows run:
	1. ./server -port 9999
	2. ./proxy -port 9990 -localhost:9999
	3. ./client -port 9990 object_list.txt

## Project details:
* Murmur3 was used as the hash function for rendezvous hashing and for the bloom filters
* Six proxy servers are simulated in the executable "proxy"
* Each bloom filter is an array of 303658 bits with five hash functions
	* This configuration results in a 0.9% chance of false positives with 30000 items in the bloom filter
	* Each bloom filter is an array of ints. Each bit in each int is one slot in the bloom filter

## Project contributions:
* Albert Dang
	* Created bloom filters
	* Inserting/searching bloom filters
	* Reading/writing to server and proxy server caches
	* Error checking
* Jason Chan
	* Set up TLS connections between client and proxy server, proxy server and server
	* TLS certificates
	* Rendezvous hashing
	* Comments
	* Logging
