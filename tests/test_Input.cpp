#include <boost/test/unit_test.hpp>

#include <GameInput.hpp>
#include <engine/GameInputState.hpp>

BOOST_AUTO_TEST_SUITE(InputTests)

BOOST_AUTO_TEST_CASE(TestStateUpdate) {
    BOOST_CHECK(GameInputState::Handbrake != GameInputState::Submission);
    BOOST_CHECK(GameInputState::Jump != GameInputState::Sprint);
    {
        // Currently tests against hard-coded input
        GameInputState state;

        SDL_Event ev;
        ev.type = SDL_KEYDOWN;
        ev.key.keysym.sym = SDLK_SPACE;

        GameInput::updateGameInputState(&state, ev);

        // Check that the correct inputs report pressed
        for (int c = 0; c < GameInputState::_MaxControls; ++c) {
            switch (static_cast<GameInputState::Control>(c)) {
                case GameInputState::Jump:
                case GameInputState::Handbrake:
                    BOOST_CHECK(
                        state.pressed(static_cast<GameInputState::Control>(c)));
                    break;
                default:
                    BOOST_CHECK(!state.pressed(
                        static_cast<GameInputState::Control>(c)));
                    break;
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(TestApplyDeadzone) {
    using GameInput::applyDeadzone;
    // Inside the dead-zone clamps to zero
    BOOST_CHECK_SMALL(applyDeadzone(0, 0.2f), 1e-5f);
    BOOST_CHECK_SMALL(applyDeadzone(1000, 0.2f), 1e-5f);
    // Full deflection maps to +/- 1
    BOOST_CHECK_CLOSE(applyDeadzone(32767, 0.2f), 1.f, 0.5f);
    BOOST_CHECK_CLOSE(applyDeadzone(-32767, 0.2f), -1.f, 0.5f);
    // Half deflection past the dead-zone is a fraction with the right sign
    Sint16 half = static_cast<Sint16>(0.5f * 32767);
    float v = applyDeadzone(half, 0.2f);
    BOOST_CHECK_GT(v, 0.f);
    BOOST_CHECK_LT(v, 1.f);
    BOOST_CHECK_LT(applyDeadzone(static_cast<Sint16>(-half), 0.2f), 0.f);
    // Sint16 minimum clamps to -1 (divisor is 32767, not 32768)
    BOOST_CHECK_CLOSE(applyDeadzone(-32768, 0.2f), -1.f, 0.5f);
    // A dead-zone of 1 silences the axis (no NaN at full deflection)
    BOOST_CHECK_SMALL(applyDeadzone(32767, 1.0f), 1e-5f);
    BOOST_CHECK_SMALL(applyDeadzone(-32768, 1.0f), 1e-5f);
}

BOOST_AUTO_TEST_CASE(TestControllerButton) {
    GameInputState state;

    SDL_Event ev;
    ev.type = SDL_CONTROLLERBUTTONDOWN;
    ev.cbutton.button = SDL_CONTROLLER_BUTTON_A;
    GameInput::updateGameInputState(&state, ev);
    // A is bound to Jump (on foot) and Handbrake (in vehicle)
    BOOST_CHECK(state.pressed(GameInputState::Jump));
    BOOST_CHECK(state.pressed(GameInputState::Handbrake));

    ev.type = SDL_CONTROLLERBUTTONUP;
    GameInput::updateGameInputState(&state, ev);
    BOOST_CHECK(!state.pressed(GameInputState::Jump));
    BOOST_CHECK(!state.pressed(GameInputState::Handbrake));
}

BOOST_AUTO_TEST_CASE(TestControllerLeftStick) {
    GameInputState state;
    SDL_Event ev;
    ev.type = SDL_CONTROLLERAXISMOTION;
    ev.caxis.axis = SDL_CONTROLLER_AXIS_LEFTX;

    ev.caxis.value = 32767;  // full right
    GameInput::updateGameInputState(&state, ev, 0.1f);
    BOOST_CHECK_GT(state.levels[GameInputState::GoRight], 0.5f);
    BOOST_CHECK_SMALL(state.levels[GameInputState::GoLeft], 1e-5f);

    ev.caxis.value = 0;  // centered -> both directions release
    GameInput::updateGameInputState(&state, ev, 0.1f);
    BOOST_CHECK_SMALL(state.levels[GameInputState::GoRight], 1e-5f);
    BOOST_CHECK_SMALL(state.levels[GameInputState::GoLeft], 1e-5f);
}

BOOST_AUTO_TEST_CASE(TestControllerTriggers) {
    GameInputState state;
    SDL_Event ev;
    ev.type = SDL_CONTROLLERAXISMOTION;

    ev.caxis.axis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    ev.caxis.value = 32767;
    GameInput::updateGameInputState(&state, ev, 0.1f);
    BOOST_CHECK_GT(state.levels[GameInputState::VehicleAccelerate], 0.5f);
    BOOST_CHECK_GT(state.levels[GameInputState::FireWeapon], 0.5f);

    ev.caxis.axis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    ev.caxis.value = 32767;
    GameInput::updateGameInputState(&state, ev, 0.1f);
    BOOST_CHECK_GT(state.levels[GameInputState::VehicleBrake], 0.5f);
    BOOST_CHECK_GT(state.levels[GameInputState::AimWeapon], 0.5f);
}

BOOST_AUTO_TEST_CASE(TestControllerRightStick) {
    GameInputState state;
    SDL_Event ev;
    ev.type = SDL_CONTROLLERAXISMOTION;

    ev.caxis.axis = SDL_CONTROLLER_AXIS_RIGHTX;
    ev.caxis.value = -32767;
    GameInput::updateGameInputState(&state, ev, 0.1f);
    BOOST_CHECK_LT(state.rightStickX, -0.5f);

    ev.caxis.axis = SDL_CONTROLLER_AXIS_RIGHTY;
    ev.caxis.value = 32767;
    GameInput::updateGameInputState(&state, ev, 0.1f);
    BOOST_CHECK_GT(state.rightStickY, 0.5f);
}

BOOST_AUTO_TEST_SUITE_END()
