#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <linux/netfilter_ipv4.h>

#define MAX 8192
#define SA struct sockaddr 

char * proxy_ip_address;
int proxy_port;

void mperror(const char* msg) {
    perror(msg);
    exit(-1);
}


int setup_proxy_socket() {
    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        mperror("proxy socket"); 
    } 

    printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(proxy_ip_address); 
    servaddr.sin_port = htons(proxy_port); 
  
    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        mperror("proxy connect"); 
    } 

    printf("connected to the server..\n"); 
    return sockfd;
}

// Function designed for chat between client and server. 
void client_logic(int client_socket, char * destination_ip_address, int port) 
{ 
    fd_set active_fd_set, read_fd_set;
    int i;
    char buff[MAX]; 
    int proxy_socket = setup_proxy_socket();
    int len = snprintf(buff, MAX, "CONNECT %s:%d HTTP/1.0\r\n\r\n", destination_ip_address, port);
    write(proxy_socket, buff, len);
    bzero(buff, sizeof(buff));
    read(proxy_socket, buff, MAX);
    printf("Proxy established the connection to %s:%d\n", destination_ip_address, port);

    FD_ZERO (&active_fd_set);
    FD_SET (proxy_socket, &active_fd_set);
    FD_SET (client_socket, &active_fd_set);

    while(1) {
        read_fd_set = active_fd_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
          mperror ("select");
        }

        // printf("Event!\n");

        for (i = 0; i < FD_SETSIZE; ++i) {
           if (FD_ISSET (i, &read_fd_set)) {
              if (i == client_socket) {
                  len = read(client_socket, buff, sizeof(buff)); 
                  // printf("Client sent a message, %d\n", len);
                  if(len <= 0) goto out;
                  write(proxy_socket, buff, len);
              }
              else
              if (i == proxy_socket) {
                  len = read(proxy_socket, buff, sizeof(buff)); 
                  // printf("Proxy sent a message, %d\n", len);
                  if(len <= 0) goto out;
                  write(client_socket, buff, len);
              }
              else {
                 mperror("wtf");
              }
           }
        }
    }
out:
    close(proxy_socket);
    printf("Connection closed\n");
} 


int setup_server_socket(int listen_port) {
    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        mperror("socket"); 
    } 

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
       mperror("setsockopt(SO_REUSEADDR) failed");
    }

    printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(listen_port); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        mperror("bind"); 
    } 

    printf("Socket successfully bound to port %d..\n", listen_port); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        mperror("listen"); 
    } 
    printf("Server listening..\n"); 
    
    return sockfd;
}

void receive_connections(int server_socket) {
    struct sockaddr_in cli; 
    int len, client_socket, childpid;
    int tcp_header_port;
    char *ip_header_address; /* dotted decimal host addr string */

    struct sockaddr_in orig_addr;
    char *orig_ip_address = "79.172.213.243"; /* dotted decimal host addr string, connecting to index.hu in test mode where SO_ORIGINAL_DST is not available */
    int orig_port = 80;
    int error;

    socklen_t socklen = sizeof(orig_addr);

    // Accept the data packet from client and verification 
    len = sizeof(cli); 
    while(1) {
      client_socket = accept(server_socket, (SA*)&cli, &len); 
      if (client_socket < 0) { 
        mperror("accept"); 
      }

      // reaping potential zombie children to free up resources
      waitpid(-1, NULL, WNOHANG);

      ip_header_address = inet_ntoa(cli.sin_addr);
      tcp_header_port = cli.sin_port;
      printf("Acccepted a client connection from (%s:%d)\n", ip_header_address, tcp_header_port); 

      error = getsockopt(client_socket, SOL_IP, SO_ORIGINAL_DST, &orig_addr, &socklen);
      if (error == 0) {
        orig_ip_address = inet_ntoa(orig_addr.sin_addr);
        orig_port = orig_addr.sin_port;
#ifdef NETWORK_BYTE_ORDER
        orig_port = htons(orig_port);
#endif
        printf("Original address before NAT (%s:%d)\n", orig_ip_address, orig_port); 
      } else {
        perror("original_dst"); // we dont exit here
      }

      if ((childpid = fork()) < 0) {
         mperror("fork");
      }
      else if (childpid == 0) {   /* child  */
        close(server_socket);
        client_logic(client_socket, orig_ip_address, orig_port);
        close(client_socket);
        exit(0);
      }

      close(client_socket);  /* parent */ 
    }
}
  
// Driver function 
int main(int argc, char* argv[]) 
{ 
    int listen_port;

    if (argc != 4) {
       fprintf(stderr, "Usage: %s listen_port http-proxy-ip-address http-proxy-port\n", argv[0]);
       return -1;
    }
    listen_port = atoi(argv[1]);
    proxy_ip_address = argv[2];
    proxy_port = atoi(argv[3]);

    int server_socket = setup_server_socket(listen_port);
    receive_connections(server_socket);
    close(server_socket);

} 
