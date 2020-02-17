/********************************************************
 * serialport.h
 *
 * Portable Serial Port Class Library for Windows and Unix.
 * @author ysuga (Sugar Sweet Robotics Co., LTD.
 * @date 2020/02/10
 ********************************************************/

#pragma once

#include <exception>
#include <vector>
#include <string>

#ifdef WIN32
#include <windows.h>
#else
#ifdef __linux__
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#define _POSIX_SOURCE 1

#else // OSX
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>
#include <IOKit/IOBSD.h>

#endif // ifdef __linux__

#endif

#include <chrono>

namespace ssr {
  namespace aqua2 {

    class ComException : public std::exception {
    private:
      std::string msg;
    public:
      ComException(const char* msg) {this->msg = msg;}
      ~ComException() throw() {}

    public:
      virtual const char* what() const throw() {return msg.c_str();}
    };

    /**
     * @brief This exception is thrown when Accessing COM Port (Read/Write) is failed.
     */
    class  ComAccessException : public ComException {
    public:
    ComAccessException(void) : ComException("COM Access") {}
      ~ComAccessException(void) throw()	{}
    };

    /**
     * @brief This exception is thrown when Opening COM port is failed.
     */
    class ComOpenException : public ComException  {
    public:
    ComOpenException(void) : ComException ("COM Open Error") {}
      ~ComOpenException(void) throw() {}
    };

    /**
     * @brief  This exception is thrown when COM port state is wrong.
     */
    class ComStateException : public ComException {
    public:
    ComStateException(void) : ComException ("COM State Exception") {}      
      ~ComStateException(void) throw() {}
    };



    

    /***************************************************
     * SerialPort
     *
     * @brief Portable Serial Port Class
     ***************************************************/
    class SerialPort {
    public:
      const static int ODD_PARITY = 0;
      const static int EVEN_PARITY = 1;
      const static int NO_PARITY = 2;
      
      const static int ONE_STOPBIT = 0;
      const static int ONE5_STOPBITS = 1;
      const static int TWO_STOPBITS = 2;
      
    private:
      std::string filename_;
      int baudrate_;
      int parity_;
      int stopbits_;
#ifdef WIN32
      HANDLE m_hComm;
#else
      int m_Fd;
#endif
      
    public:

      /**
       * @brief Constructor
       * 
       * @param filename Filename of Serial Port (eg., "COM0", "/dev/tty0")
       * @baudrate baudrate. (eg., 9600, 115200)
       */
    SerialPort(const char* filename, int baudrate, int parity=NO_PARITY, int stopbits=ONE_STOPBIT) : filename_(filename), baudrate_(baudrate), parity_(parity), stopbits_(stopbits) {
	open();
	setup();
      }
      
    SerialPort(SerialPort&& port) : filename_(port.filename_), baudrate_(port.baudrate_), parity_(port.parity_), stopbits_(port.stopbits_),
#ifdef WIN32
	m_hComm(port.m_hComm)
#else
	m_Fd(port.m_Fd)
#endif
	  {}
      


      /**
       * @brief Destructor
       */
      virtual ~SerialPort() {
	close();
      }
    public:
      bool available() const {
#ifdef WIN32
	return m_hComm != 0;
#else
	return m_Fd != 0;
#endif
      }


      void open() {
#ifdef WIN32
	m_hComm = CreateFileA(filename_.c_str(), GENERIC_READ | GENERIC_WRITE,
			      0, NULL, OPEN_EXISTING, 0, NULL );
	if(m_hComm == INVALID_HANDLE_VALUE) {
	  m_hComm = 0;
	  throw ComOpenException();
	}
#else
	if((m_Fd = ::open(filename_.c_str(), O_RDWR | O_NDELAY /*| O_NOCTTY |O_NONBLOCK*/)) < 0) {
	  m_Fd = 0;
	  throw ComOpenException();
	}
#endif
      }

      void setup() {
	if (!available()) return;
#ifdef WIN32
	DCB dcb;
	if(!GetCommState (m_hComm, &dcb)) {
	  close();
	  throw ComStateException();
	}
	
	dcb.BaudRate           = baudrate_;
	dcb.fParity            = 0;
	dcb.fOutxCtsFlow       = 0;
	dcb.fOutxDsrFlow       = 0;
	dcb.fDtrControl        = RTS_CONTROL_DISABLE;
	dcb.fDsrSensitivity    = 0;
	dcb.fTXContinueOnXoff  = 0;
	dcb.fErrorChar         = 0;
	dcb.fNull              = 0;
	dcb.fRtsControl        = RTS_CONTROL_DISABLE;
	dcb.fAbortOnError      = 0;
	dcb.ByteSize           = 8;
	if (parity_ == NO_PARITY) {
	  dcb.Parity = NOPARITY;
	} else if (parity_ == EVEN_PARITY) {
	  dcb.Parity = EVENPARITY;
	} else if (parity_ == ODD_PARITY) {
	  dcb.Parity = ODDPARITY;
	}
	
	if (stopbits_ == ONE_STOPBIT) {
	  dcb.StopBits_ = ONESTOPBIT;
	} else if (stopbits == ONE5_STOPBITS) {
	  dcb.StopBits_ = ONE5STOPBITS;
	} else if (stopbits == TWO_STOPBITS) {
	  dcb.StopBits_ = TWOSTOPBITS;
	}
	
	if (!SetCommState (m_hComm, &dcb)) {
	  close();
	  throw ComStateException();
	}
#else
	struct termios tio;
	memset(&tio, 0, sizeof(tio));
#ifdef __linux__
	cfsetspeed(&tio, baudrate_);
#else // OSX
	speed_t speed = baudrate_;
	if (ioctl(m_Fd, IOSSIOSPEED, &speed) == -1) {
	  close();
	  throw ComStateException();
	}
#endif
	tio.c_cflag |= CS8 | CLOCAL | CREAD;
	if (stopbits_ == TWO_STOPBITS) {
	  tio.c_cflag |= CSTOPB;
	}
	
	if (parity_ == ODD_PARITY) {
	  tio.c_cflag |= PARENB | PARODD;
	} else if (parity_ == EVEN_PARITY) {
	  tio.c_cflag |= PARENB;
	}
	
	if (tcsetattr(m_Fd, TCSANOW, &tio) < 0) {
	  close();
	  throw ComStateException();
	}
#endif
      }

      void close() {
#ifdef WIN32
	if(m_hComm) {
	  CloseHandle(m_hComm);
	  m_hComm = 0;
	}
#else
	if (m_Fd) {
	  ::close(m_Fd);
	  m_Fd = 0;
	}
#endif
      }
      
    public:
      /**
       * @brief flush receive buffer.
       * @return zero if success.
       */
      void flushRxBuffer() const {
#ifdef WIN32
	if(!PurgeComm(m_hComm, PURGE_RXCLEAR)) {
	  throw ComAccessException();
	}
#else
	if(tcflush(m_Fd, TCIFLUSH) < 0) {
	  throw ComAccessException();
	}
#endif
      }

    
      /**
       * @brief flush transmit buffer.
       * @return zero if success.
       */
      void flushTxBuffer() const {
#ifdef WIN32
	if(!PurgeComm(m_hComm, PURGE_TXCLEAR)) {
	  throw ComAccessException();
	}
#else
	if(tcflush(m_Fd, TCOFLUSH) < 0) {
	  throw ComAccessException();
	}
#endif
      }
    
    public:
      /**
       * @brief Get stored datasize of in Rx Buffer
       * @return Stored Data Size of Rx Buffer;
       */
      int getSizeInRxBuffer() {
#ifdef WIN32
	COMSTAT         stat;
	DWORD           lper;
      
	if(ClearCommError (m_hComm, &lper, &stat) == 0) {
	  throw ComAccessException();
	}
	return stat.cbInQue;
#else
	struct timeval timeout;
	int nread;
	timeout.tv_sec = 0, timeout.tv_usec = 0;
	fd_set fds, dmy;
	FD_ZERO(&fds);
	FD_ZERO(&dmy);
	FD_SET(m_Fd, &fds);
	int res = select(FD_SETSIZE, &fds, &dmy, &dmy, &timeout);
	switch(res) {
	case 0: //timeout
	  return 0;
	case -1: //Error
	  throw ComAccessException();
	default:
	  if(FD_ISSET(m_Fd, &fds)) {
	    ioctl(m_Fd, FIONREAD, &nread);
	    return nread;
	  }
	}
	return 0;
#endif
      }
    
      /**
       * @brief write data to Tx Buffer of Serial Port.
       *
       */
      int write(const void* src, const unsigned int size) const {
	if(size == 0) {
	  return 0;
	}
#ifdef WIN32
	DWORD WrittenBytes;
	if(!WriteFile(m_hComm, src, size, &WrittenBytes, NULL)) {
	  throw ComAccessException();
	}
	return WrittenBytes;
#else
	int ret;
	if((ret = ::write(m_Fd, src, size)) < 0) {
	  throw ComAccessException();
	}
	return ret;
#endif
      }
    
      /**
       * @brief read data from RxBuffer of Serial Port 
       */
      int read(void *dst, const unsigned int size) const {
#ifdef WIN32
	DWORD ReadBytes;
	if(!ReadFile(m_hComm, dst, size, &ReadBytes, NULL)) {
	  throw ComAccessException();
	}
      
	return ReadBytes;
#else
	int ret;
	if((ret = ::read(m_Fd, dst, size))< 0) {
	  throw ComAccessException();
	}
	return ret;
#endif
      }

      int waitAvailable(const uint32_t bytes, const double timeout=0.0) {
	try {
	  auto start = std::chrono::system_clock::now();
	  while (true) {
	    if (getSizeInRxBuffer() >= bytes) {
	      return 0;
	    }
	    if (timeout > 0.0) {
	      double duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now()-start).count();
	      if (duration > timeout) return -2;
	    }
	  }
	} catch (ComAccessException& ex) {
	  return -1;
	}
      }
    
      int readLine(char* dst, const unsigned int maxSize, const char* endMark = "\x0D\x0A") {
	int endMarkLen = strlen(endMark);
	int counter = 0;
	while(true) {
	  try {
	    while (true) {
	      if (getSizeInRxBuffer() >= 1) {
		return 0;
	      }
	    }
	  } catch (ComAccessException& ex) {
	    return -1;
	  }
	  if(read(dst+counter, 1) != 1) return -1;
	  counter++;
	  if(counter >= endMarkLen) {
	    if (strncmp(dst+counter-endMarkLen, endMark, endMarkLen) == 0) return counter;
	  }
	}
      }

    
      int readLineWithTimeout(char* dst, const unsigned int maxSize, const double timeout, const char* endMark = "\x0A\x0D") {
	int endMarkLen = strlen(endMark);
	int counter = 0;
	auto start = std::chrono::system_clock::now();
	while(true) {
	  if(waitAvailable(1) < 0) return -1;
	  if(read(dst+counter, 1) != 1) return -1;
	  counter++;
	  if(counter >= endMarkLen) {
	    if (strncmp(dst+counter-endMarkLen, endMark, endMarkLen) == 0) return counter;
	  }
	  double duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now()-start).count();
	  if (duration > timeout) {
	    return -1;
	  }
	}
      }
    

      /**
       *
       */
      int read(void *dst, const unsigned int size, const double timeout) {
	auto start = std::chrono::system_clock::now();
	while(true) {
	  if(this->getSizeInRxBuffer() >= size) { 
	    return read(dst, size);
	  }
	  double duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now()-start).count();
	  if (duration > timeout) {
	    return -1;
	  }
	}
      }
    
    };


    //// Functional interfaces
    class ByteBuffer : public std::vector<uint8_t> {
    private:
      bool _available;
    public:
      bool available() const { return _available; }
    public:
    ByteBuffer(const bool flag): _available(flag) {}
    ByteBuffer(const size_t size): std::vector<uint8_t>(size), _available(true) {}
    };

    ByteBuffer read(const SerialPort& port, const size_t length) {
      if (!port.available()) { return ByteBuffer(false); }
      ByteBuffer buffer(length);
      try {
	if (port.read(&buffer.front(), length) != length) return ByteBuffer(false); 
      } catch (std::exception& ex) {
	return ByteBuffer(false);
      }
      return buffer;
    }

    SerialPort serialport(const char* filename, int baudrate, int parity=SerialPort::NO_PARITY, int stopbits=SerialPort::ONE_STOPBIT) {
      return SerialPort(filename, baudrate, parity, stopbits);
    }

    const SerialPort& write(const SerialPort& port, const ByteBuffer& buffer) {
      if (!port.available()) return port;
      if (!buffer.available()) return port;
      try {
	if (port.write(&buffer.front(), buffer.size()) != buffer.size()) {
	  
	}
      } catch (std::exception& ex) {
      }
      return port;
    }

    const SerialPort& flushTxBuffer(const SerialPort& port) {
      if (!port.available()) return port;
      try {
	port.flushTxBuffer();
      } catch (std::exception& ex) {
      }
      return port;
    }

    const SerialPort& flushRxBuffer(const SerialPort& port) {
      if (!port.available()) return port;
      try {
	port.flushRxBuffer();
      } catch (std::exception& ex) {
      }
      return port;
    }

    SerialPort& up(SerialPort& port) {
      if (!port.available()) {
	try {
	  port.open();
	  port.setup();
	} catch (std::exception& ex) {
	}
      }
      return port;
    }

    SerialPort& down(SerialPort& port) {
      if (port.available()) {
	try {
	  port.close();
	} catch (std::exception& ex) {
	}
      }
      return port;
    }
      

    

  }; //namespace aqua2
};//namespace ssr



/*******************************************************
 * Copyright  2010, ysuga.net all rights reserved.
 *******************************************************/
