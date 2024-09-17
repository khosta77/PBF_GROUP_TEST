#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <exception>
#include <string>
#include <ctime>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

std::atomic<size_t> i = 0;

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

struct DateTime {
    std::chrono::system_clock::time_point time_point;

    DateTime() : time_point(std::chrono::system_clock::now()) {}

    std::string to_string() const {
        auto time_t = std::chrono::system_clock::to_time_t(time_point);
        auto local_tm = *std::localtime(&time_t);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count() % 1000;

        std::ostringstream oss;
        oss << std::put_time(&local_tm, "[%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3) << milliseconds << "]";
        return oss.str();
    }

    int get_seconds() const {
        return std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count() % 60;
    }

    int get_minutes() const {
        return std::chrono::duration_cast<std::chrono::minutes>(time_point.time_since_epoch()).count() % 60;
    }

    int get_hours() const {
        return std::chrono::duration_cast<std::chrono::hours>(time_point.time_since_epoch()).count() % 24;
    }

    int get_year() const {
        auto time_t = std::chrono::system_clock::to_time_t(time_point);
        auto local_tm = *std::localtime(&time_t);
        return local_tm.tm_year + 1900;
    }

    int get_month() const {
        auto time_t = std::chrono::system_clock::to_time_t(time_point);
        auto local_tm = *std::localtime(&time_t);
        return local_tm.tm_mon + 1;
    }

    int get_day() const {
        auto time_t = std::chrono::system_clock::to_time_t(time_point);
        auto local_tm = *std::localtime(&time_t);
        return local_tm.tm_mday;
    }
};

std::vector<std::string> split( const std::string& content, const char separator )
{
    std::vector<std::string> arrayString;
    std::string buffer = "";
    for( size_t i = 0, I = content.size(); i < I; ++i )
    {
        if( content[i] != separator )
            buffer += content[i];
        else
        {
            arrayString.push_back(buffer);
            buffer = "";
        }
    }
    arrayString.push_back(buffer);
    buffer.clear();
    return arrayString;
}

class Server final
{
private:
    int _listener;
    struct sockaddr_in _addr;

    std::vector<int> _socketsToClear;

    std::set<int> _clients;
    fd_set _readset;
    timeval _timeout;

    std::string readFromRecv( const int& s )
    {
        std::string receivedData;
        char *buffer = new char[1024];
        while(true)
        {
            int bytesReceived = recv( s, buffer, 1024, 0 );
            if( bytesReceived <= 0 )
            {
                delete []buffer;
                receivedData.clear();
                return "";
            }
            receivedData.append( buffer, bytesReceived );
            if( receivedData.back() == '\n' )
                break;
        }
        delete []buffer;
        return receivedData;
    }

    void sendToSock( const int& s, const std::string& msg )
    {
        const char* dataPtr = msg.c_str();
        size_t dataSize = msg.length();
        size_t totalSent = 0;
        while( totalSent < dataSize )
        {
            int bytesSent = send( s, ( dataPtr + totalSent ), ( dataSize - totalSent ), 0 );
            if( bytesSent == -1 )
                break;
            totalSent += bytesSent;
        }
    }

    void clearSocket( const int& sock )
    {
        std::cout << "socket " << sock << " free" << std::endl;
        close(sock);
        _socketsToClear.push_back(sock);
    }

public:
    Server( const std::string& IP, const int& PORT )
    {
        _listener = socket( AF_INET, SOCK_STREAM, 0 );
        if( _listener < 0 )
            throw ServerException( ( "listener = " + std::to_string(_listener) ) );
        fcntl( _listener, F_SETFL, O_NONBLOCK );

        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(PORT);
        _addr.sin_addr.s_addr = inet_addr(IP.c_str());

        if( int _bind = bind( _listener, ( struct sockaddr* )&_addr, sizeof(_addr) ); _bind < 0 )
            throw ServerException( ( "bind = " + std::to_string(_bind) ) );
        listen( _listener, 1 );

        std::cout << "server in system address: " << IP << ":" << PORT << std::endl;
        
        // Задаём таймаут
        _timeout.tv_sec = 1;
        _timeout.tv_usec = 0;
    }

    ~Server()
    {
        _clients.clear();
        _socketsToClear.clear();
    }
     
    struct ClientData
    {
        std::string _name;
        int _t;

        ClientData( const std::string& name = "", const int& T = -1 ) : _name(name), _t(T) {}
        ClientData& operator=( const ClientData& rhs )
        {
            if( this->_name != rhs._name )
            {
                this->_name = rhs._name;
                this->_t = rhs._t;
            }
            return *this;
        }
    };

    std::map<int, ClientData> df;
    
    int run()
    {
        while( true )
        {
            // Заполняем множество сокетов
            FD_ZERO( &_readset );
            FD_SET( _listener, &_readset );
            for( auto it = _clients.begin(); it != _clients.end(); it++ )
                FD_SET( *it, &_readset );
            
            int mx = std::max( _listener, *max_element( _clients.begin(), _clients.end() ) );
            int sel = select( ( mx + 1 ), &_readset, NULL, NULL, &_timeout );

            if( sel < 0 )
                throw ServerException( ( "select " + std::to_string(sel) ) );
            else if( sel == 0 ) {}
            
            if( FD_ISSET( _listener, &_readset ) )
            {
                if( int sock = accept( _listener, NULL, NULL ); sock >= 0 )
                {
                    fcntl( sock, F_SETFL, O_NONBLOCK );
                    _clients.insert(sock);
                }
                else
                    throw ServerException( "Sock no accept" );
            }
            
            for( auto it = _clients.begin(); it != _clients.end(); it++ )
            {
                if( FD_ISSET( *it, &_readset ) )  // Считать данные о том если новые данные о потоке
                {
                    std::string content = readFromRecv( *it );
                    if( content.empty() )
                        clearSocket( *it );
                    else
                    {
                        content.pop_back();
                        auto vec = split( content, ' ' );
                        df[*it] = ClientData( vec[0], std::stoi(vec[1]) );
                    }
                }
            }

            for( auto it = df.begin(); it != df.end(); ++it )
            {
                DateTime datetime;
                if( ( ( datetime.get_seconds() % it->second._t ) == 0 ) )
                    std::cout << datetime.to_string() << ' ' << it->second._name << std::endl;
            }

            for( const auto& it : _socketsToClear )
            {
                _clients.erase(it);
            }
            _socketsToClear.clear();
        }
        return 0;
    }

};  // Server

int main()
{
    Server server( "127.0.0.1", 8000 );
    return server.run();
}


