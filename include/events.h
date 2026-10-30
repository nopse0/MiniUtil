#pragma once

#include "fsm_state_fwd.h"
#include "forced_scene_1npc.h"

namespace fsm { class fsm; }

namespace event_chain {

	template <typename T>
	concept IsValidEventChain = requires(T ec, game::context& ctx, SKSE::SerializationInterface * a_serde, float a_current_time) {
		{ ec.trigger_phase1(ctx) } -> std::same_as<bool>;
		{ ec.get_next_p() } -> std::same_as<float>;
		{ ec.get_next_g() } -> std::same_as<float>;
		{ ec.set_retry_cooldown(ctx) } -> std::same_as<void>;
		{ ec.serialize(a_serde, a_current_time) } -> std::same_as<bool>;
	};

	using event_variant = std::variant<forced_scene_1npc::forced_scene_1npc>;

	static_assert(IsValidEventChain<forced_scene_1npc::forced_scene_1npc>);

	class manager
	{
		std::vector<event_variant> _events;
	
	public:
		void initialize(game::context& ctx);
		std::optional<fsm::fsm_state> trigger(game::context& ctx);

		bool serialize(SKSE::SerializationInterface* a_serde, game::context& ctx);
		int32_t deserialize(SKSE::SerializationInterface* a_serde);

		event_variant* get_active_event();

	private:
		float get_next_trigger_time();
	};

}
