#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include "interface/INetwork.hpp"

namespace Zappy
{

/**
 * @class TcpSocket
 * @brief Encapsulates the POSIX socket primitive; the only place sockets are touched.
 */
class TcpSocket : public INetwork
{
  public:
    /**
     * @class TcpSocketException
     * @brief Error raised by a socket operation; a kind of INetworkException so the
     * session can catch it at the interface seam.
     */
    class TcpSocketException : public INetworkException
    {
      public:
        explicit TcpSocketException(const std::string &message) : INetworkException(message)
        {
        }
    };

    TcpSocket();
    ~TcpSocket() override;

    /**
     * @brief Opens the connection to the server.
     * @param host Server hostname or address.
     * @param port Server TCP port.
     * @throws TcpSocketException On resolution or connection failure.
     */
    void connect(const std::string &host, int port) override;

    /**
     * @brief Sends raw data to the server.
     * @param data Bytes to send.
     * @throws TcpSocketException On a write error.
     */
    void send(const std::string &data) override;

    /**
     * @brief Reads the data currently available from the server.
     * @return The bytes read (possibly empty).
     * @throws TcpSocketException On a read error.
     */
    std::string receive() override;

  private:
    int _fd; ///< Socket file descriptor, -1 when closed.
};

} // namespace Zappy
