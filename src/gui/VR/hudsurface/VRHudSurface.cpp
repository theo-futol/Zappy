#include "VR/hudsurface/VRHudSurface.hpp"

#include <cmath>

#include "Graphics/primitives/Primitives.hpp"
#include "types/MeshData.hpp"

namespace Zappy
{

namespace
{

constexpr const char *QuadMeshId = "vr_hud_quad";
constexpr const char *ShaderId = "vr_hudpanel";

} // namespace

VRHudSurface::VRHudSurface(AssetCache &assets, GraphicsContext &context, int pixelWidth, int pixelHeight)
    : _assets(assets), _context(context), _pixelWidth(pixelWidth), _pixelHeight(pixelHeight), _framebuffer(0), _colorTexture(0), _depthRenderbuffer(0), _center(0.0f, 1.2f, -1.0f),
      _right(1.0f, 0.0f, 0.0f), _up(0.0f, 1.0f, 0.0f), _normal(0.0f, 0.0f, 1.0f), _worldWidth(1.0f), _worldHeight(0.6f)
{
    MeshData quad = Primitives::groundQuad();

    _assets.createMesh(QuadMeshId, quad.vertices, quad.indices, {3, 3, 2});
    _assets.loadShader(ShaderId, "shaders/hudpanel.vert", "shaders/hudpanel.frag");

    glGenTextures(1, &_colorTexture);
    glBindTexture(GL_TEXTURE_2D, _colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _pixelWidth, _pixelHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _pixelWidth, _pixelHeight);

    glGenFramebuffers(1, &_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRenderbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

VRHudSurface::~VRHudSurface()
{
    if (_framebuffer != 0)
        glDeleteFramebuffers(1, &_framebuffer);
    if (_depthRenderbuffer != 0)
        glDeleteRenderbuffers(1, &_depthRenderbuffer);
    if (_colorTexture != 0)
        glDeleteTextures(1, &_colorTexture);
}

void VRHudSurface::setPose(const Vec3 &center, const Vec3 &forward, float worldWidth, float worldHeight)
{
    _center = center;
    _normal = glm::normalize(forward);
    _right = glm::normalize(glm::cross(Vec3(0.0f, 1.0f, 0.0f), _normal));
    _up = glm::cross(_normal, _right);
    _worldWidth = worldWidth;
    _worldHeight = worldHeight;
}

void VRHudSurface::renderHud(IRenderer &hud, const RenderContext &context)
{
    GLint previousViewport[4];

    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    _context.setViewport(_pixelWidth, _pixelHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _context.setDepthTest(false);
    hud.render(context);
    _context.setDepthTest(true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

void VRHudSurface::renderQuad(const Mat4 &view, const Mat4 &projection)
{
    Shader *shader = _assets.shader(ShaderId);
    Mesh *quad = _assets.mesh(QuadMeshId);

    if (shader == nullptr || quad == nullptr)
        return;

    // groundQuad() lies flat on XZ with a +Y normal; its (X, Y, Z) basis becomes our (right, normal, up).
    Mat4 model(1.0f);

    model[0] = Vec4(_right * _worldWidth, 0.0f);
    model[1] = Vec4(_normal, 0.0f);
    model[2] = Vec4(_up * _worldHeight, 0.0f);
    model[3] = Vec4(_center, 1.0f);

    shader->use();
    shader->setUniform("uModel", model);
    shader->setUniform("uView", view);
    shader->setUniform("uProjection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _colorTexture);
    shader->setUniform("uTexture", 0);
    quad->draw();
}

bool VRHudSurface::hitTest(const Ray &ray, double &outPixelX, double &outPixelY) const
{
    float denom = glm::dot(_normal, ray.direction);

    if (std::fabs(denom) < 1e-6f)
        return false;

    float distance = glm::dot(_center - ray.origin, _normal) / denom;

    if (distance < 0.0f)
        return false;

    Vec3 hit = ray.origin + ray.direction * distance;
    Vec3 local = hit - _center;
    float u = glm::dot(local, _right) / _worldWidth + 0.5f;
    float v = glm::dot(local, _up) / _worldHeight + 0.5f;

    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
        return false;

    outPixelX = static_cast<double>(u) * _pixelWidth;
    outPixelY = static_cast<double>(1.0f - v) * _pixelHeight;
    return true;
}

} // namespace Zappy
