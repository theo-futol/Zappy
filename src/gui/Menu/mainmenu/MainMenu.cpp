#include "Menu/mainmenu/MainMenu.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Graphics/mesh/Mesh.hpp"
#include "Graphics/modelloader/ModelLoader.hpp"
#include "Graphics/primitives/Primitives.hpp"
#include "Graphics/shader/Shader.hpp"
#include "types/Event.hpp"
#include "types/Mat.hpp"

namespace Zappy
{

namespace
{
const Vec3 Background(0.03f, 0.035f, 0.05f);
const Vec3 LightColor(1.0f, 1.0f, 1.0f);    ///< White headlamp light for the cabin.
const Vec3 ModelColor(0.70f, 0.80f, 0.90f); ///< Base tint for untextured parts.
// Art direction: worn steel + amber screens + a cyan monitor, matching the dropship.
const Vec3 PanelColor(0.05f, 0.06f, 0.08f);    ///< Translucent steel console body.
const Vec3 PanelEdge(0.42f, 0.26f, 0.10f);     ///< Rusted amber panel edge.
const Vec3 Accent(0.97f, 0.60f, 0.16f);        ///< Screen amber (primary accent).
const Vec3 AmberDim(0.55f, 0.34f, 0.10f);      ///< Dim amber for idle borders.
const Vec3 Cyan(0.36f, 0.82f, 0.92f);          ///< Monitor cyan (secondary accent).
const Vec3 FieldColor(0.08f, 0.10f, 0.13f);    ///< Input field body.
const Vec3 FieldFocus(0.13f, 0.16f, 0.21f);    ///< Focused input field body.
const Vec3 ButtonColor(0.10f, 0.12f, 0.15f);   ///< Idle button body.
const Vec3 SelectedColor(0.97f, 0.60f, 0.16f); ///< Selected mode button (amber fill).
const Vec3 Steel(0.40f, 0.45f, 0.52f);         ///< Neutral steel line/label.
const Vec3 Label(0.60f, 0.68f, 0.76f);         ///< Muted label text.
const Vec3 ErrorColor(0.96f, 0.36f, 0.32f);
const float PanelAlpha = 0.78f; ///< Console opacity (cabin glows through).
} // namespace

MainMenu::MainMenu(Window &window, GraphicsContext &context)
    : _window(window), _context(context), _assets(nullptr), _text(nullptr), _ship(nullptr), _host(), _port(), _mode(RenderMode::TwoD), _focus(Field::Host), _error(),
      _freeFly(false), _freeEye(0.0f), _freeYaw(0.0f), _freePitch(0.0f), _held(), _lastTime(0.0), _portal(nullptr), _transitioning(false),
      _transitionStart(0.0), _pendingConfig{MenuResult::Quit, "", 0, RenderMode::TwoD}
{
}

void MainMenu::init()
{
    MeshData quad = Primitives::quad();

    try
    {
        _assets = std::make_unique<AssetCache>();
        _assets->loadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
        _assets->loadShader("phong", "shaders/phong.vert", "shaders/phong.frag");
        _assets->loadShader("flash", "shaders/basic.vert", "shaders/flash.frag");
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
    loadShip();
    loadPortal();
}

void MainMenu::loadShip()
{
    ModelLoader loader;

    try
    {
        _ship = std::make_unique<RenderModel>(loader.load(ShipModelPath), *_assets, "menu_ship", ShipModelDir);
    }
    catch (const ModelLoader::ModelLoaderException &e)
    {
        _ship = nullptr; // background is decorative: a missing model must not break the menu
    }
    catch (const RenderModel::RenderModelException &e)
    {
        _ship = nullptr;
    }
}

void MainMenu::loadPortal()
{
    ModelLoader loader;

    try
    {
        _portal = std::make_unique<RenderModel>(loader.load(PortalModelPath), *_assets, "menu_portal", PortalModelDir);
    }
    catch (const ModelLoader::ModelLoaderException &e)
    {
        _portal = nullptr; // transition is optional: fall through instantly if the portal is missing
    }
    catch (const RenderModel::RenderModelException &e)
    {
        _portal = nullptr;
    }
}

void MainMenu::setDefaults(const std::string &host, int port, RenderMode mode)
{
    _host = host;
    _port = (port > 0) ? std::to_string(port) : "";
    _mode = mode;
    _focus = _host.empty() ? Field::Host : Field::Port;
    _error.clear();
    _transitioning = false; // reset so a returning session shows the menu instead of replaying the transition
    _freeFly = false;
    _held.clear();
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

void MainMenu::drawRect(const Rect &rect, const Vec3 &color, float alpha)
{
    Shader *shader = _assets->shader("flash");
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
    shader->setUniform("uAlpha", alpha);
    mesh->draw();
}

void MainMenu::drawOutline(const Rect &rect, const Vec3 &color, float thickness, float alpha)
{
    drawRect(Rect{rect.x, rect.y, rect.width, thickness}, color, alpha);                           // bottom
    drawRect(Rect{rect.x, rect.y + rect.height - thickness, rect.width, thickness}, color, alpha); // top
    drawRect(Rect{rect.x, rect.y, thickness, rect.height}, color, alpha);                          // left
    drawRect(Rect{rect.x + rect.width - thickness, rect.y, thickness, rect.height}, color, alpha); // right
}

void MainMenu::drawCornerBrackets(const Rect &rect, float length, float thickness, const Vec3 &color)
{
    float rx = rect.x;
    float ry = rect.y;
    float rw = rect.width;
    float rh = rect.height;

    drawRect(Rect{rx, ry, length, thickness}, color); // bottom-left
    drawRect(Rect{rx, ry, thickness, length}, color);
    drawRect(Rect{rx + rw - length, ry, length, thickness}, color); // bottom-right
    drawRect(Rect{rx + rw - thickness, ry, thickness, length}, color);
    drawRect(Rect{rx, ry + rh - thickness, length, thickness}, color); // top-left
    drawRect(Rect{rx, ry + rh - length, thickness, length}, color);
    drawRect(Rect{rx + rw - length, ry + rh - thickness, length, thickness}, color); // top-right
    drawRect(Rect{rx + rw - thickness, ry + rh - length, thickness, length}, color);
}

void MainMenu::drawCenteredText(const std::string &text, float centerX, float y, const Color &color, float scale)
{
    _text->drawText(text, centerX - _text->measure(text, scale) / 2.0f, y, color, scale);
}

void MainMenu::drawModeButton(const Rect &rect, const std::string &label, bool selected)
{
    drawRect(rect, selected ? SelectedColor : ButtonColor, selected ? 1.0f : 0.8f);
    drawOutline(rect, selected ? Accent : Steel, 1.5f, selected ? 1.0f : 0.55f);
    drawCenteredText(label, rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f - 8.0f, toColor(selected ? PanelColor : Label), 0.9f);
}

void MainMenu::drawActionButton(const Rect &rect, const std::string &label, bool primary)
{
    drawRect(rect, primary ? AmberDim : ButtonColor, primary ? 0.35f : 0.8f);
    drawOutline(rect, primary ? Accent : Steel, primary ? 2.0f : 1.5f, primary ? 1.0f : 0.55f);
    drawCenteredText(label, rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f - 8.0f, toColor(primary ? Accent : Label), 1.0f);
}

void MainMenu::drawTextField(const Rect &rect, const std::string &label, const std::string &value, bool focused)
{
    drawRect(rect, focused ? FieldFocus : FieldColor, 0.85f);
    drawOutline(rect, focused ? Accent : Steel, 1.5f, focused ? 1.0f : 0.45f);
    if (focused)
        drawRect(Rect{rect.x, rect.y, rect.width, 2.5f}, Accent);
    _text->drawText(label, rect.x, rect.y + rect.height + 9.0f, toColor(Cyan), 0.8f);
    _text->drawText(value.empty() ? "" : value, rect.x + 14.0f, rect.y + rect.height / 2.0f - 8.0f, toColor(focused ? Accent : Label));
}

Vec3 MainMenu::freeForward() const
{
    float yaw = glm::radians(_freeYaw);
    float pitch = glm::radians(_freePitch);

    return glm::normalize(Vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)));
}

void MainMenu::enterFreeFly()
{
    if (_ship == nullptr)
        return;

    Vec3 look = glm::normalize(Vec3(LookX, LookY, LookZ));

    _freeEye = _ship->center() + Vec3(EyeX, EyeY, EyeZ) * _ship->radius();
    _freeYaw = glm::degrees(std::atan2(look.x, look.z));
    _freePitch = glm::degrees(std::asin(glm::clamp(look.y, -1.0f, 1.0f)));
    _lastTime = glfwGetTime();
    _held.clear();
}

void MainMenu::updateFreeCamera()
{
    double now = glfwGetTime();
    float dt = static_cast<float>(now - _lastTime);

    _lastTime = now;
    if (!_freeFly || _ship == nullptr)
        return;
    if (dt <= 0.0f || dt > 0.25f)
        dt = 1.0f / 60.0f; // ignore first-frame / stall spikes

    float lookStep = FreeLookSpeed * dt;

    if (_held.count(KeyLeft))
        _freeYaw -= lookStep;
    if (_held.count(KeyRight))
        _freeYaw += lookStep;
    if (_held.count(KeyUp))
        _freePitch += lookStep;
    if (_held.count(KeyDown))
        _freePitch -= lookStep;
    _freePitch = glm::clamp(_freePitch, -89.0f, 89.0f);

    Vec3 forward = freeForward();
    Vec3 right = glm::normalize(glm::cross(forward, Vec3(0.0f, 1.0f, 0.0f)));
    Vec3 move(0.0f);

    if (_held.count(KeyW))
        move += forward;
    if (_held.count(KeyS))
        move -= forward;
    if (_held.count(KeyD))
        move += right;
    if (_held.count(KeyA))
        move -= right;
    if (_held.count(KeySpace))
        move += Vec3(0.0f, 1.0f, 0.0f);
    if (_held.count(KeyShiftL))
        move -= Vec3(0.0f, 1.0f, 0.0f);
    if (glm::length(move) > 0.0f)
        _freeEye += glm::normalize(move) * (FreeMoveSpeed * _ship->radius() * dt);

    if (!_held.empty())
    {
        Vec3 c = _ship->center();
        float r = _ship->radius();

        std::fprintf(stderr, "[MENU CAM] EyeX=%.3f EyeY=%.3f EyeZ=%.3f  LookX=%.3f LookY=%.3f LookZ=%.3f  (yaw=%.1f pitch=%.1f)\n", (_freeEye.x - c.x) / r, (_freeEye.y - c.y) / r,
                     (_freeEye.z - c.z) / r, forward.x, forward.y, forward.z, _freeYaw, _freePitch);
    }
}

void MainMenu::drawShip()
{
    Shader *shader = _assets->shader("phong");

    if (_ship == nullptr || shader == nullptr)
        return;

    float aspect = static_cast<float>(_window.width()) / static_cast<float>(_window.height());
    Vec3 center = _ship->center();
    float radius = _ship->radius();
    Vec3 eye = center + Vec3(EyeX, EyeY, EyeZ) * radius;
    Vec3 dir;

    if (_freeFly)
    {
        eye = _freeEye;
        dir = freeForward();
    }
    else
    {
        // Weightless spin: yaw turns continuously through a full 360 while a gentle pitch
        // bob keeps the motion feeling like a floating head rather than a flat turntable.
        float t = static_cast<float>(glfwGetTime());
        float yaw = t * SpinSpeed;
        float pitch = glm::radians(PitchSwayDeg) * std::sin(t * PitchSwaySpeed);
        Mat4 sway = glm::rotate(Mat4(1.0f), yaw, Vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(Mat4(1.0f), pitch, Vec3(1.0f, 0.0f, 0.0f));

        dir = Mat3(sway) * glm::normalize(Vec3(LookX, LookY, LookZ));
    }

    Vec3 target = eye + dir;
    Mat4 view = glm::lookAt(eye, target, Vec3(0.0f, 1.0f, 0.0f));
    Mat4 projection = glm::perspective(glm::radians(ShipFov), aspect, ShipNear, ShipFar);

    renderShipModel(view, projection, eye, Mat4(1.0f));
}

void MainMenu::renderShipModel(const Mat4 &view, const Mat4 &projection, const Vec3 &eye, const Mat4 &base)
{
    Shader *shader = _assets->shader("phong");

    if (_ship == nullptr || shader == nullptr)
        return;

    Mat4 model = base * _ship->unitTransform();

    _context.setDepthTest(true);
    shader->use();
    shader->setUniform("uView", view);
    shader->setUniform("uProjection", projection);
    shader->setUniform("uLightPos", eye);
    shader->setUniform("uViewPos", eye);
    shader->setUniform("uLightColor", LightColor);
    shader->setUniform("uTexture", 0);
    shader->setUniform("uBaseColor", ModelColor);
    // The dropship hull is skinned: its raw vertices live in skin space, so drawing
    // them statically drops/misplaces whole walls. Pose it at its rest (bind) pose.
    if (_ship->skinned())
        _ship->drawSkinned(*shader, model, _ship->bindJoints());
    else
        _ship->drawInstanced(*shader, std::vector<Mat4>{model});
}

void MainMenu::startTransition(const MenuConfig &config)
{
    _transitioning = true;
    _transitionStart = glfwGetTime();
    _pendingConfig = config;
    _freeFly = false;
    _held.clear();
}

void MainMenu::drawFlash(float alpha)
{
    Shader *shader = _assets->shader("flash");
    Mesh *mesh = _assets->mesh("quad");
    float w = static_cast<float>(_window.width());
    float h = static_cast<float>(_window.height());

    if (shader == nullptr || mesh == nullptr)
        return;

    Mat4 model = glm::translate(Mat4(1.0f), Vec3(w / 2.0f, h / 2.0f, 0.0f)) * glm::scale(Mat4(1.0f), Vec3(w, h, 1.0f));

    _context.setDepthTest(false);
    shader->use();
    shader->setUniform("uView", Mat4(1.0f));
    shader->setUniform("uProjection", glm::ortho(0.0f, w, 0.0f, h));
    shader->setUniform("uModel", model);
    shader->setUniform("uColor", Vec3(1.0f, 1.0f, 1.0f));
    shader->setUniform("uAlpha", alpha);
    mesh->draw();
}

void MainMenu::drawTransition(float t)
{
    Shader *shader = _assets->shader("phong");
    float aspect = static_cast<float>(_window.width()) / static_cast<float>(_window.height());
    Vec3 center = (_ship != nullptr) ? _ship->center() : Vec3(0.0f);
    float radius = (_ship != nullptr) ? _ship->radius() : 1.0f;
    Vec3 forward(0.0f, 0.0f, 1.0f); // model forward: the cockpit is toward +Z
    Vec3 startEye = center + Vec3(EyeX, EyeY, EyeZ) * radius;
    Vec3 cockpitEye = center + Vec3(CockpitEyeX, CockpitEyeY, CockpitEyeZ) * radius;
    Vec3 restDir = glm::normalize(Vec3(LookX, LookY, LookZ));
    Vec3 eye;
    Vec3 dir;

    float diveStart = WalkFraction + PauseFraction;

    if (t < WalkFraction)
    {
        // Phase 1: walk from the cabin to the cockpit while turning to face forward.
        float p = t / WalkFraction;

        p = p * p * (3.0f - 2.0f * p); // smoothstep ease
        eye = glm::mix(startEye, cockpitEye, p);
        dir = glm::normalize(glm::mix(restDir, forward, p));
    }
    else if (t < diveStart)
    {
        // Phase 2: hold still at the cockpit, facing forward (the calm before the launch).
        eye = cockpitEye;
        dir = forward;
    }
    else
    {
        // Phase 3: dive into the portal. Cubic ease-in = a slow start that builds into a real
        // engine-like acceleration.
        float p = (t - diveStart) / (1.0f - diveStart);

        eye = cockpitEye + forward * (p * p * p * DiveDistance * radius);
        dir = forward;
    }

    Mat4 view = glm::lookAt(eye, eye + dir, Vec3(0.0f, 1.0f, 0.0f));
    Mat4 projection = glm::perspective(glm::radians(ShipFov), aspect, ShipNear, ShipFar);

    _context.setViewport(_window.width(), _window.height());
    _window.clear(Color(0.0f, 0.0f, 0.0f, 1.0f));
    renderShipModel(view, projection, eye, Mat4(1.0f));

    if (_portal != nullptr && shader != nullptr)
    {
        Vec3 portalPos = cockpitEye + forward * (PortalDistance * radius);
        float grow = PortalScale * radius * (0.6f + 0.8f * t);
        Mat4 spin = glm::rotate(Mat4(1.0f), t * glm::radians(PortalSpinDeg), forward);
        Mat4 orient = glm::rotate(Mat4(1.0f), glm::radians(PortalYawDeg), Vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(Mat4(1.0f), glm::radians(PortalPitchDeg), Vec3(1.0f, 0.0f, 0.0f));
        Mat4 base = glm::translate(Mat4(1.0f), portalPos) * spin * orient * glm::scale(Mat4(1.0f), Vec3(grow)) * _portal->unitTransform();

        _context.setDepthTest(true);
        shader->use();
        shader->setUniform("uView", view);
        shader->setUniform("uProjection", projection);
        shader->setUniform("uLightPos", eye);
        shader->setUniform("uViewPos", eye);
        shader->setUniform("uLightColor", LightColor);
        shader->setUniform("uTexture", 0);
        shader->setUniform("uBaseColor", ModelColor);
        _portal->drawInstanced(*shader, std::vector<Mat4>{base});
    }

    if (t > FlashStart)
        drawFlash(glm::clamp((t - FlashStart) / (1.0f - FlashStart), 0.0f, 1.0f));
    _window.swapBuffers();
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
    _window.clear(Color(Background.r, Background.g, Background.b, 1.0f));
    drawShip();
    _context.setDepthTest(false);

    // Console body: translucent steel so the lit cabin glows through.
    drawRect(panel, PanelColor, PanelAlpha);
    drawRect(Rect{panel.x, panel.y, 4.0f, panel.height}, Accent, 0.9f); // left accent rail
    drawOutline(panel, PanelEdge, 1.5f, 0.85f);
    drawCornerBrackets(panel, 22.0f, 3.0f, Accent);

    // Header strip with an amber divider under the title.
    drawRect(Rect{panel.x + Pad, titleY - 12.0f, panel.width - 2.0f * Pad, 2.0f}, Accent, 0.9f);
    _text->drawText("ZAPPY", titleX, titleY, toColor(Accent), 1.5f);

    drawTextField(hostField(), "HOST", _host, _focus == Field::Host);
    drawTextField(portField(), "PORT", _port, _focus == Field::Port);

    _text->drawText("DISPLAY", modeTwo.x, modeTwo.y + modeTwo.height + 9.0f, toColor(Cyan), 0.8f);
    drawModeButton(modeTwo, "2D", _mode == RenderMode::TwoD);
    drawModeButton(modeThree, "3D", _mode == RenderMode::ThreeD);
    drawModeButton(modeTorus, "TORE", _mode == RenderMode::ThreeDTorus);

    drawActionButton(validate, "VALIDER", true);
    drawActionButton(quit, "QUITTER", false);
    if (!_error.empty())
        drawCenteredText(_error, panel.x + panel.width / 2.0f, quit.y - 28.0f, toColor(ErrorColor), 0.85f);
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
            if (_transitioning)
                continue; // animation is playing: swallow all input until it finishes
            if (event.type == EventType::Resize)
            {
                _text->setScreenSize(event.width, event.height);
                continue;
            }
            if (event.type == EventType::KeyPress && event.key == KeyFreeToggle)
            {
                _freeFly = !_freeFly;
                if (_freeFly)
                    enterFreeFly();
                else
                    _held.clear();
                std::fprintf(stderr, "[MENU CAM] free-fly %s\n", _freeFly ? "ON (WASD move, Space/Shift up-down, arrows look)" : "OFF");
                continue;
            }
            if (_freeFly)
            {
                if (event.type == EventType::KeyPress)
                    _held.insert(event.key);
                else if (event.type == EventType::KeyRelease)
                    _held.erase(event.key);
                continue;
            }
            if (event.type == EventType::Char)
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
                    startTransition(config);
            }
            else if (event.type == EventType::MouseButton && event.key == 0 && event.pressed)
            {
                if (handleClick(event.mouseX, event.mouseY, config))
                {
                    if (config.result == MenuResult::Connect)
                        startTransition(config);
                    else
                        return config;
                }
            }
        }
        if (_transitioning)
        {
            float t = static_cast<float>((glfwGetTime() - _transitionStart) / TransitionDuration);

            drawTransition(std::min(t, 1.0f));
            if (t >= 1.0f)
                return _pendingConfig;
            continue;
        }
        updateFreeCamera();
        draw();
    }
    return MenuConfig{MenuResult::Quit, _host, 0, _mode};
}

} // namespace Zappy
