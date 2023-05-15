#include "TCPRequestChannel.h"

using namespace std;

// Constructor for the TCPRequestChannel class
TCPRequestChannel::TCPRequestChannel(const std::string _ip_address, const std::string _port_no) {

    // Server address structure
    struct sockaddr_in srv_addr;

    // Create the TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Initialize server address structure using memset
    memset(&srv_addr, 0, sizeof(srv_addr));

    // Set address family and port number
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(stoi(_port_no));

    if (!_ip_address.empty()) {
        // Convert IP address and connect to the server
        inet_pton(AF_INET, _ip_address.c_str(), &(srv_addr.sin_addr));
        connect(sockfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    } else {
        // Set address and bind the socket
        srv_addr.sin_addr.s_addr = INADDR_ANY;
        bind(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

        // Listen for incoming connections
        listen(sockfd, SOMAXCONN);
    }
}

// Constructor for accepting connections
TCPRequestChannel::TCPRequestChannel(int _sockfd) {
    sockfd = _sockfd;
}

// Destructor for the TCPRequestChannel class
TCPRequestChannel::~TCPRequestChannel() {
    close(sockfd);
}

// Accept a connection and return the socket file descriptor
int TCPRequestChannel::accept_conn() {
    struct sockaddr_storage clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    return accept(sockfd, (struct sockaddr*)&clientAddr, &addrLen);
}

// Read data from the socket
int TCPRequestChannel::cread(void* buffer, int bufSize) {
    return recv(sockfd, buffer, bufSize, 0);
}

// Write data to the socket
int TCPRequestChannel::cwrite(void* buffer, int bufSize) {
    return send(sockfd, buffer, bufSize, 0);
}
