#include "Protocol/registry/HandlerRegistry.hpp"

namespace Zappy
{

HandlerRegistry::HandlerRegistry() : _handlers(), _byKey()
{
}

void HandlerRegistry::registerHandler(std::unique_ptr<ICommandHandler> handler)
{
    ICommandHandler *raw = handler.get();

    for (const auto &key : raw->keys())
        _byKey[key] = raw;
    _handlers.push_back(std::move(handler));
}

void HandlerRegistry::dispatch(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    auto it = _byKey.find(key);

    if (it == _byKey.end())
        return;
    it->second->handle(key, args, state);
}

} // namespace Zappy
