#pragma once

#include <exception>
#include <string>

namespace Zappy
{

/**
 * @class INetwork
 * @brief Contract for a network transport, decoupling session logic from the socket.
 */
class INetwork
{
  public:
    /**
     * @class INetworkException
     * @brief Transport error contract; every INetwork implementation raises this (or a
     * subclass), so callers can catch at the interface seam without knowing the concrete type.
     */
    class INetworkException : public std::exception
    {
      public:
        explicit INetworkException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    virtual ~INetwork() = default;

    /**
     * @brief Opens the connection to the server.
     * @param host Server hostname or address.
     * @param port Server TCP port.
     */
    virtual void connect(const std::string &host, int port) = 0;

    /**
     * @brief Sends raw data to the server.
     * @param data Bytes to send.
     */
    virtual void send(const std::string &data) = 0;

    /**
     * @brief Reads the data currently available from the server.
     * @return The bytes read (possibly empty).
     */
    virtual std::string receive() = 0;
};

} // namespace Zappy
