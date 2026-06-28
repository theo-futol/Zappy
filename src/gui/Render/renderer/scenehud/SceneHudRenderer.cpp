#include "Render/renderer/scenehud/SceneHudRenderer.hpp"

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

const Vec3 SceneHudRenderer::BarColor(0.07f, 0.07f, 0.09f);
const Vec3 SceneHudRenderer::AccentColor(0.95f, 0.55f, 0.15f);
const Vec3 SceneHudRenderer::CardColor(0.10f, 0.11f, 0.14f);
const Vec3 SceneHudRenderer::LabelColor(0.80f, 0.83f, 0.90f);
const Vec3 SceneHudRenderer::ButtonColor(0.20f, 0.22f, 0.28f);

SceneHudRenderer::SceneHudRenderer(AssetCache &assets, TextRenderer &text) : _assets(assets), _text(text), _width(0), _height(0), _following(false), _pov(false), _cameraFree(false)
{
}

void SceneHudRenderer::setFollowing(bool following)
{
    _following = following;
}

void SceneHudRenderer::setPov(bool pov)
{
    _pov = pov;
}

void SceneHudRenderer::setCameraFree(bool free)
{
    _cameraFree = free;
}

SceneHudRenderer::Button SceneHudRenderer::cameraButton() const
{
    float boxX = static_cast<float>(_width) - BoxWidth - Margin;
    float buttonY = static_cast<float>(_height) - BoxHeight - 2.0f * Margin - ButtonSize;

    return Button{boxX, buttonY, BoxWidth, ButtonSize};
}

bool SceneHudRenderer::cameraButtonHit(double px, double py) const
{
    return contains(cameraButton(), static_cast<float>(px), static_cast<float>(_height) - static_cast<float>(py));
}

SceneHudRenderer::Button SceneHudRenderer::menuButton() const
{
    return Button{Margin, static_cast<float>(_height) - Margin - ButtonSize, MenuButtonWidth, ButtonSize};
}

bool SceneHudRenderer::menuButtonHit(double px, double py) const
{
    return contains(menuButton(), static_cast<float>(px), static_cast<float>(_height) - static_cast<float>(py));
}

Color SceneHudRenderer::toColor(const Vec3 &color)
{
    return Color{color.r, color.g, color.b, 1.0f};
}

void SceneHudRenderer::setScreenSize(int width, int height)
{
    _width = width;
    _height = height;
}

void SceneHudRenderer::drawRect(Shader &shader, Mesh &mesh, float x, float y, float width, float height, const Vec3 &color) const
{
    Mat4 model = glm::translate(Mat4(1.0f), Vec3(x + width / 2.0f, y + height / 2.0f, 0.0f));

    model = glm::scale(model, Vec3(width, height, 1.0f));
    shader.setUniform("uModel", model);
    shader.setUniform("uColor", color);
    mesh.draw();
}

void SceneHudRenderer::drawBar(const RenderContext &context) const
{
    const GameState &state = context.state;
    Shader *shader = _assets.shader("basic");
    Mesh *mesh = _assets.mesh("quad");
    float textY = BarHeight / 2.0f - 8.0f;
    float teamsX = Margin;

    if (shader != nullptr && mesh != nullptr)
    {
        shader->use();
        shader->setUniform("uView", Mat4(1.0f));
        shader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(_width), 0.0f, static_cast<float>(_height)));
        drawRect(*shader, *mesh, 0.0f, 0.0f, static_cast<float>(_width), BarHeight, BarColor);
        drawRect(*shader, *mesh, 0.0f, BarHeight - 3.0f, static_cast<float>(_width), 3.0f, AccentColor);
        for (std::size_t i = 0; i < state.teams().size(); ++i)
        {
            Color c = state.teamColor(state.teams()[i]);

            drawRect(*shader, *mesh, teamsX + static_cast<float>(i) * TeamStep, textY, SwatchSize, SwatchSize, Vec3(c.r, c.g, c.b));
        }
    }
    for (std::size_t i = 0; i < state.teams().size(); ++i)
        _text.drawText(state.teams()[i], teamsX + static_cast<float>(i) * TeamStep + SwatchSize + 8.0f, textY, state.teamColor(state.teams()[i]));
    if (!state.winner().empty())
        _text.drawText("WINNER  " + state.winner(), static_cast<float>(_width) * 0.5f, textY, Color{0.35f, 1.0f, 0.45f, 1.0f});
    if (!state.messages().empty())
        _text.drawText(state.messages().back().text, static_cast<float>(_width) * 0.62f, textY, state.messages().back().color);
}

float SceneHudRenderer::panelHeightFor(const IEntity &selected) const
{
    int rows = (selected.getEntityType() == "player") ? (_following ? 2 : 1) : 0;

    return CardPadding * 2.0f + static_cast<float>(selected.infoLines().size() + 1) * LineHeight + static_cast<float>(rows) * (ButtonSize + CardPadding);
}

float SceneHudRenderer::panelBottomFor(const IEntity &selected) const
{
    float panelTop = static_cast<float>(_height) - BoxHeight - 3.0f * Margin - ButtonSize;

    return panelTop - panelHeightFor(selected);
}

SceneHudRenderer::Button SceneHudRenderer::followButton(const IEntity &selected) const
{
    float panelX = static_cast<float>(_width) - PanelWidth - Margin;

    return Button{panelX + CardPadding, panelBottomFor(selected) + CardPadding, PanelWidth - 2.0f * CardPadding, ButtonSize};
}

SceneHudRenderer::Button SceneHudRenderer::modeButton(const IEntity &selected) const
{
    float panelX = static_cast<float>(_width) - PanelWidth - Margin;

    return Button{panelX + CardPadding, panelBottomFor(selected) + CardPadding + ButtonSize + CardPadding, PanelWidth - 2.0f * CardPadding, ButtonSize};
}

bool SceneHudRenderer::followButtonHit(double px, double py, const GameState &state) const
{
    const IEntity *selected = state.selectedEntity();

    if (selected == nullptr || selected->getEntityType() != "player")
        return false;
    return contains(followButton(*selected), static_cast<float>(px), static_cast<float>(_height) - static_cast<float>(py));
}

bool SceneHudRenderer::modeButtonHit(double px, double py, const GameState &state) const
{
    const IEntity *selected = state.selectedEntity();

    if (selected == nullptr || selected->getEntityType() != "player" || !_following)
        return false;
    return contains(modeButton(*selected), static_cast<float>(px), static_cast<float>(_height) - static_cast<float>(py));
}

void SceneHudRenderer::drawStatePanel(const RenderContext &context) const
{
    const IEntity *selected = context.state.selectedEntity();

    if (selected == nullptr)
        return;

    std::vector<std::string> lines = selected->infoLines();
    bool isPlayer = selected->getEntityType() == "player";
    float panelX = static_cast<float>(_width) - PanelWidth - Margin;
    float panelHeight = panelHeightFor(*selected);
    float panelY = panelBottomFor(*selected);
    Shader *shader = _assets.shader("basic");
    Mesh *mesh = _assets.mesh("quad");
    Button follow = followButton(*selected);
    Button mode = modeButton(*selected);

    if (shader != nullptr && mesh != nullptr)
    {
        shader->use();
        shader->setUniform("uView", Mat4(1.0f));
        shader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(_width), 0.0f, static_cast<float>(_height)));
        drawRect(*shader, *mesh, panelX, panelY, PanelWidth, panelHeight, CardColor);
        drawRect(*shader, *mesh, panelX, panelY + panelHeight - 3.0f, PanelWidth, 3.0f, AccentColor);
        if (isPlayer)
            drawRect(*shader, *mesh, follow.x, follow.y, follow.width, follow.height, ButtonColor);
        if (isPlayer && _following)
            drawRect(*shader, *mesh, mode.x, mode.y, mode.width, mode.height, ButtonColor);
    }
    _text.drawText("SELECTION", panelX + CardPadding, panelY + panelHeight - CardPadding - LineHeight + 4.0f, toColor(AccentColor));
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        float lineY = panelY + panelHeight - CardPadding - static_cast<float>(i + 2) * LineHeight + 4.0f;

        _text.drawText(lines[i], panelX + CardPadding, lineY, toColor(LabelColor));
    }
    if (isPlayer)
        _text.drawText(_following ? "SE DETACHER" : "SUIVRE", follow.x + 10.0f, follow.y + 5.0f, toColor(AccentColor));
    if (isPlayer && _following)
        _text.drawText(_pov ? "VUE: POV" : "VUE: 3EME", mode.x + 10.0f, mode.y + 5.0f, toColor(AccentColor));
}

void SceneHudRenderer::drawTilePanel(const RenderContext &context) const
{
    const GameState &state = context.state;

    if (!state.hasSelectedTile())
        return;

    GridPosition tilePos = state.selectedTile();
    const Tile &tile = state.map().at(tilePos.x, tilePos.y);
    std::vector<std::string> lines = tile.resources().describe(true);

    if (tile.incanting())
        lines.insert(lines.begin(), "INCANTATION");
    if (lines.empty())
        lines.push_back("(vide)");

    float panelHeight = CardPadding * 2.0f + static_cast<float>(lines.size() + 1) * LineHeight;
    float panelX = Margin;
    float panelY = Margin;
    Shader *shader = _assets.shader("basic");
    Mesh *mesh = _assets.mesh("quad");

    if (shader != nullptr && mesh != nullptr)
    {
        shader->use();
        shader->setUniform("uView", Mat4(1.0f));
        shader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(_width), 0.0f, static_cast<float>(_height)));
        drawRect(*shader, *mesh, panelX, panelY, PanelWidth, panelHeight, CardColor);
        drawRect(*shader, *mesh, panelX, panelY + panelHeight - 3.0f, PanelWidth, 3.0f, AccentColor);
    }
    _text.drawText("TILE  " + std::to_string(tilePos.x) + ", " + std::to_string(tilePos.y), panelX + CardPadding, panelY + panelHeight - CardPadding - LineHeight + 4.0f,
                   toColor(AccentColor));
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        float lineY = panelY + panelHeight - CardPadding - static_cast<float>(i + 2) * LineHeight + 4.0f;

        _text.drawText(lines[i], panelX + CardPadding, lineY, toColor(LabelColor));
    }
}

SceneHudRenderer::Button SceneHudRenderer::minusButton() const
{
    float boxX = static_cast<float>(_width) - BoxWidth - Margin;
    float boxY = static_cast<float>(_height) - BoxHeight - Margin;

    return Button{boxX + 12.0f, boxY + 12.0f, ButtonSize, ButtonSize};
}

SceneHudRenderer::Button SceneHudRenderer::plusButton() const
{
    float boxX = static_cast<float>(_width) - BoxWidth - Margin;
    float boxY = static_cast<float>(_height) - BoxHeight - Margin;

    return Button{boxX + BoxWidth - 12.0f - ButtonSize, boxY + 12.0f, ButtonSize, ButtonSize};
}

bool SceneHudRenderer::contains(const Button &button, float px, float py)
{
    return px >= button.x && px <= button.x + button.width && py >= button.y && py <= button.y + button.height;
}

void SceneHudRenderer::drawTimeBox(const RenderContext &context) const
{
    const GameState &state = context.state;
    Shader *shader = _assets.shader("basic");
    Mesh *mesh = _assets.mesh("quad");
    float boxX = static_cast<float>(_width) - BoxWidth - Margin;
    float boxY = static_cast<float>(_height) - BoxHeight - Margin;
    Button minus = minusButton();
    Button plus = plusButton();
    Button camera = cameraButton();

    if (shader != nullptr && mesh != nullptr)
    {
        shader->use();
        shader->setUniform("uView", Mat4(1.0f));
        shader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(_width), 0.0f, static_cast<float>(_height)));
        drawRect(*shader, *mesh, boxX, boxY, BoxWidth, BoxHeight, BarColor);
        drawRect(*shader, *mesh, boxX, boxY + BoxHeight - 3.0f, BoxWidth, 3.0f, AccentColor);
        drawRect(*shader, *mesh, minus.x, minus.y, minus.width, minus.height, ButtonColor);
        drawRect(*shader, *mesh, plus.x, plus.y, plus.width, plus.height, ButtonColor);
        drawRect(*shader, *mesh, camera.x, camera.y, camera.width, camera.height, ButtonColor);
    }
    _text.drawText("TIME UNIT", boxX + 12.0f, boxY + BoxHeight - 28.0f, toColor(AccentColor));
    _text.drawText("-", minus.x + 9.0f, minus.y + 4.0f, toColor(LabelColor));
    _text.drawText("+", plus.x + 6.0f, plus.y + 4.0f, toColor(LabelColor));
    _text.drawText(std::to_string(state.timeUnit()), minus.x + ButtonSize + 14.0f, minus.y + 4.0f, toColor(LabelColor));
    _text.drawText(_cameraFree ? "CAM: VOL LIBRE" : "CAM: ORBITE", camera.x + 10.0f, camera.y + 5.0f, toColor(AccentColor));
}

std::string SceneHudRenderer::handleClick(double px, double py, const GameState &state) const
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

void SceneHudRenderer::drawMenuButton() const
{
    Shader *shader = _assets.shader("basic");
    Mesh *mesh = _assets.mesh("quad");
    Button menu = menuButton();

    if (shader != nullptr && mesh != nullptr)
    {
        shader->use();
        shader->setUniform("uView", Mat4(1.0f));
        shader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(_width), 0.0f, static_cast<float>(_height)));
        drawRect(*shader, *mesh, menu.x, menu.y, menu.width, menu.height, ButtonColor);
    }
    _text.drawText("MENU", menu.x + 12.0f, menu.y + 5.0f, toColor(AccentColor));
}

void SceneHudRenderer::render(const RenderContext &context)
{
    drawBar(context);
    drawTimeBox(context);
    drawStatePanel(context);
    drawTilePanel(context);
    drawMenuButton();
}

} // namespace Zappy
