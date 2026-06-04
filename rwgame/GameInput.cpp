#include "GameInput.hpp"

#include "engine/GameState.hpp"

#include <SDL_gamecontroller.h>
#include <algorithm>
#include <cmath>
#include <unordered_map>

// Hardcoded Controls SDLK_* -> GameInputState::Control
const std::unordered_multimap<int, GameInputState::Control> kDefaultControls = {
    /* On Foot */
    {SDLK_LCTRL, GameInputState::FireWeapon},
    {SDLK_KP_0, GameInputState::FireWeapon},
    {SDLK_KP_ENTER, GameInputState::NextWeapon},
    {SDLK_KP_PERIOD, GameInputState::LastWeapon},
    {SDLK_w, GameInputState::GoForward},
    {SDLK_UP, GameInputState::GoForward},
    {SDLK_s, GameInputState::GoBackwards},
    {SDLK_DOWN, GameInputState::GoBackwards},
    {SDLK_a, GameInputState::GoLeft},
    {SDLK_LEFT, GameInputState::GoLeft},
    {SDLK_d, GameInputState::GoRight},
    {SDLK_RIGHT, GameInputState::GoRight},
    {SDLK_PAGEUP, GameInputState::ZoomIn},
    {SDLK_z, GameInputState::ZoomIn},
    {SDLK_PAGEDOWN, GameInputState::ZoomOut},
    {SDLK_x, GameInputState::ZoomOut},
    {SDLK_f, GameInputState::EnterExitVehicle},
    {SDLK_RETURN, GameInputState::EnterExitVehicle},
    {SDLK_c, GameInputState::ChangeCamera},
    {SDLK_HOME, GameInputState::ChangeCamera},
    {SDLK_RCTRL, GameInputState::Jump},
    {SDLK_SPACE, GameInputState::Jump},
    {SDLK_LSHIFT, GameInputState::Sprint},
    {SDLK_RSHIFT, GameInputState::Sprint},
    {SDLK_LALT, GameInputState::Walk},
    {SDLK_DELETE, GameInputState::AimWeapon},
    {SDLK_CAPSLOCK, GameInputState::LookBehind},

    /* In Vehicle */
    {SDLK_LCTRL, GameInputState::VehicleFireWeapon},
    {SDLK_a, GameInputState::VehicleLeft},
    {SDLK_LEFT, GameInputState::VehicleLeft},
    {SDLK_d, GameInputState::VehicleRight},
    {SDLK_RIGHT, GameInputState::VehicleRight},
    {SDLK_w, GameInputState::VehicleAccelerate},
    {SDLK_UP, GameInputState::VehicleAccelerate},
    {SDLK_d, GameInputState::VehicleBrake},
    {SDLK_DOWN, GameInputState::VehicleBrake},
    {SDLK_INSERT, GameInputState::ChangeRadio},
    {SDLK_r, GameInputState::ChangeRadio},
    {SDLK_LSHIFT, GameInputState::Horn},
    {SDLK_RSHIFT, GameInputState::Horn},
    {SDLK_KP_PLUS, GameInputState::Submission},
    {SDLK_CAPSLOCK, GameInputState::Submission},
    {SDLK_RCTRL, GameInputState::Handbrake},
    {SDLK_SPACE, GameInputState::Handbrake},
    {SDLK_KP_9, GameInputState::VehicleAimUp},
    {SDLK_KP_2, GameInputState::VehicleAimDown},
    {SDLK_KP_4, GameInputState::VehicleAimLeft},
    {SDLK_KP_6, GameInputState::VehicleAimRight},
    {SDLK_KP_9, GameInputState::VehicleDown},
    {SDLK_KP_2, GameInputState::VehicleUp},
    {SDLK_KP_1, GameInputState::LookLeft},
    {SDLK_q, GameInputState::LookLeft},
    {SDLK_KP_2, GameInputState::LookRight},
    {SDLK_e, GameInputState::LookRight},
};

// Hardcoded Controls SDL_CONTROLLER_BUTTON_* -> GameInputState::Control.
// Buttons may bind multiple controls (on-foot + in-vehicle); the active
// context is resolved later in IngameState, mirroring the keyboard map.
const std::unordered_multimap<int, GameInputState::Control>
    kDefaultControllerButtons = {
        {SDL_CONTROLLER_BUTTON_A, GameInputState::Jump},
        {SDL_CONTROLLER_BUTTON_A, GameInputState::Handbrake},
        {SDL_CONTROLLER_BUTTON_B, GameInputState::Crouch},
        {SDL_CONTROLLER_BUTTON_B, GameInputState::Horn},
        {SDL_CONTROLLER_BUTTON_X, GameInputState::EnterExitVehicle},
        {SDL_CONTROLLER_BUTTON_Y, GameInputState::ChangeCamera},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, GameInputState::NextWeapon},
        // VehicleFireWeapon is an alias of FireWeapon
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
         GameInputState::VehicleFireWeapon},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, GameInputState::LastWeapon},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, GameInputState::LookBehind},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, GameInputState::Sprint},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, GameInputState::ZoomIn},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, GameInputState::ChangeRadio},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, GameInputState::ZoomOut},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, GameInputState::ChangeRadio},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, GameInputState::LookBehind},
};

float GameInput::applyDeadzone(Sint16 raw, float deadzone) {
    // A dead-zone of 1 (or more) silences the axis entirely; also avoids
    // a divide-by-zero in the rescale below.
    if (deadzone >= 1.f) {
        return 0.f;
    }
    float n = std::clamp(raw / 32767.f, -1.f, 1.f);
    float mag = std::abs(n);
    if (mag < deadzone) {
        return 0.f;
    }
    float scaled = (mag - deadzone) / (1.f - deadzone);
    return std::copysign(scaled, n);
}

void GameInput::updateGameInputState(GameInputState *state,
                                     const SDL_Event &event, float deadzone) {
    switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            auto sym = event.key.keysym.sym;
            auto level = event.type == SDL_KEYDOWN ? 1.f : 0.f;
            auto& levels = state->levels;

            auto [rangeBegin, rangeEnd] = kDefaultControls.equal_range(sym);
            for (auto it = rangeBegin; it != rangeEnd; ++it) {
                levels[it->second] = level;
            }
        } break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            auto button = event.cbutton.button;
            auto level =
                event.type == SDL_CONTROLLERBUTTONDOWN ? 1.f : 0.f;
            auto& levels = state->levels;

            auto [rangeBegin, rangeEnd] =
                kDefaultControllerButtons.equal_range(button);
            for (auto it = rangeBegin; it != rangeEnd; ++it) {
                levels[it->second] = level;
            }
        } break;
        case SDL_CONTROLLERAXISMOTION: {
            auto& levels = state->levels;
            float v = applyDeadzone(event.caxis.value, deadzone);
            switch (event.caxis.axis) {
                case SDL_CONTROLLER_AXIS_LEFTX:
                    levels[GameInputState::GoRight] = std::max(0.f, v);
                    levels[GameInputState::GoLeft] = std::max(0.f, -v);
                    break;
                case SDL_CONTROLLER_AXIS_LEFTY:
                    // SDL: up is negative
                    levels[GameInputState::GoForward] = std::max(0.f, -v);
                    levels[GameInputState::GoBackwards] = std::max(0.f, v);
                    break;
                case SDL_CONTROLLER_AXIS_RIGHTX:
                    state->rightStickX = v;
                    break;
                case SDL_CONTROLLER_AXIS_RIGHTY:
                    state->rightStickY = v;
                    break;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                    levels[GameInputState::FireWeapon] = v;
                    levels[GameInputState::VehicleAccelerate] = v;
                    break;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                    levels[GameInputState::AimWeapon] = v;
                    levels[GameInputState::VehicleBrake] = v;
                    break;
                default:
                    break;
            }
        } break;
    }
}
