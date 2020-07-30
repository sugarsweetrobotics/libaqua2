#pragma once

#include <iostream>

#define _WINSOCKAPI_


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>

#include <stdio.h>
#else // WIN32
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <poll.h>
#endif // WIN32


#include <stdint.h>
#include <exception>
#include <string>
#include <sstream>


#pragma comment(lib, "Ws2_32.lib")

namespace ssr {
  namespace aqua2 {
    class SocketInitializer {
    private:
#ifdef WIN32
      WSADATA wsaData;
#else
#endif
      
    public:
      SocketInitializer() {
#ifdef WIN32
	WSAStartup(MAKEWORD(2, 0), &wsaData);
#else
	
#endif
      }
      
      ~SocketInitializer() {
#ifdef WIN32
	//WSAStartup(MAKEWORD(2, 0), &wsaData);
#else
	
#endif
      }
      
      
    };
    
    inline void initializeSocket() {
#ifdef WIN32
      WSADATA wsaData;
      WSAStartup(MAKEWORD(2, 0), &wsaData);
#else
      
#endif
    }
    
    
    
    class SocketException : public std::exception {
    private:
      std::string msg;
    public:
      SocketException() : msg("Unknown") {}
      SocketException(const char* msg_) : msg(msg_) {}
      ~SocketException() throw() {}
      
      
    public:
      const char* what() const throw() {
	return (("SocketException: ") + msg).c_str();
      }
      
    };
    
    /**
     * class Socket.
     */
    class Socket {
    private:
      bool okay_;
      
#ifdef WIN32
      
      SOCKET m_Socket;
      struct sockaddr_in m_SockAddr;
      int len;
      
#else // WIN32
      int m_Socket;
      struct sockaddr_in  m_SockAddr;
      struct hostent*     m_HostEnt;
#endif // WIN32
      
    private:
      void initSocket() {
#ifdef WIN32
	m_Socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (m_Socket == INVALID_SOCKET) {
	  throw SocketException("socket function failed.");
	}
	
#else
	if ((m_Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	  throw SocketException("socket function failed.");
	}
#endif
      }
      
    public:
      bool okay() const { return okay_ ; }

      bool isConnected() {
            /*
            fd_set read_sd;
            FD_ZERO(&read_sd);
            FD_SET(m_Socket, &read_sd);

            fd_set rsd = read_sd;
            struct timeval timevalue;
            timevalue.tv_sec = 0;
            timevalue.tv_usec = 0;
            int sel = select(m_Socket + 1, &rsd, 0, 0, &timevalue);
            if (sel > 0) {
                  char buf[1024] = {0};
                  int bytes = recv(m_Socket, buf, sizeof(buf), 0);
                  if (bytes == 0) return false;
            } else if (sel < 0) {
                  perror("isConnected Error");
            }
            return true;
            */
#ifdef WIN32
          const int numFds = 1;
          WSAPOLLFD fds[numFds];
          fds[0].fd = m_Socket;
          fds[0].events = POLLIN;
          int timeoutMs = 0;

          auto res = WSAPoll(fds, numFds, timeoutMs);
          if (fds[0].revents & (POLLIN | POLLHUP)) {
              return false;
          }
          return true;
#else
            struct pollfd pfd;
            pfd.fd = m_Socket;
            pfd.events = POLLSTANDARD; 
            int timeout = 0;
            poll(&pfd, 1, timeout); 
            auto revents = pfd.revents; 
            if (revents & POLLHUP) {  return false; }
            return true;
#endif            
      }


    public:
      Socket() {
      }
      
      /**
       * Constructor
       */
      Socket(const char* address, const uint32_t port) : okay_(false) {
	      connect(address, port);
      }
      
      Socket(const Socket& socket): okay_(false) {
	      copyFrom(socket);
      }
      
      void operator=(const Socket& socket)  { copyFrom(socket); }
    public:
      void connect(const char* address, const uint32_t port)
      {
       initSocket();
#ifdef WIN32
       m_SockAddr.sin_family = AF_INET;
       m_SockAddr.sin_port = htons(port);
       
       // m_SockAddr.sin_addr.S_un.S_addr = inet_addr(address);
       inet_pton(AF_INET, address, &m_SockAddr.sin_addr.S_un.S_addr);
       if (m_SockAddr.sin_addr.S_un.S_addr == 0xffffffff) {
	 //struct hostent *host;
	 struct addrinfo hints, *addrinfo;
	 
	 //	host = gethostbyname(address);
	 int err;
	 
	 //	if (host == NULL) {
	 if (err = getaddrinfo(address, NULL, &hints, &addrinfo) != 0) {
	   throw SocketException("gethostbyname failed.");
	 }
	 
	 m_SockAddr.sin_addr.S_un = ((struct sockaddr_in *)(addrinfo->ai_addr))->sin_addr.S_un;
	 /*
	   m_SockAddr.sin_addr.S_un.S_addr =
	   *(unsigned int *)host->h_addr_list[0];
	   }
	 */
	 
	 freeaddrinfo(addrinfo);
       }
       if (::connect(m_Socket, (struct sockaddr *)&m_SockAddr, sizeof(m_SockAddr)) < 0) {
	 throw SocketException("connect failed.");
       }
       
       
#else
       memset((char*)&m_SockAddr, 0, sizeof(m_SockAddr));
       
       if( (m_HostEnt=gethostbyname(address))==NULL) {
	 throw SocketException("gethostbyname failed.");
       }
       
       bcopy(m_HostEnt->h_addr, &m_SockAddr.sin_addr, m_HostEnt->h_length);
       m_SockAddr.sin_family = AF_INET;
       m_SockAddr.sin_port   = htons(port);
       
       if (::connect(m_Socket, (struct sockaddr *)&m_SockAddr, sizeof(m_SockAddr)) < 0){
            std::ostringstream ss;
            ss << "Connect Failed. (address=" << address << ", port=" << port << ")";
            throw SocketException(ss.str().c_str());
       }
       okay_ = true;
#endif
      }
      
      void copyFrom(const Socket& socket)
      {
#ifdef WIN32
       okay_ = socket.okay_;
       m_SockAddr = socket.m_SockAddr;
       m_Socket = socket.m_Socket;
       
#else // WIN32
       okay_ = socket.okay_;
       m_SockAddr = socket.m_SockAddr;
       m_Socket = socket.m_Socket;
#endif
      }
      
      
#ifdef WIN32
      Socket(SOCKET hsocket, struct sockaddr_in sockaddr_) : okay_(true)
      {
       m_Socket = hsocket;
       m_SockAddr = sockaddr_;
      }
#else // WIN32
      Socket(int hsocket, struct sockaddr_in& sockaddr_): okay_(true)
	{
            m_Socket = hsocket;
            m_SockAddr = sockaddr_;
	}
      
#endif
      
      /**
       * Desctructor
       */
      ~Socket() {
#ifdef WIN32
		 //closesocket(m_Socket);
#else
		//close();
#endif
      }
      
      int getSizeInRxBuffer()
      {
#ifdef WIN32
       unsigned long count;
       if (::ioctlsocket(m_Socket, FIONREAD, &count) != 0) {
	 throw SocketException("ioctlsocket failed.");
       }
       return count;
#else
       int count = 0;
       if (ioctl(m_Socket, FIONREAD, &count) != 0) {
	 return -1;
       }
       return count;
#endif
      }
      
      int write(const void* src, const unsigned int size)
      {
       
#ifdef WIN32
       return ::send(m_Socket, (const char*)src, size, 0);
#else
       return send(m_Socket, src, size, 0);
#endif
      }
      
      int read(void* dst, const unsigned int size)
      {
#ifdef WIN32
       return ::recv(m_Socket, (char*)dst, size, 0);
#else
       return recv(m_Socket, dst, size, 0);
#endif      
      }
      
      int close()
      {
#ifdef WIN32
       if (::closesocket(m_Socket) < 0)
	 {
	  return -1;
	 }
       return 0;
#else
            if (okay()) {
                  if (::close(m_Socket) < 0) {
                        return -1;
                  }
                  return 0;
            } 
            return 0;
#endif
      }
    };
    
  }
}
