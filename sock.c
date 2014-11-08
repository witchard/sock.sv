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
    char buffer[BUFFER_SIZE];
};

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
    struct handle* h = malloc(sizeof(struct handle));
    if(h)
        h->sock = sock;
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

void* sock_open(const char* uri) {
    size_t len = strlen(uri);
    if( len > 6 && strncmp("tcp://", uri, 6) == 0 ) {
        return tcp_sock_open(uri+6);
    }

    return NULL;
}

void sock_close(void* handle) {
    if(!handle)
        return;
    
    struct handle* h = handle;
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
    struct handle* h = handle;

    // Create output string (replace null termination with newline)
    memcpy(h->buffer, data, len);
    h->buffer[len] = '\n';
    len++;

    // Write
    int ret = 0;
    int done = 0;
    while(ret != -1 && done != len) {
        ret = send(h->sock, h->buffer+done, len-done, NULL);
        done += ret;
    }
    return ret == -1 ? 0 : 1; // Success if ret != -1
}

const char* sock_readln(void* handle) {
    if(!handle) 
        return 0;
    
    struct handle* h = handle;
    //if(!fgets(h->buffer, BUFFER_SIZE, h->filep))
    //    h->buffer[0] = '\0'; // Empty string on error
    return h->buffer;
}

