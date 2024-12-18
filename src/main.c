#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


int die(char *message);

int msg(char *message);

void do_something(int connfd);


int main() {

	msg("Starting server...");

    // Obtain socket handle
	// AF_INET is for IPv4 
	// SOCK_STREAM is for TCP
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	// Configure the socket
	int val = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	
	// bind to an address - wildcard address 0.0.0.0:1234
	// note, the port and address numbers are converted to big endian
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(1234);     
	addr.sin_addr.s_addr = ntohl(0);
	int addr_len = sizeof(addr);
	int rv = bind(fd, (const struct sockaddr *)&addr, addr_len);
	if (rv) {
		die("bind()");
	}

	// listen
	rv = listen(fd, SOMAXCONN);
	if (rv) {
		die("listen()");
	}

    getsockname(fd, (struct sockaddr *)&addr, &addr_len);
	printf("Listening on %s:%i\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	// accept connctions
	while (1) {
		struct sockaddr_in client_addr = {};
		socklen_t addrlen = sizeof(client_addr);
		int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);		
		if (connfd < 0) {
			// error
			continue;
		}
		getpeername(connfd, (struct sockaddr *)&client_addr, &addrlen);
		printf("Accepted connection from %s:%i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		do_something(connfd);
		close(connfd);
	}

	return 0;
}

int die(char *message) {
	printf("%s failed: %s\n", message, strerror(errno));
	return 1;
}

int msg(char *message) {
	printf("%s\n", message);
}

void do_something(int connfd) {
	msg("doSomething()");
	char rbuf[64] = {};
	ssize_t n = read(connfd, rbuf, sizeof(rbuf) -1);
	if (n < 0) {
		msg("read() error");
		return;
	}
	printf("client says: %s\n", rbuf);

	char wbuf[] = "hello";
	write(connfd, wbuf, strlen(wbuf));
}