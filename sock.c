#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef _WIN32
// Need to say we are on recent enough windows to get getaddrinfo...
// http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32
#define _WIN32_WINNT 0x0501
#define popen _popen 
// ^^ TODO Is this necessary
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif /* _WIN32 */

// Thanks: http://beej.us/guide/bgnet/
//         https://github.com/jimloco/Csocket

#define BUFFER_SIZE 1024*1024
struct handle {
    FILE* filep;
    char buffer[BUFFER_SIZE];
    char is_proc;
};

int sock_init() {
#ifdef _WIN32
    // Init the windows sockets
    WSADATA wsaData;
    if( WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR) {
        printf("WSAStartup failed.\n");
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
        printf("Error getting host info: %s\n", gai_strerror(status));
        return NULL;
    }
    free(string);

    // Try and connect
    int sock = -1;
    for(p = res; sock == -1 && p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sock != -1) {
            status = connect(sock, p->ai_addr, p->ai_addrlen);
            if(status < 0) {
                close(sock);
                sock = -1;
            }
        }
    }
    freeaddrinfo(res); // free the linked list
    if( sock < 0 ) {
        printf("No valid host found\n");
        return NULL;
    }
    
    // Create file handles (this makes our life easer for read / write)
    FILE* filep  = fdopen(sock, "w+");
    if( filep == NULL ) {
        close(sock);
        return NULL;
    }

    struct handle* h = malloc(sizeof(struct handle));
    if(!h) {
        fclose(filep);
        return NULL;
    }
    h->filep = filep;
    h->is_proc = 0;
    return h;
}

void* proc_sock_open(const char* cmd) {
    FILE* filep = popen(cmd, "rw");

    struct handle* h = malloc(sizeof(struct handle));
    if(!h) {
        pclose(filep);
        return NULL;
    }
    h->filep = filep;
    h->is_proc = 1;
    return h;
}

void* sock_open(const char* uri) {
    size_t len = strlen(uri);
    if( len > 6 && strncmp("tcp://", uri, 6) == 0 ) {
        return tcp_sock_open(uri+6);
    } else if( len > 7 && strncmp("proc://", uri, 7) == 0 ) {
        return proc_sock_open(uri+7);
    }

    printf("Invalid URI\n");
    return NULL;
}

void sock_close(void* handle) {
    if(!handle)
        return;
    
    struct handle* h = handle;
    if(h->is_proc)
        pclose(h->filep);
    else
        fclose(h->filep);
    free(h);
}

int sock_writeln(void* handle, const char* data) {
    if(!handle) 
        return 0; // Invalid handle
    
    struct handle* h = handle;
    int ret = 0;
    if(fprintf(h->filep, "%s\n", data) < 0)
        return 0; // Failed to write
    
    fflush(h->filep); // Flush
    return 1;
}

const char* sock_readln(void* handle) {
    if(!handle) 
        return 0;
    
    struct handle* h = handle;
    if(!fgets(h->buffer, BUFFER_SIZE, h->filep))
        h->buffer[0] = '\0'; // Empty string on error
    return h->buffer;
}

