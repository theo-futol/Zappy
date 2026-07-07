#pragma once

#include <array>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Graphics/context/GraphicsContext.hpp"
#include "Graphics/text/TextRenderer.hpp"
#include "Graphics/window/Window.hpp"
#include "Input/inputhandler/InputHandler.hpp"
#include "Model/gamestate/GameState.hpp"
#include "Render/asset/AssetCache.hpp"
#include "Render/camera/free/FreeCamera.hpp"
#include "Render/camera/orbit/OrbitCamera.hpp"
#include "Render/camera/topdown/TopDownCamera.hpp"
#include "Render/projection/torus/TorusProjection.hpp"
#include "Render/renderer/hud/HudRenderer.hpp"
#include "Render/renderer/scenehud/SceneHudRenderer.hpp"
#include "VR/handtracking/VRHandTracking.hpp"
#include "VR/hudsurface/VRHudSurface.hpp"
#include "VR/input/VRInput.hpp"
#include "VR/interaction/VRPointer.hpp"
#include "VR/rig/VRRig.hpp"
#include "VR/session/VRSession.hpp"
#include "VR/tabletop/VRTabletop.hpp"
#include "interface/ICamera.hpp"
#include "interface/IProjection.hpp"
#include "interface/IRenderer.hpp"
#include "types/Orientation.hpp"
#include "types/RenderMode.hpp"

namespace Zappy
{

/**
 * @class RenderSystem
 * @brief Owns the window, GPU resources and renderers; drives one frame of output.
 */
class RenderSystem
{
  public:
    /**
     * @class RenderSystemException
     * @brief Error raised during render setup or while loading assets.
     */
    class RenderSystemException : public std::exception
    {
      public:
        explicit RenderSystemException(const std::string &message) : _message(message)
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
     * @brief Builds the render system on top of an existing window and context.
     * @param window Window owned by the caller (borrowed, must outlive this system).
     * @param context Graphics context owned by the caller (borrowed, must outlive this system).
     * @param mode Display mode selecting the view pipeline (2D or 3D).
     * @param vrRequested Whether to attempt a VR session on top of that pipeline (best-effort:
     *        falls back to desktop rendering with a warning if no OpenXR runtime/headset answers).
     */
    RenderSystem(Window &window, GraphicsContext &context, RenderMode mode, bool vrRequested = false);

    RenderSystem(const RenderSystem &) = delete;
    RenderSystem &operator=(const RenderSystem &) = delete;
    RenderSystem(RenderSystem &&) = delete;
    RenderSystem &operator=(RenderSystem &&) = delete;

    /**
     * @brief Creates the window and context and loads the renderers and assets.
     * @throws RenderSystemException On window, context or asset failure.
     */
    void init();

    /**
     * @brief Draws one frame from the given state.
     * @param state Game state to render.
     */
    void render(const GameState &state);

    /**
     * @brief Polls input and updates the camera and selection.
     * @param state State updated on selection.
     * @return False when the user asked to quit, true otherwise.
     */
    bool processInput(GameState &state);

    /**
     * @brief Takes the commands queued this frame (e.g. from HUD buttons or selection).
     * @return The pending command lines, moved out (the queue is left empty).
     */
    std::vector<std::string> takeOutgoing();

    /**
     * @brief Whether the user clicked the HUD "MENU" button (asking to return to the main menu).
     * @return True if the loop should end to show the menu rather than quit.
     */
    bool wantsMenu() const;

    /**
     * @brief Whether a VR session was successfully established (vrRequested was true and it worked).
     * @return True if render()/processInput() are driving the headset instead of the desktop path.
     */
    bool vrEnabled() const;

    /**
     * @brief Non-fatal message explaining why VR isn't active, set when vrRequested was true but
     * session setup failed (no runtime, no headset, Wayland instead of X11, ...).
     * @return The warning, or an empty string if VR wasn't requested or started successfully.
     */
    const std::string &vrWarning() const;

  private:
    /**
     * @brief Loads the 2D pipeline: top-down camera, planar projection, assets, renderers, HUD.
     * @throws RenderSystemException On asset or text loading failure.
     */
    void initTwoD();

    /**
     * @brief Loads the 3D pipeline: orbit camera (the scene stays empty until the 3D renderers land).
     */
    void initThreeD();

    /**
     * @brief Picks the entity under a 2D (top-down) click and updates the selection.
     * @param px Click X in pixels.
     * @param py Click Y in pixels.
     * @param state State whose selection is set or cleared.
     */
    void selectAt(double px, double py, GameState &state);

    /**
     * @brief Picks the entity under a 3D click by casting a ray onto the ground.
     * @param px Click X in pixels.
     * @param py Click Y in pixels.
     * @param state State whose selection is set or cleared.
     */
    void selectAt3D(double px, double py, GameState &state);

    /**
     * @brief Selects the entity standing on a tile (preferring players), or clears the selection.
     * @param tileX Tile X coordinate.
     * @param tileY Tile Y coordinate.
     * @param state State whose selection is set or cleared.
     */
    void selectEntityAt(int tileX, int tileY, GameState &state);

    /**
     * @brief Routes a 2D-mode click to the HUD's buttons, shared by the desktop and VR input paths.
     * @param clickX Click X in pixels (desktop cursor, or a VR ray/panel hit converted to HUD pixels).
     * @param clickY Click Y in pixels.
     * @param state State updated by the HUD.
     * @param pickMapFallback Receives true if nothing in the HUD consumed the click (caller should
     *        fall back to its own map picking: selectAt() for the mouse, VRTabletop::pickTile() for VR).
     * @return False if the MENU button was hit (caller should stop and return to the menu).
     */
    bool handleHudClick(double clickX, double clickY, GameState &state, bool &pickMapFallback);

    /**
     * @brief Routes a 3D-mode click to the scene HUD's buttons, shared by the desktop and VR input paths.
     * @param clickX Click X in pixels (desktop cursor, or a VR ray/panel hit converted to HUD pixels).
     * @param clickY Click Y in pixels.
     * @param state State updated by the HUD (camera/follow/mode toggles) or a queued command.
     * @param pickMapFallback Receives true if nothing in the HUD consumed the click (caller should
     *        fall back to its own map picking: selectAt3D() for the mouse, VRPointer for VR).
     * @return False if the MENU button was hit (caller should stop and return to the menu).
     */
    bool handleSceneHudClick(double clickX, double clickY, GameState &state, bool &pickMapFallback);

    /**
     * @brief Finds the tile under a screen point by projecting every tile and taking the nearest one.
     *
     * Screen-space picking: each tile center is projected to the screen; among the tiles whose
     * projection lands within a pixel radius of the cursor, the one closest to the camera (front
     * face of the torus) wins. Works on any curved surface, unlike a ray-plane intersection.
     * @param px Screen X in pixels.
     * @param py Screen Y in pixels.
     * @param state State read for the map dimensions and projection.
     * @param tileX Receives the hit tile X.
     * @param tileY Receives the hit tile Y.
     * @return True if a tile was found under the cursor, false otherwise.
     */
    bool groundTile(double px, double py, const GameState &state, int &tileX, int &tileY) const;

    /**
     * @brief Updates the hovered tile in the state from the current cursor position (3D mode).
     * @param state State whose hovered tile is set or cleared.
     */
    void updateHover(GameState &state);

    /**
     * @brief Resolves the entity currently followed, auto-detaching if it is gone or not a player.
     * @param state State holding the selected/followed entity.
     * @return The followed player, or nullptr if not following.
     */
    const IEntity *resolveFollowed(const GameState &state);

    /**
     * @brief Stops following and recenters the orbit camera on the map center.
     * @param state State read for the map dimensions.
     */
    void detachFollow(const GameState &state);

    /**
     * @brief Orbit yaw (degrees) whose horizontal forward matches a cardinal orientation.
     * @param orientation Entity orientation.
     * @return The yaw to aim a POV in that direction.
     */
    static float orientationYaw(Orientation orientation);

    /**
     * @brief Attempts to establish a VR session on top of whichever pipeline init() already built.
     *
     * Best-effort and never fatal: catches VRSession::VRSessionException (and friends) and leaves
     * _vrEnabled false with _vrWarning set, so the caller keeps rendering the desktop path.
     * @param vrRequested Whether the caller asked for VR (--vr); does nothing if false.
     */
    void initVR(bool vrRequested);

    /**
     * @brief VR counterpart of processInput(): syncs actions/poses and handles VR-specific input.
     * @param state State updated on selection.
     * @return False when the user asked to quit or the runtime asked to exit, true otherwise.
     */
    bool processInputVR(GameState &state);

    /**
     * @brief VR counterpart of render(): submits one stereo frame via the OpenXR session.
     * @param state Game state to render.
     */
    void renderVR(const GameState &state);

    /**
     * @brief Builds the view/projection pair for one eye, folding in the VR rig and (2D mode) the tabletop.
     * @param eye Eye pose/fov from VRSession::locateViews().
     * @return The eye's view and projection matrices.
     */
    std::pair<Mat4, Mat4> vrEyeMatrices(const VRSession::EyeView &eye) const;

    /**
     * @brief Routes a VR "select" press (trigger or pinch) on one hand to HUD hit-testing or map picking.
     * @param hand VRInput::LeftHand or VRInput::RightHand.
     * @param state State updated on selection or by a HUD button.
     */
    void handleVRSelect(int hand, GameState &state);

    static constexpr const char *FlatGroundModelPath = "assets/ground/scene.gltf";                    ///< Flat ground model (3D flat mode).
    static constexpr const char *TorusModelPath = "assets/torus/scene.gltf";                          ///< Torus world model (3D torus mode, the planet shell).
    static constexpr const char *SkyCrossPath = "assets/sky/Cubemap/Cubemap_Sky_15-512x512.png";     ///< 4x3 cross sky image in 3D mode.
    static constexpr float GroundEpsilon = 1e-6f;                                                    ///< Min |ray.y| to intersect the ground plane.
    static constexpr float FollowEye = 1.0f;                                                         ///< Height of the follow-camera target above the tile.
    static constexpr float FollowRadius = 2.5f;                                                      ///< Initial follow-camera distance (bounding radius to fit).
    static constexpr float TorusMajor = 12.5f;                                                       ///< Torus major radius R (world units).
    static constexpr float TorusMinor = 3.0f;                                                        ///< Torus minor radius r (world units).
    static constexpr float TorusFramePad = 1.4f;                                                     ///< Multiplier on (R + r) for the initial camera framing.
    static constexpr float PickRadiusPx = 32.0f;                                                     ///< Screen-space pick radius around the cursor (pixels).

    int _width;  ///< Window width in pixels.
    int _height; ///< Window height in pixels.

    Window &_window;           ///< Borrowed window (owned by Core, shared with the menu).
    GraphicsContext &_context; ///< Borrowed OpenGL state (owned by Core).
    std::unique_ptr<AssetCache> _assets;       ///< Owned GPU resource cache.
    std::unique_ptr<TextRenderer> _text;       ///< Owned text renderer.

    std::unique_ptr<ICamera> _camera;         ///< Active camera, chosen by mode (owning, polymorphic).
    std::unique_ptr<IProjection> _projection; ///< Active grid-to-world mapping, chosen by mode.
    TopDownCamera *_topDown;                  ///< Non-owning view of _camera in 2D mode (for picking/pan); null in 3D.
    OrbitCamera *_orbit;                      ///< Non-owning view of _camera in 3D mode (for orbit/dolly); null in 2D.
    TorusProjection *_torus;                  ///< Non-owning view of _projection in 3D mode (for grid size); null in 2D.
    InputHandler _input;                      ///< Input translation.

    std::vector<std::unique_ptr<IRenderer>> _renderers; ///< Ordered world render stages.
    std::unique_ptr<HudRenderer> _hud;                  ///< 2D HUD overlay (drawn separately, full viewport, depth off).
    std::unique_ptr<SceneHudRenderer> _sceneHud;        ///< 3D HUD overlay (bottom bar + selection card).
    bool _framed;                                       ///< Whether the map was auto-framed once (then user controls).
    std::vector<std::string> _outgoing;                 ///< Commands queued this frame, drained by Core.
    RenderMode _mode;                                   ///< Active display mode (selects the view pipeline).
    double _mouseX;                                     ///< Last known cursor X in pixels (for hover).
    double _mouseY;                                     ///< Last known cursor Y in pixels (for hover).
    bool _following;                                    ///< Whether the camera follows the selected entity (3D).
    bool _pov;                                           ///< Follow sub-mode: true = first-person POV, false = third-person orbit.
    FreeCamera _free;                                   ///< Free-fly camera (3D, alternative to orbit).
    bool _freeFly;                                      ///< Whether the 3D camera is in free-fly mode (when not following).
    bool _returnToMenu;                                 ///< Set when the HUD "MENU" button is clicked (back to the main menu).

    bool _vrRequested;                                  ///< Whether the caller asked for VR (--vr), regardless of whether it worked.
    bool _vrEnabled;                                    ///< True once a VR session was successfully established.
    std::string _vrWarning;                             ///< Non-fatal reason VR isn't active (empty if not requested or running).
    std::unique_ptr<VRSession> _vrSession;              ///< OpenXR instance/session/swapchain owner (null unless VR is enabled).
    std::unique_ptr<VRInput> _vrInput;                  ///< Controller/hand action set (null unless VR is enabled).
    std::unique_ptr<VRHandTracking> _vrHands;           ///< Optional XR_EXT_hand_tracking wrapper (null if unsupported).
    VRRig _vrRig;                                       ///< Headset pose + locomotion, folded into per-eye matrices.
    std::unique_ptr<VRHudSurface> _vrHud;               ///< World-space quad presenting the active HUD renderer.
    std::unique_ptr<VRTabletop> _vrTabletop;            ///< 2D-mode table placement (null outside RenderMode::TwoD).
    bool _vrFramed;                                     ///< Whether the VR rig/tabletop was auto-framed once.
    double _vrDisplayTime;                              ///< Predicted display time for the current frame (from beginFrame()).
    bool _vrShouldRender;                               ///< Whether this frame should actually be rendered (from beginFrame()).
    std::array<VRSession::EyeView, VRSession::EyeCount> _vrEyeViews; ///< Eye poses/fovs located this frame.
    static constexpr float VRNearPlane = 0.05f;         ///< Near clip distance for VR eye projections.
    static constexpr float VRFarPlane = 5000.0f;        ///< Far clip distance for VR eye projections.
    static constexpr float VRMoveMetersPerSecond = 1.4f; ///< Thumbstick locomotion speed.
    static constexpr float VRTurnDegreesPerSecond = 90.0f; ///< Smooth-turn speed for the turn thumbstick.
    static constexpr float VRPickRadius = 0.4f;         ///< World-space pick radius for VRPointer (3D modes).
};

} // namespace Zappy
