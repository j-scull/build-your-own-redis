#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const size_t k_max_msg = 4096;

int die(char *message);

int msg(char *message);

void do_something(int connfd);

static int32_t read_full(int fd, char *buf, size_t n);

static int32_t write_all(int fd, const char *buf, size_t n);

static int32_t one_request(int connfd);

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

		// serve request from the client on the same connection
		while (1) {
			int32_t err = one_request(connfd);
			if (err) {
				break;
			}
		}
		close(connfd);
	}

	return 0;
}

/** Exit the program */
int die(char *message) {
	printf("%s failed: %s\n", message, strerror(errno));
	return 1;
}

/** Print a message to stdout */
int msg(char *message) {
	printf("%s\n", message);
}

/** simple request processing */
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

/** Read n bytes from file descriptor into a buffer */
static int32_t read_full(int fd, char *buf, size_t n) {
	while (n > 0) {
		ssize_t rv = read(fd, buf, n);
		if (rv <= 0) {
			return -1;
		}
		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}
	return 0;
}

/** Write n bytes from the buffer to the file descriptor */
static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1; 
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

/** Handle a single request and send a response */
static int32_t one_request(int connfd) {
    
	// read the length in the 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }


    // read the request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // process 
    printf("client says: %.*s\n", len, &rbuf[4]);
    
	// send response
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}