#ifndef RWGAME_GAMECONTROLLER_HPP
#define RWGAME_GAMECONTROLLER_HPP

#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>

/**
 * Owns the active SDL game controller and its hot-plug lifecycle.
 * Policy: "first pad wins" — the first opened controller is bound and
 * others are ignored until it is removed. Holds no input-mapping logic.
 */
class GameController {
public:
    GameController() = default;
    ~GameController();

    GameController(const GameController&) = delete;
    GameController& operator=(const GameController&) = delete;

    /// Open the first already-connected controller, if any.
    void openFirstAvailable();

    /// Handle SDL_CONTROLLERDEVICEADDED (cdevice.which = device index).
    void onDeviceAdded(int deviceIndex);

    /// Handle SDL_CONTROLLERDEVICEREMOVED (cdevice.which = instance id).
    /// @return true if the active controller was the one removed.
    bool onDeviceRemoved(SDL_JoystickID instanceId);

    /// @return true if an event from this instance id is the active pad.
    bool isActive(SDL_JoystickID instanceId) const;

    bool isOpen() const {
        return controller_ != nullptr;
    }

private:
    void open(int deviceIndex);
    void close();

    SDL_GameController* controller_ = nullptr;
    SDL_JoystickID instanceId_ = -1;
};

#endif
