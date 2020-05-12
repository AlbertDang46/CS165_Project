#include <tls.h>
#include <iostream>

int main() {
	tls_init();
	std::cout << "Hello world!" << std::endl;
	return 0;
}
