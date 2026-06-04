#include "GameController.hpp"

GameController::~GameController() {
    close();
}

void GameController::openFirstAvailable() {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            open(i);
            if (isOpen()) {
                break;
            }
        }
    }
}

void GameController::onDeviceAdded(int deviceIndex) {
    if (isOpen()) {
        return;  // first pad wins
    }
    if (SDL_IsGameController(deviceIndex)) {
        open(deviceIndex);
    }
}

bool GameController::onDeviceRemoved(SDL_JoystickID instanceId) {
    if (controller_ != nullptr && instanceId == instanceId_) {
        close();
        return true;
    }
    return false;
}

bool GameController::isActive(SDL_JoystickID instanceId) const {
    return controller_ != nullptr && instanceId == instanceId_;
}

void GameController::open(int deviceIndex) {
    controller_ = SDL_GameControllerOpen(deviceIndex);
    if (controller_ != nullptr) {
        SDL_Joystick* js = SDL_GameControllerGetJoystick(controller_);
        instanceId_ = SDL_JoystickInstanceID(js);
    }
}

void GameController::close() {
    if (controller_ != nullptr) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
        instanceId_ = -1;
    }
}
