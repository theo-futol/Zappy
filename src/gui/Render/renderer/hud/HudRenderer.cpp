#include "Render/renderer/hud/HudRenderer.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Command/commandbuilder/CommandBuilder.hpp"
#include "Model/gamestate/GameState.hpp"
#include "Model/tile/Tile.hpp"
#include "interface/IEntity.hpp"
#include "types/GridPosition.hpp"
#include "types/Mat.hpp"

namespace Zappy
{

HudRenderer::HudRenderer(AssetCache &assets, TextRenderer &text) : _assets(assets), _text(text), _width(0), _height(0)
{
}

int HudRenderer::panelWidth(int windowWidth)
{
    return static_cast<int>(static_cast<float>(windowWidth) * PanelRatio);
}

void HudRenderer::setScreenSize(int width, int height)
{
    _width = width;
    _height = height;
}

void HudRenderer::drawRect(Shader &shader, Mesh &mesh, float x, float y, float width, float height, const Vec3 &color) const
{
    Mat4 model = glm::translate(Mat4(1.0f), Vec3(x + width / 2.0f, y + height / 2.0f, 0.0f));

    model = glm::scale(model, Vec3(width, height, 1.0f));
    shader.setUniform("uModel", model);
    shader.setUniform("uColor", color);
    mesh.draw();
}

float HudRenderer::headerLineY() const
{
    return static_cast<float>(_height) - TitleHeight - Margin - LineHeight;
}

HudRenderer::Button HudRenderer::plusButton() const
{
    return Button{static_cast<float>(_width) - Margin - ButtonSize, headerLineY() - 6.0f, ButtonSize, ButtonSize};
}

HudRenderer::Button HudRenderer::minusButton() const
{
    return Button{static_cast<float>(_width) - Margin - 2.0f * ButtonSize - ButtonGap, headerLineY() - 6.0f, ButtonSize, ButtonSize};
}

HudRenderer::Button HudRenderer::menuButton() const
{
    return Button{static_cast<float>(_width) - Margin - MenuButtonWidth, static_cast<float>(_height) - TitleHeight + (TitleHeight - ButtonSize) / 2.0f, MenuButtonWidth,
                  ButtonSize};
}

bool HudRenderer::contains(const Button &button, float px, float py)
{
    return px >= button.x && px <= button.x + button.width && py >= button.y && py <= button.y + button.height;
}

bool HudRenderer::menuButtonHit(double px, double py) const
{
    return contains(menuButton(), static_cast<float>(px), static_cast<float>(_height) - static_cast<float>(py));
}

std::string HudRenderer::handleClick(double px, double py, const GameState &state) const
{
    float fx = static_cast<float>(px);
    float fy = static_cast<float>(_height) - static_cast<float>(py);
    int base = state.timeUnit() < MinTimeUnit ? MinTimeUnit : state.timeUnit();

    if (contains(minusButton(), fx, fy))
        return CommandBuilder::setTimeUnit(std::max(MinTimeUnit, base / 2));
    if (contains(plusButton(), fx, fy))
        return CommandBuilder::setTimeUnit(std::min(MaxTimeUnit, base * 2));
    return "";
}

void HudRenderer::render(const RenderContext &context)
{
    Shader *shader = _assets.shader("basic");
    Mesh *mesh = _assets.mesh("quad");
    const GameState &state = context.state;
    int panel = panelWidth(_width);
    float panelX = static_cast<float>(_width - panel);
    float contentX = panelX + Margin;
    float headerY = static_cast<float>(_height) - TitleHeight - Margin - LineHeight;
    float teamsTop = headerY - 3.0f * LineHeight;
    Button minus = minusButton();
    Button plus = plusButton();
    Button menu = menuButton();

    if (shader != nullptr && mesh != nullptr)
    {
        shader->use();
        shader->setUniform("uView", Mat4(1.0f));
        shader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(_width), 0.0f, static_cast<float>(_height)));
        drawRect(*shader, *mesh, panelX, 0.0f, static_cast<float>(panel), static_cast<float>(_height), PanelColor);
        drawRect(*shader, *mesh, panelX, static_cast<float>(_height) - TitleHeight, static_cast<float>(panel), TitleHeight, AccentColor);
        drawRect(*shader, *mesh, panelX, 0.0f, SeparatorWidth, static_cast<float>(_height), AccentColor);
        drawRect(*shader, *mesh, contentX, static_cast<float>(_height) - TitleHeight + (TitleHeight - LogoSize) / 2.0f, LogoSize, LogoSize, PlaceholderColor);
        int row = 0;
        for (const std::string &team : state.teams())
        {
            Color c = state.teamColor(team);

            drawRect(*shader, *mesh, contentX, teamsTop - static_cast<float>(row) * LineHeight, SwatchSize, SwatchSize, Vec3(c.r, c.g, c.b));
            ++row;
        }
        drawRect(*shader, *mesh, minus.x, minus.y, minus.width, minus.height, ButtonColor);
        drawRect(*shader, *mesh, plus.x, plus.y, plus.width, plus.height, ButtonColor);
        drawRect(*shader, *mesh, menu.x, menu.y, menu.width, menu.height, ButtonColor);
    }
    Color label{0.80f, 0.83f, 0.90f, 1.0f};
    _text.drawText("ZAPPY", contentX + LogoSize + 14.0f, static_cast<float>(_height) - TitleHeight / 2.0f - 8.0f, Color{0.07f, 0.07f, 0.09f, 1.0f});
    _text.drawText("TIME UNIT  " + std::to_string(state.timeUnit()), contentX, headerY, label);
    _text.drawText("-", minus.x + 9.0f, headerY, label);
    _text.drawText("+", plus.x + 6.0f, headerY, label);
    _text.drawText("MENU", menu.x + 10.0f, menu.y + 5.0f, Color{0.95f, 0.55f, 0.15f, 1.0f});
    _text.drawText("TEAMS", contentX, headerY - 2.0f * LineHeight, label);
    int row = 0;
    for (const std::string &team : state.teams())
    {
        _text.drawText(team, contentX + SwatchSize + 10.0f, teamsTop - static_cast<float>(row) * LineHeight, state.teamColor(team));
        ++row;
    }
    if (!state.winner().empty())
    {
        float winnerY = teamsTop - static_cast<float>(state.teams().size()) * LineHeight - LineHeight;

        _text.drawText("WINNER  " + state.winner(), contentX, winnerY, Color{0.35f, 1.0f, 0.45f, 1.0f});
    }
    if (state.hasSelectedTile())
    {
        GridPosition tilePos = state.selectedTile();
        const Tile &tile = state.map().at(tilePos.x, tilePos.y);
        std::vector<std::string> resources = tile.resources().describe(true);
        float tileY = static_cast<float>(_height) * 0.80f;
        int line = 1;

        _text.drawText("TILE  " + std::to_string(tilePos.x) + ", " + std::to_string(tilePos.y), contentX, tileY, Color{0.95f, 0.55f, 0.15f, 1.0f});
        if (tile.incanting())
        {
            _text.drawText("INCANTATION", contentX, tileY - static_cast<float>(line) * LineHeight, Color{0.70f, 0.45f, 1.0f, 1.0f});
            ++line;
        }
        if (resources.empty())
            _text.drawText("(vide)", contentX, tileY - static_cast<float>(line) * LineHeight, label);
        else
            for (const std::string &resource : resources)
            {
                _text.drawText(resource, contentX, tileY - static_cast<float>(line) * LineHeight, label);
                ++line;
            }
    }
    const IEntity *selected = state.selectedEntity();
    if (selected != nullptr)
    {
        float selY = static_cast<float>(_height) * 0.55f;
        int line = 1;

        _text.drawText("SELECTED", contentX, selY, Color{0.95f, 0.55f, 0.15f, 1.0f});
        for (const std::string &info : selected->infoLines())
        {
            _text.drawText(info, contentX, selY - static_cast<float>(line) * LineHeight, label);
            ++line;
        }
    }
    if (!state.winner().empty() && shader != nullptr && mesh != nullptr)
    {
        float bandHeight = 170.0f;
        float bandY = static_cast<float>(_height) * 0.5f - bandHeight * 0.5f;
        std::string title = "VICTOIRE";
        std::string team = state.winner();
        float titleScale = 3.0f;
        float teamScale = 1.9f;

        drawRect(*shader, *mesh, 0.0f, bandY, static_cast<float>(_width), bandHeight, Vec3(0.07f, 0.07f, 0.09f));
        drawRect(*shader, *mesh, 0.0f, bandY + bandHeight - 3.0f, static_cast<float>(_width), 3.0f, AccentColor);
        drawRect(*shader, *mesh, 0.0f, bandY, static_cast<float>(_width), 3.0f, AccentColor);
        _text.drawText(title, (static_cast<float>(_width) - _text.measure(title, titleScale)) / 2.0f, bandY + bandHeight * 0.55f, Color{0.95f, 0.55f, 0.15f, 1.0f}, titleScale);
        _text.drawText(team, (static_cast<float>(_width) - _text.measure(team, teamScale)) / 2.0f, bandY + bandHeight * 0.20f, state.teamColor(team), teamScale);
    }
}

} // namespace Zappy
