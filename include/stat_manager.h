#pragma once

namespace stats {
	class stat_manager
	{
	public:
		static inline float get_actor_arousal(RE::Actor* a_actor) {
			if (form_cache::SLAArousalFaction) {
				return a_actor->GetFactionRank(form_cache::SLAArousalFaction, false) / 100.f;
			}
			return 0.f;
		}

		static inline float get_player_arousal(RE::Actor* a_actor) {
			if (form_cache::SLAArousalFaction) {
				return a_actor->GetFactionRank(form_cache::SLAArousalFaction, true) / 100.f;
			}
			return 0.f;
		}

	};
}