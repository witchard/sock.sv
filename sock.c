#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef _WIN32
// Need to say we are on recent enough windows to get getaddrinfo...
// http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <ws2tcpip.h>
#endif /* _WIN32 */

#if defined __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define closesocket(s) close(s);
#endif /* __linux__ */

// Thanks: http://beej.us/guide/bgnet/
//         https://github.com/jimloco/Csocket

#define BUFFER_SIZE 1024*1024
struct handle {
    SOCKET sock;
    char wbuf[BUFFER_SIZE];
    char rbuf[BUFFER_SIZE+1];
    size_t roff; // Read pointer
    size_t eoff; // Read end
};

#ifdef __cplusplus
extern "C" {
#endif

int sock_init() {
#ifdef _WIN32
    // Init the windows sockets
    WSADATA wsaData;
    if( WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR) {
        return -1;
    }
#endif /* _WIN32 */
    return 0;
}

void sock_shutdown() {
#ifdef _WIN32
    WSACleanup();    
#endif /* _WIN32 */
}

struct handle* init_struct(SOCKET sock) {
    struct handle* h = (struct handle*) malloc(sizeof(struct handle));
    if(h) {
        h->sock = sock;
        h->roff = 0;
        h->eoff = 0;
        h->rbuf[BUFFER_SIZE] = '\0'; // Overflow protection for long strings
    }
    return h;
}

void* tcp_sock_open(const char* name) {
    // Extract hostname / port
    char* string = strdup(name);
    if(!string)
        return NULL;
    char* colon = strchr(string, ':');
    if(!colon) {
        free(string);
        return NULL;
    }
    *colon = '\0'; // Split string into hostname and port
    const char* hostname = string;
    const char* port = colon + 1;

    int status;
    struct addrinfo hints, *res, *p;

    // Setup hints - we want TCP and dont care if its IPv6 or IPv4
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    // Look up the host
    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        free(string);
        return NULL;
    }
    free(string);

    // Try and connect
    SOCKET sock = INVALID_SOCKET;
    for(p = res; sock == INVALID_SOCKET && p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sock != INVALID_SOCKET) {
            status = connect(sock, p->ai_addr, p->ai_addrlen);
            if(status == SOCKET_ERROR) {
                closesocket(sock);
                sock = INVALID_SOCKET;
            }
        }
    }
    freeaddrinfo(res); // free the linked list
    if( sock == INVALID_SOCKET ) {
        return NULL;
    }

    // Create handle
    return init_struct(sock);
}

#ifdef __linux__
void* unix_sock_open(const char* name) {
    // Build endpoint details
    struct sockaddr_un endpoint;
    if(strlen(name) == 0 || strlen(name) > sizeof(endpoint.sun_path))
        return NULL; // Invalid socket name
    endpoint.sun_family = AF_UNIX;
    strcpy(endpoint.sun_path, name);
    size_t len = strlen(endpoint.sun_path) + sizeof(endpoint.sun_family);
    if(endpoint.sun_path[0] == '@')
	endpoint.sun_path[0] = '\0'; // Use @ for abstract namespace

    // Create socket
    SOCKET sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock == INVALID_SOCKET) {
	return NULL;
    }

    // Connect
    if( connect(sock, (struct sockaddr *)&endpoint, len) == SOCKET_ERROR) {
	closesocket(sock);
	return NULL;
    }

    // Create handle
    return init_struct(sock);
}
#endif

void* sock_open(const char* uri) {
    size_t len = strlen(uri);
    if( len > 6 && strncmp("tcp://", uri, 6) == 0 ) {
        return tcp_sock_open(uri+6);
    }
#ifdef __linux__
    else if( len > 7 && strncmp("unix://", uri, 7) == 0 ) {
        return unix_sock_open(uri+7);
    }
#endif

    return NULL;
}

void sock_close(void* handle) {
    if(!handle)
        return;
    
    struct handle* h = (struct handle*) handle;
    closesocket(h->sock);
    free(h);
}

int sock_writeln(void* handle, const char* data) {
    // Validate input
    if(!handle) 
        return 0; // Invalid handle
    size_t len = strlen(data);
    if(len >= BUFFER_SIZE)
        return 0; // String too big
    struct handle* h = (struct handle*) handle;

    // Create output string (replace null termination with newline)
    memcpy(h->wbuf, data, len);
    h->wbuf[len] = '\n';
    len++;

    // Write
    int ret = 0;
    int done = 0;
    while(ret != -1 && done != len) {
        ret = send(h->sock, h->wbuf+done, len-done, 0);
        done += ret;
    }
    return ret == -1 ? 0 : 1; // Success if ret != -1
}

const char* sock_readln(void* handle) {
    // Validate input
    if(!handle) 
        return 0;    
    struct handle* h = (struct handle*) handle;

    // Prepare read - move down any spare data from last time
    if( h->roff > h->eoff ) {
        memmove(h->rbuf, h->rbuf + h->eoff, h->roff - h->eoff);
        h->roff -= h->eoff;
    } else {
        h->roff = 0;
    }

    // Read
    int ret = 0;
    char* end = (char*) memchr((char*)(h->rbuf), '\n', h->roff);
    while(ret != -1 && h->roff != BUFFER_SIZE && end == NULL) {
        ret = recv(h->sock, h->rbuf + h->roff, BUFFER_SIZE - h->roff, 0);
        if( ret != -1 )
            end = (char*) memchr((char*)(h->rbuf + h->roff), '\n', ret); // Search for \n
        h->roff += ret;
    }

    // Tidy up string
    if( ret == -1 ) {
        h->rbuf[0] = '\0'; // Empty string on error
    }
    if( end ) {
        *end = '\0'; // Replace newline
        h->eoff = end + 1 - h->rbuf; // Store where we got to
    } else {
		h->roff = 0;
	}

    return h->rbuf;
}

#ifdef __cplusplus
}
#endif

