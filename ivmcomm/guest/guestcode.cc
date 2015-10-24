
#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <unistd.h>
using namespace std;
struct request {
  int mode;
  int id;
  int uid;
  int rw_perm;
};

struct response {
  int err;
  int offset;
  int size;
};

int main()
{

  int status;
  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
  // is created in C++, it will be given a block of memory. This memory is not necessary
  // empty. Therefor we use the memset function to make sure all fields are NULL.
  memset(&host_info, 0, sizeof host_info);

  std::cout << "Setting up the structs..."  << std::endl;

  host_info.ai_family = AF_INET;     // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

  // Now fill up the linked list of host_info structs with google's address information.
  status = getaddrinfo("localhost", "8888", &host_info, &host_info_list);
  // getaddrinfo returns 0 on succes, or some other value when an error occured.
  // (translated into human readable text by the gai_gai_strerror function).
  if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;


  std::cout << "Creating a socket..."  << std::endl;
  int socketfd ; // The socket descripter
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
  host_info_list->ai_protocol);
  if (socketfd == -1)  std::cout << "socket error " ;
  status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1)  std::cout << "connect error" ;



  request r;
  r.mode = 1;
  r.id = 1;
  r.uid = 1;
  r.rw_perm = 1;

  response rs;
  std::cout << "send()ing message..."  << std::endl;
  int len;
  ssize_t bytes_sent;
  struct nts {
    int x;
  };
  nts nt;
  len = sizeof(request);
  for (int i = 0; i < 100000; ++i) {
    //socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
    //  host_info_list->ai_protocol);
    //status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    //if (status == -1)  std::cout << "connect error" ;
    r.mode = 1;
    bytes_sent = send(socketfd, (&r), len, 0);
    recv(socketfd, (&rs), len, 0);
    r.mode = 2;
    bytes_sent = send(socketfd, (&r), len, 0);
    recv(socketfd, (&nt), sizeof(nts), 0);
    //close(socketfd);
  }
}
