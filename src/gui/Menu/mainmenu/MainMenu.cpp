#include "Menu/mainmenu/MainMenu.hpp"

#include <exception>
#include <vector>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Graphics/mesh/Mesh.hpp"
#include "Graphics/primitives/Primitives.hpp"
#include "Graphics/shader/Shader.hpp"
#include "types/Event.hpp"
#include "types/Mat.hpp"

namespace Zappy
{

namespace
{
const Vec3 Background(0.06f, 0.07f, 0.10f);
const Vec3 PanelColor(0.10f, 0.11f, 0.15f);
const Vec3 Accent(0.95f, 0.55f, 0.15f);
const Vec3 FieldColor(0.16f, 0.17f, 0.22f);
const Vec3 FieldFocus(0.22f, 0.24f, 0.32f);
const Vec3 ButtonColor(0.20f, 0.22f, 0.28f);
const Vec3 SelectedColor(0.20f, 0.55f, 0.30f);
const Vec3 Label(0.80f, 0.83f, 0.90f);
const Vec3 ErrorColor(0.95f, 0.35f, 0.35f);
} // namespace

MainMenu::MainMenu(Window &window, GraphicsContext &context)
    : _window(window), _context(context), _assets(nullptr), _text(nullptr), _host(), _port(), _mode(RenderMode::TwoD), _focus(Field::Host), _error()
{
}

void MainMenu::init()
{
    MeshData quad = Primitives::quad();

    try
    {
        _assets = std::make_unique<AssetCache>();
        _assets->loadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
        _assets->createMesh("quad", quad.vertices, quad.indices, {3});
        _text = std::make_unique<TextRenderer>();
        _text->loadFont("fonts/BlackOpsOne-Regular.ttf", 22);
        _text->setScreenSize(_window.width(), _window.height());
    }
    catch (const AssetCache::AssetCacheException &e)
    {
        throw MainMenuException(std::string("asset: ") + e.what());
    }
    catch (const TextRenderer::TextRendererException &e)
    {
        throw MainMenuException(std::string("text: ") + e.what());
    }
}

void MainMenu::setDefaults(const std::string &host, int port, RenderMode mode)
{
    _host = host;
    _port = (port > 0) ? std::to_string(port) : "";
    _mode = mode;
    _focus = _host.empty() ? Field::Host : Field::Port;
    _error.clear();
}

void MainMenu::setError(const std::string &error)
{
    _error = error;
}

Color MainMenu::toColor(const Vec3 &color)
{
    return Color{color.r, color.g, color.b, 1.0f};
}

bool MainMenu::contains(const Rect &rect, float px, float py)
{
    return px >= rect.x && px <= rect.x + rect.width && py >= rect.y && py <= rect.y + rect.height;
}

float MainMenu::panelLeft() const
{
    return (static_cast<float>(_window.width()) - PanelWidth) / 2.0f;
}

float MainMenu::panelBottom() const
{
    return (static_cast<float>(_window.height()) - PanelHeight) / 2.0f;
}

MainMenu::Rect MainMenu::hostField() const
{
    float fieldX = panelLeft() + (PanelWidth - FieldWidth) / 2.0f;
    float rowTop = panelBottom() + PanelHeight - Pad - 64.0f;

    return Rect{fieldX, rowTop - FieldHeight, FieldWidth, FieldHeight};
}

MainMenu::Rect MainMenu::portField() const
{
    Rect host = hostField();

    return Rect{host.x, host.y - RowGap, FieldWidth, FieldHeight};
}

MainMenu::Rect MainMenu::modeTwoButton() const
{
    Rect port = portField();
    float third = (FieldWidth - 2.0f * 10.0f) / 3.0f;

    return Rect{port.x, port.y - RowGap, third, FieldHeight};
}

MainMenu::Rect MainMenu::modeThreeButton() const
{
    Rect two = modeTwoButton();

    return Rect{two.x + two.width + 10.0f, two.y, two.width, FieldHeight};
}

MainMenu::Rect MainMenu::modeTorusButton() const
{
    Rect three = modeThreeButton();

    return Rect{three.x + three.width + 10.0f, three.y, three.width, FieldHeight};
}

MainMenu::Rect MainMenu::validateButton() const
{
    Rect mode = modeTwoButton();

    return Rect{hostField().x, mode.y - RowGap, FieldWidth, ButtonHeight};
}

MainMenu::Rect MainMenu::quitButton() const
{
    Rect validate = validateButton();

    return Rect{validate.x, validate.y - 60.0f, FieldWidth, ButtonHeight};
}

void MainMenu::appendChar(int codepoint)
{
    if (_focus == Field::Host)
    {
        if (codepoint > 32 && codepoint < 127 && _host.size() < 253)
            _host.push_back(static_cast<char>(codepoint));
    }
    else if (_focus == Field::Port)
    {
        if (codepoint >= '0' && codepoint <= '9' && _port.size() < 5)
            _port.push_back(static_cast<char>(codepoint));
    }
}

bool MainMenu::tryValidate(MenuConfig &config)
{
    int port = 0;

    if (_host.empty())
    {
        _error = "Host requis";
        return false;
    }
    try
    {
        port = std::stoi(_port);
    }
    catch (const std::exception &)
    {
        _error = "Port invalide";
        return false;
    }
    if (port <= 0 || port > MaxPort)
    {
        _error = "Port hors plage (1-65535)";
        return false;
    }
    config = MenuConfig{MenuResult::Connect, _host, port, _mode};
    return true;
}

bool MainMenu::handleClick(double px, double py, MenuConfig &config)
{
    float fx = static_cast<float>(px);
    float fy = static_cast<float>(_window.height()) - static_cast<float>(py);

    if (contains(hostField(), fx, fy))
        _focus = Field::Host;
    else if (contains(portField(), fx, fy))
        _focus = Field::Port;
    else if (contains(modeTwoButton(), fx, fy))
        _mode = RenderMode::TwoD;
    else if (contains(modeThreeButton(), fx, fy))
        _mode = RenderMode::ThreeD;
    else if (contains(modeTorusButton(), fx, fy))
        _mode = RenderMode::ThreeDTorus;
    else if (contains(validateButton(), fx, fy))
        return tryValidate(config);
    else if (contains(quitButton(), fx, fy))
    {
        config = MenuConfig{MenuResult::Quit, _host, 0, _mode};
        return true;
    }
    else
        _focus = Field::None;
    return false;
}

void MainMenu::drawRect(const Rect &rect, const Vec3 &color)
{
    Shader *shader = _assets->shader("basic");
    Mesh *mesh = _assets->mesh("quad");
    Mat4 model = glm::translate(Mat4(1.0f), Vec3(rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f, 0.0f));

    if (shader == nullptr || mesh == nullptr)
        return;
    model = glm::scale(model, Vec3(rect.width, rect.height, 1.0f));
    shader->use();
    shader->setUniform("uView", Mat4(1.0f));
    shader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(_window.width()), 0.0f, static_cast<float>(_window.height())));
    shader->setUniform("uModel", model);
    shader->setUniform("uColor", color);
    mesh->draw();
}

void MainMenu::drawTextField(const Rect &rect, const std::string &label, const std::string &value, bool focused)
{
    drawRect(rect, focused ? FieldFocus : FieldColor);
    if (focused)
        drawRect(Rect{rect.x, rect.y, rect.width, 3.0f}, Accent);
    _text->drawText(label, rect.x, rect.y + rect.height + 8.0f, toColor(Label));
    _text->drawText(value, rect.x + 12.0f, rect.y + rect.height / 2.0f - 8.0f, toColor(focused ? Accent : Label));
}

void MainMenu::draw()
{
    Rect panel{panelLeft(), panelBottom(), PanelWidth, PanelHeight};
    Rect modeTwo = modeTwoButton();
    Rect modeThree = modeThreeButton();
    Rect modeTorus = modeTorusButton();
    Rect validate = validateButton();
    Rect quit = quitButton();
    float titleX = panelLeft() + Pad;
    float titleY = panelBottom() + PanelHeight - Pad - 28.0f;

    _context.setViewport(_window.width(), _window.height());
    _text->setScreenSize(_window.width(), _window.height());
    _context.setDepthTest(false);
    _window.clear(Color(Background.r, Background.g, Background.b, 1.0f));
    drawRect(panel, PanelColor);
    drawRect(Rect{panel.x, panel.y + panel.height - 4.0f, panel.width, 4.0f}, Accent);
    _text->drawText("ZAPPY", titleX, titleY, toColor(Accent));
    drawTextField(hostField(), "HOST", _host, _focus == Field::Host);
    drawTextField(portField(), "PORT", _port, _focus == Field::Port);
    drawRect(modeTwo, _mode == RenderMode::TwoD ? SelectedColor : ButtonColor);
    drawRect(modeThree, _mode == RenderMode::ThreeD ? SelectedColor : ButtonColor);
    drawRect(modeTorus, _mode == RenderMode::ThreeDTorus ? SelectedColor : ButtonColor);
    _text->drawText("2D", modeTwo.x + modeTwo.width / 2.0f - 14.0f, modeTwo.y + modeTwo.height / 2.0f - 8.0f, toColor(Label));
    _text->drawText("3D", modeThree.x + modeThree.width / 2.0f - 14.0f, modeThree.y + modeThree.height / 2.0f - 8.0f, toColor(Label));
    _text->drawText("TORE", modeTorus.x + modeTorus.width / 2.0f - 28.0f, modeTorus.y + modeTorus.height / 2.0f - 8.0f, toColor(Label));
    drawRect(validate, ButtonColor);
    drawRect(quit, ButtonColor);
    _text->drawText("VALIDER", validate.x + validate.width / 2.0f - 44.0f, validate.y + validate.height / 2.0f - 8.0f, toColor(Accent));
    _text->drawText("QUITTER", quit.x + quit.width / 2.0f - 44.0f, quit.y + quit.height / 2.0f - 8.0f, toColor(Label));
    if (!_error.empty())
        _text->drawText(_error, panel.x + Pad, quit.y - 28.0f, toColor(ErrorColor));
    _context.setDepthTest(true);
    _window.swapBuffers();
}

MenuConfig MainMenu::run()
{
    MenuConfig config{MenuResult::Quit, _host, 0, _mode};

    while (_window.isOpen())
    {
        std::vector<Event> events = _window.pollEvents();

        for (const Event &event : events)
        {
            if (event.type == EventType::Close)
                return MenuConfig{MenuResult::Quit, _host, 0, _mode};
            if (event.type == EventType::Resize)
                _text->setScreenSize(event.width, event.height);
            else if (event.type == EventType::Char)
                appendChar(event.key);
            else if (event.type == EventType::KeyPress && event.key == Backspace)
            {
                if (_focus == Field::Host && !_host.empty())
                    _host.pop_back();
                else if (_focus == Field::Port && !_port.empty())
                    _port.pop_back();
            }
            else if (event.type == EventType::KeyPress && event.key == Tab)
                _focus = (_focus == Field::Host) ? Field::Port : Field::Host;
            else if (event.type == EventType::KeyPress && event.key == Enter)
            {
                if (tryValidate(config))
                    return config;
            }
            else if (event.type == EventType::MouseButton && event.key == 0 && event.pressed)
            {
                if (handleClick(event.mouseX, event.mouseY, config))
                    return config;
            }
        }
        draw();
    }
    return MenuConfig{MenuResult::Quit, _host, 0, _mode};
}

} // namespace Zappy
