#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
#include <algorithm>
#include <fstream>
#include <sstream>

std::vector<int> clients;
std::mutex mtx;

std::string htmlToString(std::string filename)
{
    std::ifstream file(filename.c_str(), std::ios::in);
    std::stringstream buffer;
    if (file)
    {
        buffer << file.rdbuf(); // Read file contents into a stringstream
    }
    else
    {
        std::cerr << "Error opening file: " << filename << std::endl;
    }
    return buffer.str();
}

// void handleClient(int clientSocket)
// {
//     std::string response = htmlToString("index.html");

//     // Calculate correct Content-Length
//     std::string httpResponse =
//         "HTTP/1.1 200 OK\r\n"
//         "Content-Type: text/html\r\n"
//         "Content-Length: " +
//         std::to_string(response.size()) + "\r\n"
//                                           "\r\n" +
//         response;

//     // Send the complete HTTP response (headers + body)
//     send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0);
//     close(clientSocket);
// }

void handleClient(int clientSocket)
{
    char buffer[1024] = {0};
    read(clientSocket, buffer, 1024);

    // Parse the HTTP request line
    std::string request(buffer);
    std::string path;

    if (request.find("GET / ") != std::string::npos)
    {
        // Client requested the root path, send a redirect to /index.html
        std::string redirectResponse =
            "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: /index.html\r\n"
            "\r\n";

        send(clientSocket, redirectResponse.c_str(), redirectResponse.size(), 0);
    }
    else if (request.find("GET /index.html") != std::string::npos)
    {
        // Client requested /index.html, serve the HTML content
        std::string response = htmlToString("index.html");

        std::string httpResponse =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " +
            std::to_string(response.size()) + "\r\n"
                                              "\r\n" +
            response;

        send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0);
    }
    else
    {
        // Handle other cases (e.g., 404 Not Found)
        std::string notFoundResponse =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 45\r\n"
            "\r\n"
            "<html><body><h1>404 Not Found</h1></body></html>";

        send(clientSocket, notFoundResponse.c_str(), notFoundResponse.size(), 0);
    }

    close(clientSocket);
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    int port = 8080;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Listening on localhost port: " << port << std::endl;

    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        std::lock_guard<std::mutex> lock(mtx);
        clients.push_back(new_socket);
        std::cout << "New client: ID: " << new_socket << " \n";
        std::thread(handleClient, new_socket).detach();
    }

    close(server_fd);
    return 0;
}