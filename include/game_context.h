#pragma once

#include "time_manager.h"
#include "settings.h"

// cycle here, need forward declaration
namespace event_chain {
	struct manager;
}

namespace game {

	struct context {
		settings::settings& _settings;
		time_manager::clock& _clock;
		event_chain::manager& _event_chain_manager;
	};

}
