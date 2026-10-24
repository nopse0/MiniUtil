#pragma once

#include "time_manager.h"
#include "settings.h"
#include "minigame.h"
#include "sl_scenes.h"
#include "random.h"

// cycle here, need forward declaration
namespace event_chain {
	struct manager;
}

namespace game {

	struct context {
		settings::settings& _settings;
		time_manager::clock& _clock;
		event_chain::manager& _event_chain_manager;
		minigame::struggle_game& _struggle_game;
		sl_scenes::sl_scene_starter& _scene_starter;
		random::random& _rng;
	};

}
