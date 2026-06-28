#include "Render/theme/ThemeRegistry.hpp"

namespace Zappy
{

ThemeRegistry::ThemeRegistry() : _themes()
{
    _themes.push_back(Theme{"assets/transformers/optimus/scene.gltf", "assets/eggs/drugdor/scene.gltf"});
    _themes.push_back(Theme{"assets/transformers/mirage/scene.gltf", "assets/eggs/drugdor/scene.gltf"});
    _themes.push_back(Theme{"assets/transformers/megatron/scene.gltf", "assets/eggs/drugdor/scene.gltf"});
    _themes.push_back(Theme{"assets/transformers/scrapper/scene.gltf", "assets/eggs/drugdor/scene.gltf"});
}

const Theme &ThemeRegistry::forTeam(std::size_t teamIndex) const
{
    return _themes[teamIndex % _themes.size()];
}

std::size_t ThemeRegistry::count() const
{
    return _themes.size();
}

} // namespace Zappy
