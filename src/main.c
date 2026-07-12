#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <genericVector.h>
#include <util.h>


struct Conn {
    int fd;
    int want_read;
    int want_write;
    int want_close;
	Vector *incoming; // uint8_t
	Vector *outgoing; // uint8_t
} typedef Conn;

typedef struct pollfd Pollfd;

const size_t k_max_msg = 4096;


static int die(char *message);

static int msg(const char *message);

static void msg_errno(const char *msg);

static int32_t read_full(int fd, char *buf, size_t n);

static int32_t write_all(int fd, const char *buf, size_t n);

static void buf_append(Vector *buf, uint8_t *data, size_t len);

static void buf_consume(Vector *buf, size_t n);

static void fd_set_nb(int fd);

static Conn *handle_accept(int fd);

/** application callback when the socket is writable */
static void handle_read(Conn *conn); 

/** application callback when the socket is writable */
static void handle_write(Conn *conn);

/** Process one request, if there is enough data */
static int try_one_request(Conn *conn); 

/** Checks whether a Conn has been initialised */
static int is_valid(Conn *conn);

/** Close connection */
static void close_connection(Conn *conn);

int main() {

	printf("Starting server...\n");

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

	// A map of file descriptors to client connections
	Vector *fd2conn = make_vector(sizeof(Conn));

	Vector *poll_args = make_vector(sizeof(Pollfd));
	
	// the event loop
	while (1) {

		clear(poll_args);

		Pollfd pfd = {fd, POLLIN, 0};
		push_back(poll_args, (void *)&pfd);

		for (int i = 0; i < size(fd2conn); i++) {
			
			Conn *conn = get(fd2conn, i);
			if (is_valid(conn) == 0){
				continue;
			}

			Pollfd pfd = {conn->fd, POLLERR, 0};
			if (conn->want_read) {
				pfd.events |= POLLIN;
			}
			if (conn->want_write) {
				pfd.events |= POLLOUT;
			}
			push_back(poll_args, (void *)&pfd);
		}

		// wait for readiness
		int rv = poll(data(poll_args), (nfds_t)size(poll_args), -1);
		if (rv < 0 && errno == EINTR) {
			continue; 
		}
		if (rv < 0) {
			die("poll");
		}

		// handle the listening socket
		if (((Pollfd *)get(poll_args, 0))->revents) {
			Conn *conn = handle_accept(fd);
			if (conn) {

				// put the connection in the fd to connection map
				if (size(fd2conn) <= (size_t)conn->fd) {
					resize(fd2conn, conn->fd+1);
				}
				set(fd2conn, conn, conn->fd);
			}
		}

		// handle conection sockets
		// skiping the first
		for (size_t i = 1; i < size(poll_args); ++i) {
			uint32_t ready = ((Pollfd *)get(poll_args, i))->revents;
			if (ready == 0) {
				continue;
			}

			Pollfd *p = (Pollfd *)get(poll_args, i);
			int fd = p->fd;
			Conn *conn = get(fd2conn, fd);

			if (ready & POLLIN) {
				assert(conn->want_read);
				handle_read(conn);
			}
			if (ready & POLLOUT) {
				assert(conn->want_write);
				handle_write(conn);
			}

			// close the socket
			if ((ready & POLLERR) || conn->want_close) {
				close_connection(conn);
			}
		}
	}
	return 0;
}

/** Exit the program */
static int die(char *message) {
	printf("%s failed: %s\n", message, strerror(errno));
	return 1;
}

/** Print a message to stdout */
static int msg(const char *message) {
	printf("%s\n", message);
}

/** Print an error message to stdout */
static void msg_errno(const char *msg) {
    fprintf(stderr, "[errno:%d] %s\n", errno, msg);
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

static void buf_append(Vector *buf, uint8_t *data, size_t len) {
	insert(buf, end(buf), data, len); 
}

static void buf_consume(Vector *buf, size_t n) {
	erase(buf, begin(buf), begin(buf) + n);
}

/** Make the listening socket non-blocking */
static void fd_set_nb(int fd) {
	errno = 0;
	int flags = fcntl(fd, F_GETFL, 0);
	if (errno) {
		die("fcntl error");
		return;
	}

	flags |= O_NONBLOCK;

	errno = 0;
	(void)fcntl(fd, F_SETFL, flags);
	if (errno) {
		die("fnctl error");
	}
}

/** Accept a connection.
 *  Application callback when the listening socket is ready
 */
static Conn *handle_accept(int fd) {
	struct sockaddr_in client_addr = {};
	socklen_t addrlen = sizeof(client_addr);
	int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);		
	if (connfd < 0) {
		msg_errno("accept() error");
		return NULL;
	}

	uint32_t ip = client_addr.sin_addr.s_addr;
	fprintf(stdout, "new client from %u.%u.%u.%u:%u\n",
		ip & 255, (ip >> 8) & 255, (ip >>16) & 255, ip >> 24,
		ntohs(client_addr.sin_port));

	fd_set_nb(connfd);

	Conn *conn = (Conn *)malloc(sizeof(Conn));
	conn->fd = connfd;
	conn->want_read = 1;
	conn->incoming = make_vector(sizeof(uint8_t));
	conn->outgoing = make_vector(sizeof(uint8_t));
	return conn;
}

static void handle_read(Conn *conn) {
	uint8_t buf[64 * 1024];
	ssize_t rv = read(conn->fd, buf, sizeof(buf));

	uint32_t len = 0;
	memcpy(&len, buf, 4);

	printf("%02x %02x %02x %02x\n",
       (unsigned char)buf[0],
       (unsigned char)buf[1],
       (unsigned char)buf[2],
       (unsigned char)buf[3]);

	if (rv < 0) {
		msg_errno("read() error");
		conn->want_close = 1;
		return;
	}
	if (rv == 0) {
		if (size(conn->incoming) == 0) {
			msg("client closed");
		}
		else {
			msg("unexpected EOF");                                  
		}
		conn->want_close = 1;
		return;
	}

	buf_append(conn->incoming, buf, (size_t)rv);

	// parse request, generate respone
	// pipelining
	while (try_one_request(conn) == 1) {}

	if (size(conn->outgoing) > 0) {
		conn->want_read = 0;
		conn->want_write = 1;
		return handle_write(conn);
	}

}

static int try_one_request(Conn *conn) {

	// try to parse
	// Protocol: message header
	if (size(conn->incoming) < 4) {
		return 0;  // want read
	}

	uint32_t len = 0;
	memcpy(&len, data(conn->incoming), 4);
	if (len > k_max_msg) {
		// protocol error
		msg("too long");
		conn->want_close = 1;
		return 0;
	}

	// Protocol: message body
	if (4 + len > size(conn->incoming)) {
		return 0; // want read
	}
	uint8_t *request = get(conn->incoming, 4);

	printf("client says: len:%d data:%.*s\n",
        len, len < 100 ? len : 100, request);

	// echo response
	buf_append(conn->outgoing, (uint8_t *)&len, 4);
	buf_append(conn->outgoing, request, len);

	// remove message from incoming
	buf_consume(conn->incoming, 4+len);
	return 1;
}


static void handle_write(Conn *conn) {
	assert(size(conn->outgoing) > 0);
	ssize_t rv = write(conn->fd, data(conn->outgoing), size(conn->outgoing));
	if (rv < 0 && errno == EAGAIN) {
		return;
	}
	if (rv < 0) {
		msg_errno("write() error");
		conn->want_close = 1;
		return;
	}

	// remove written data from outgoing
	buf_consume(conn->outgoing, (size_t)rv);

	if (size(conn->outgoing) == 0) {
		conn->want_read = 1;
		conn->want_write = 0;
	}
}


static int is_valid(Conn *conn) {

	if (conn == NULL || !conn) {
		return 0;
	}
	if (conn->fd != 0) {
		return 1;
	}
	if (conn->incoming != NULL) {
		return 1;
	} 
	if (conn->outgoing != NULL) {
		return 1;
	} 
	if (conn->want_close != 0) {
		return 1;
	} 
	if (conn->want_read != 0) {
		return 1;
	} 
	if (conn->want_write != 0) {
		return 1;
	}
	return 0;
}

static void close_connection(Conn *conn) {
	(void)close(conn->fd);
	free(conn->incoming);
	free(conn->outgoing);
	conn->fd = 0;
	conn->want_read = 0;
	conn->want_write = 0;
	conn->want_close = 0;
	conn->incoming = NULL;
	conn->incoming = NULL;
}
