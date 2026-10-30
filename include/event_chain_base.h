#pragma once

#include "game_context.h"
#include "fsm_state_fwd.h"

namespace event_chain {

	struct delayed {
		float _next_play_second = 0.f;
		float _next_game_day = 0.f;
		float _delay_play_seconds = 0.f;
		float _delay_game_days = 0.f;
		float _delay_retry_play_seconds = 0.f;
		bool  _is_active = false;
		inline float get_next_p() const { return _next_play_second; }
		inline float get_next_g() const { return _next_game_day; }
		inline bool is_active() const { return _is_active; }

		bool serialize(SKSE::SerializationInterface* a_serde, float a_current_time);
		int32_t deserialize(SKSE::SerializationInterface* a_serde);

		inline void set_retry_cooldown(game::context& ctx) {
			_is_active = false;
			_next_play_second = ctx._clock.get_play_second() + _delay_retry_play_seconds;
			logger_custom::debug("forced_scene_1npc set_retry_cooldown next_p={} next_g={}", get_next_p(), get_next_g());
		}

		inline void set_retry_cooldown_xl(game::context& ctx, float a_extra_large_p) {
			_is_active = false;
			_next_play_second = ctx._clock.get_play_second() + std::max(_delay_retry_play_seconds, a_extra_large_p);
			logger_custom::debug("forced_scene_1npc set_retry_cooldown_xl next_p={} next_g={}", get_next_p(), get_next_g());
		}

		inline void set_regular_cooldown(game::context& ctx) {
			_is_active = false;
			_next_play_second = ctx._clock.get_play_second() + _delay_play_seconds;
			_next_game_day = ctx._clock.get_game_day() + _delay_game_days;
			logger_custom::debug("forced_scene_1npc set_regular_cooldown next_p={} next_g={}", get_next_p(), get_next_g());
		}
	};

	
	// 1. Das primäre Template der Hilfsstruktur (wird nie direkt aufgerufen)
	template<typename Signature>
	struct CallbackBinder;

	// 2. Die Partial Specialization für Member-Function-Pointer
	template<typename R, typename ChainType, typename... Args>
	struct CallbackBinder<R(ChainType::*)(Args...)> {

		static auto bind(uint32_t id, ChainType* chain_instance) {
			using MemPtrType = R(ChainType::*)(Args...);

			// 1. Hole den rohen Member-Function-Pointer über dein resolve-Template
			MemPtrType mem_ptr = ChainType::template resolve<MemPtrType>(id);

			// 2. Erzeuge die std::function mit dynamischem Rückgabetyp und Argumenten
			std::function<R(Args...)> final_cb = nullptr;

			if (mem_ptr && chain_instance) {
				final_cb = [chain_instance, mem_ptr](Args... args) -> R {
					return (chain_instance->*mem_ptr)(args...);
					};
			}

			// Rückgabe des Paars: pair<uint32_t, std::function<R(Args...)>>
			return std::make_pair(id, final_cb);
		}
	};

	// 3. Die saubere Wrapper-Funktion, die du in deinem Code aufrufst
	template<typename Signature, typename ChainType>
	auto bind_callback(uint32_t id, ChainType* chain_instance) {
		return CallbackBinder<Signature>::bind(id, chain_instance);
	}


}
