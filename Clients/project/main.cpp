#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <thread>
#include <chrono>
#include <exception>
#include <string>
#include <unordered_map>
#include <tuple>

class MyException : public std::exception
{
public:
    explicit MyException( const std::string& msg ) : m_msg(msg) {}
    const char *what() const noexcept override { return m_msg.c_str(); }
private:
    std::string m_msg;
};

class ServerException : public MyException
{
public:
    ServerException( const std::string& m ) : MyException(m) {}
};

std::string IP = "127.0.0.1";

class Caller
{
private:

    std::string readFromSock( const int& sock )
    {
        std::string receivedData;
        char *buffer = new char[1024];
        while(true)
        {
            int bytesReceived = recv( sock, buffer, 1024, 0 );
            if( bytesReceived <= 0 )
            {
                delete []buffer;
                receivedData.clear();
                return "";
            }
            receivedData.append( buffer, bytesReceived );
            if( receivedData.back() == ';' )
                break;
        }
        delete []buffer;
        return receivedData;
    }

    void sendToSock( const int& sock, const std::string& msg )
    {
        const char* dataPtr = msg.c_str();
        size_t dataSize = msg.length();
        size_t totalSent = 0;
        while( totalSent < dataSize )
        {
            int bytesSent = send( sock, ( dataPtr + totalSent ), ( dataSize - totalSent ), 0 );
            if( bytesSent == -1 )
                break;
            totalSent += bytesSent;
        }
    }

    int openSocket( const std::string& IP, const int& PORT )
    {
        int sock = socket( AF_INET, SOCK_STREAM, 0 );
        if( sock < 0 )
            throw ServerException( ( "listener = " + std::to_string(sock) ) );

        struct sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = inet_addr(IP.c_str());

        if( connect( sock, ( struct sockaddr* )&addr, sizeof(addr) ) < 0 )
            throw ServerException( "connect" );
        return sock;
    }

public:
    Caller() {}

    ~Caller() {}

    struct Client
    {
        std::string _name;
        int _sock;
        int _port;
        int _t;

        Client( const std::string& name = "", const int& sock = 0, const int& port = 0, const int& t = 0 ) :
            _name(name), _sock(sock), _port(port), _t(t) {}
        Client( const Client& rhs )
        {
            this->_name = rhs._name;
            this->_sock = rhs._sock;
            this->_port = rhs._port;
            this->_t = rhs._t;
        }

        Client& operator=( const Client& rhs )
        {
            if( this->_sock != rhs._sock )
            {
                this->_name = rhs._name;
                this->_sock = rhs._sock;
                this->_port = rhs._port;
                this->_t = rhs._t;
            }
            return *this;
        }

        ~Client()
        {
            _name.clear();
        }

    };

    std::vector<Client> clients;

    int pushClient()
    {
        std::string string;
        int port = 8000;
        int time = 0;
        std::cin >> string >> time;

        // В идеале мы должны проводить проверки на корректные данные и т д
        
        Client client;
        client._name = string;
        client._port = port;
        client._sock = openSocket( IP, port );
        client._t = time;
        clients.push_back(client);  // По факту можно не сохранять. . .

        std::string message = string + " " + std::to_string(time);
        sendToSock( client._sock, ( message + "\n" ) );
        return client._sock;
    }
};

int main()
{
    Caller caller;
    while(true)
    {
        if( caller.pushClient() >= 15 )
            break;
    }
    while(true);  // Ожидание, если больше 15 пользователей, тк границы возможных пользователей не указаны,
                  // я ограничил 15-ю пользователями. 
    return 0;
}


