#include "Render/theme/ThemeRegistry.hpp"

namespace Zappy
{

ThemeRegistry::ThemeRegistry() : _themes()
{
    _themes.push_back(Theme{"assets/trantorian/ice-golem/scene.gltf", "assets/eggs/ice-golem/scene.gltf"});
    _themes.push_back(Theme{"assets/trantorian/sand-golem/scene.gltf", "assets/eggs/sand-golem/scene.gltf"});
    _themes.push_back(Theme{"assets/trantorian/drugdor/scene.gltf", "assets/eggs/drugdor/scene.gltf"});
    _themes.push_back(Theme{"assets/trantorian/sick-golem/scene.gltf", "assets/eggs/sick-golem/scene.gltf"});
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
