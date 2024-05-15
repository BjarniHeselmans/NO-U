#ifdef _WIN32
    #define _WIN32_WINNT _WIN32_WINNT_WIN7
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <string.h>
    void OSInit(void) {
        WSADATA wsaData;
        int WSAError = WSAStartup(MAKEWORD(2, 0), &wsaData);
        if (WSAError != 0) {
            fprintf(stderr, "WSAStartup errno = %d\n", WSAError);
            exit(-1);
        }
    }
    void OSCleanup(void) {
        WSACleanup();
    }
    #define perror(string) fprintf(stderr, string ": WSA errno = %d\n", WSAGetLastError())
    #define close closesocket
    #define SD_SEND SD_BOTH
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <errno.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <string.h>
    void OSInit(void) {}
    void OSCleanup(void) {}
#endif

int initialization();
void execution(int internet_socket);
void cleanup(int internet_socket);

int main(int argc, char *argv[]) {
    OSInit();

    int internet_socket = initialization();

    execution(internet_socket);

    cleanup(internet_socket);

    OSCleanup();

    return 0;
}

int initialization() {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo("api.ipgeolocation.io", "20", &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd = -1;
    struct addrinfo *p;
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (sockfd == -1) {
        fprintf(stderr, "socket: no valid socket address found\n");
        exit(2);
    }

    return sockfd;
}

void execution(int internet_socket) {
    const char *request = "GET /ipgeo?apiKey=
7d62ca6272534ea88de59891b49a1280
 HTTP/1.1\r\n"
                          "Host: api.ipgeolocation.io\r\n"
                          "Connection: close\r\n\r\n";
    
    int bytes_sent = send(internet_socket, request, strlen(request), 0);
    if (bytes_sent == -1) {
        perror("send");
        return;
    }

    char buffer[1000];
    int bytes_received = 0;
    FILE *log_file = fopen("IPLOG.txt", "w");
    if (!log_file) {
        perror("fopen");
        return;
    }

    while ((bytes_received = recv(internet_socket, buffer, sizeof buffer - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        fputs(buffer, log_file);
    }

    if (bytes_received == -1) {
        perror("recv");
    }

    fclose(log_file);
}

void cleanup(int internet_socket) {
    int shutdown_return = shutdown(internet_socket, SD_SEND);
    if (shutdown_return == -1) {
        perror("shutdown");
    }

    close(internet_socket);
}