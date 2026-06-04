#ifndef RWGAME_GAMEINPUT_HPP
#define RWGAME_GAMEINPUT_HPP

#include <SDL_events.h>

struct GameInputState;

namespace GameInput {
/// Normalize a raw SDL axis value to [-1, 1] with a linear dead-zone.
/// For trigger axes whose raw range is [0, 32767], the output is [0, 1].
/// The divisor 32767 (not 32768) means Sint16 min (-32768) clamps to -1.
/// @param deadzone Fraction of full range to suppress, in [0, 1);
///                 values >= 1 return 0 for all inputs.
float applyDeadzone(Sint16 raw, float deadzone);

void updateGameInputState(GameInputState* state, const SDL_Event& event,
                          float deadzone = 0.2f);
}

#endif
