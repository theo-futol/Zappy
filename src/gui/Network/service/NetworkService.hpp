#pragma once

#include <exception>
#include <memory>
#include <string>

#include "Model/gamestate/GameState.hpp"
#include "Protocol/parser/MessageParser.hpp"
#include "Protocol/registry/HandlerRegistry.hpp"
#include "interface/INetwork.hpp"

namespace Zappy
{

/**
 * @class NetworkService
 * @brief Owns the connection and protocol pipeline; turns server bytes into model updates.
 *
 * Holds references into its own members (the parser references the registry), so
 * it is neither copyable nor movable.
 */
class NetworkService
{
  public:
    /**
     * @class NetworkServiceException
     * @brief Error raised by the network session.
     */
    class NetworkServiceException : public std::exception
    {
      public:
        explicit NetworkServiceException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    /**
     * @brief Builds the service.
     * @param state Game state the handlers will mutate.
     * @param socket Transport to own (ownership transferred).
     */
    NetworkService(GameState &state, std::unique_ptr<INetwork> socket);

    NetworkService(const NetworkService &) = delete;
    NetworkService &operator=(const NetworkService &) = delete;
    NetworkService(NetworkService &&) = delete;
    NetworkService &operator=(NetworkService &&) = delete;

    /**
     * @brief Connects to the server and authenticates as the GUI.
     * @param host Server hostname or address.
     * @param port Server TCP port.
     * @throws NetworkServiceException On connection or authentication failure.
     */
    void connect(const std::string &host, int port);

    /**
     * @brief Drains all currently available server data and applies it to the state.
     *
     * Non-blocking: returns immediately when no data is pending. Meant to be called
     * once per frame from the main loop.
     * @throws NetworkServiceException On a transport error (e.g. server disconnect).
     */
    void update();

  private:
    /**
     * @brief Registers the protocol handlers into the registry.
     *
     * Single mount point: adding a handler means one line here.
     */
    void registerHandlers();

    GameState &_state;                 ///< Authoritative state to update.
    HandlerRegistry _registry;         ///< Owned command handlers.
    std::unique_ptr<INetwork> _socket; ///< Owned transport.
    MessageParser _parser;             ///< Stream parser feeding the registry.
};

} // namespace Zappy
