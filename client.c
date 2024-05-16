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

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	int getaddrinfo_return = getaddrinfo( "::1", "22", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		//Step 1.2
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			//Step 1.3
			int connect_return = connect( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( connect_return == -1 )
			{
				perror( "connect" );
				close( internet_socket );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

void execution(int internet_socket) {
    const char *request = "GET /ipgeo?apiKey=7d62ca6272534ea88de59891b49a1280 HTTP/1.1\r\n"
                          "Host: api.ipgeolocation.io\r\n"
                          "Connection: close\r\n\r\n";
    
    int bytes_sent = send(internet_socket, request, strlen(request), 0);
    if (bytes_sent == -1) {
        perror("send eror");
        return;
    }

    char buffer[1000];
    int bytes_received = 0;

    while ((bytes_received = recv(internet_socket, buffer, sizeof buffer - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received : %s\n", buffer);
    }

    if (bytes_received == -1) {
        perror("recveive error");
    }
}

void cleanup(int internet_socket) {
    int shutdown_return = shutdown(internet_socket, SD_SEND);
    if (shutdown_return == -1) {
        perror("cleanup error");
    }

    close(internet_socket);
}