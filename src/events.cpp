//#include "time_manager.h"
#include "game_context.h"
//#include "fsm.h"
#include "events.h"
#include "state_all.h"

#if 0
namespace events {

	// Called by game thread
	void manager::game_update_settings()
	{
		if (_is_loading.load(std::memory_order_relaxed)) {
			return;
		}

		if (_ui_set_settings_updated.load(std::memory_order_acquire))
		{
			_current_settings = _ui_set_settings;
			_ui_set_settings_updated.store(false, std::memory_order_release);

			_ui_get_settings = _current_settings;
			_ui_get_settings_updated.store(true, std::memory_order_release);
		}
	}

	void manager::set_loading_state(bool loading)
	{
		_is_loading.store(loading, std::memory_order_release);
	}

	void manager::deserialize(const settings& loaded_settings)
	{

		// Disable GUI
		// _is_loading.store(loading, std::memory_order_release);

		// Instead of passing in the new settings, we could also do the SKSE deserialization right here. But set_loading_state(true) must still be called beforehand!
		// ...

		_current_settings = loaded_settings;

		// 2. Ausstehende GUI-Schreibbefehle verwerfen, die VOR dem Laden gesendet wurden (also the GUI must not change these while _is_loading==true) !!!!
		_ui_set_settings_updated.store(false, std::memory_order_release);

		_ui_get_settings = _current_settings;
		_ui_get_settings_updated.store(true, std::memory_order_release);
	}

	// With this, the GUI checks if it should be disabled (when settings are loaded from save game)
	bool manager::ui_is_loading() const
	{
		return _is_loading.load(std::memory_order_acquire);
	}


	// Called by GUI thread to set the current settings
	void manager::ui_set_settings(const settings& a_settings)
	{
		if (_is_loading.load(std::memory_order_relaxed)) {
			return;
		}

		_ui_set_settings = a_settings;
		_ui_set_settings_updated.store(true, std::memory_order_release);
	}

	// Called by GUI thread to read the current settings. Returns if the settings had been changed since the last call
	bool manager::ui_get_settings(settings& a_settings)
	{
		if (_ui_get_settings_updated.load(std::memory_order_acquire))
		{
			a_settings = _ui_get_settings;;
			_ui_get_settings_updated.store(false, std::memory_order_release);
			return true;
		}
		return false; // no change
	}

}

#endif

namespace event_chain {

	static inline bool event_less(const event_variant& lhs, const event_variant& rhs) {
		auto lhs_play = std::visit([](const auto& ev) { return ev.get_next_p(); }, lhs);
		auto rhs_play = std::visit([](const auto& ev) { return ev.get_next_p(); }, rhs);
		if (lhs_play == rhs_play) {
			auto lhs_game = std::visit([](const auto& ev) { return ev.get_next_g(); }, lhs);
			auto rhs_game = std::visit([](const auto& ev) { return ev.get_next_g(); }, rhs);
			return lhs_game < rhs_game;
		}
		else return lhs_play < rhs_play;
	}

	void manager::initialize(game::context& ctx) {
		auto nowp = ctx._clock.get_play_second();
		auto nowg = ctx._clock.get_game_day();

		auto& ed = ctx._settings._forced_scene_1npc_delays;
		auto fs = forced_scene_1npc::forced_scene_1npc { nowp + ed._cooldown_p, nowg + ed._cooldown_g, ed._cooldown_p, ed._cooldown_g, ed._retry_delay_p };

		_events.push_back(std::move(fs));
	}

	bool manager::serialize(SKSE::SerializationInterface* a_serde, game::context& ctx) {
		// 1. Anzahl der Event-Ketten speichern
		uint32_t num_events = static_cast<uint32_t>(_events.size());
		if (!a_serde->WriteRecordData(&num_events, sizeof(num_events))) {
			return false;
		}

		auto current_time = ctx._clock.get_play_second();

		// 2. Jede einzelne Kette (inklusive Typ!) über std::visit serialisieren
		for (auto& a_chain : _events) {
			uint32_t chain_type_index = static_cast<uint32_t>(a_chain.index());
			if (!a_serde->WriteRecordData(&chain_type_index, sizeof(chain_type_index))) {
				return false;
			}
			bool chain_success = std::visit([a_serde, current_time](auto& ev) -> bool {
				return ev.serialize(a_serde, current_time);
				}, a_chain);

			if (!chain_success) {
				return false;
			}
		}

		return true;
	}

	int32_t manager::deserialize(SKSE::SerializationInterface* a_serde) {
		uint32_t num_events = 0;
		uint32_t read_bytes = a_serde->ReadRecordData(&num_events, sizeof(num_events));
		if (read_bytes != sizeof(num_events))
			return -1;

		_events.clear();
		_events.reserve(num_events);

		for (uint32_t i = 0; i < num_events; ++i) {
			// Hier liest du den Typ-Index der Event-Kette aus dem Savegame
			uint32_t chain_type_index = 0;
			int32_t current_read = a_serde->ReadRecordData(&chain_type_index, sizeof(chain_type_index));
			if (current_read != sizeof(chain_type_index)) {
				return -1;
			}
			read_bytes += current_read;

			// Switch über die möglichen Event-Ketten-Typen in deiner event_variant
			switch (chain_type_index) {
			case 0: { // forced_scene_1npc
				event_chain::forced_scene_1npc::forced_scene_1npc ev;
				int32_t current_read = ev.deserialize(a_serde); // Lädt Basisklasse + spezifische Daten + stellt FSM-State wieder her!
				if (current_read < 0)
					return -1;
				read_bytes += current_read;
				_events.push_back(std::move(ev));
				break;
			}
				  // case 2: ... andere Ketten-Typen analog
			default:
				SKSE::log::error("Unbekannter Event-Ketten-Typ beim Laden!");
				return -1;
			}
		}

		return read_bytes;
	}

	event_variant* manager::get_active_event() {
		for (auto& ev : _events) {
			bool active = std::visit([](const auto& chain) { return chain.is_active(); }, ev);
			if (active) {
				return &ev; // Direkt die Adresse des Elements im Vektor zurückgeben
			}
		}
		return nullptr;
	}

	float manager::get_next_trigger_time() {
		auto it = std::min_element(_events.begin(), _events.end(), event_less);
		if (it != _events.end()) {
			return std::visit([](const auto& ev) { return ev.get_next_p(); }, *it);
		}
		else
			return NAN;
	}

	std::optional<fsm::fsm_state> manager::trigger(game::context& ctx) {
		if (get_active_event() != nullptr) {
			logger::critical("manager::trigger: trigger called, but there still is an event chain active");
			return std::nullopt;
		}
		auto it = std::min_element(_events.begin(), _events.end(), event_less);
		if (it == _events.end())
			return std::nullopt;
		auto next_p = std::visit([](const auto& ev) { return ev.get_next_p(); }, *it);
		if (next_p > ctx._clock.get_play_second())
			return std::nullopt;
		bool triggered = std::visit([&ctx](auto& ev) -> bool { return ev.trigger_phase1(ctx); }, *it);
		if (!triggered) {
			std::visit([&ctx](auto& ev) -> void { ev.set_retry_cooldown(ctx); }, *it);
			logger_custom::debug("manager::trigger !triggered, next trigger check at play time second {}", get_next_trigger_time()); // clumsy to log this, but useful for debugging
			return std::nullopt;
		}
		else {
			auto result = std::visit([&ctx](auto& ev) -> std::optional<fsm::fsm_state> { return ev.trigger_phase2(ctx); }, *it);
			logger_custom::debug("manager::trigger triggered at play time second {}, scheduled time was {}", ctx._clock.get_play_second(), get_next_trigger_time());
#ifdef __INTELLISENSE__
#pragma diag_suppress 1776  // Schaltet E1776 NUR für IntelliSense ab, E1776 ist kein Problem hier, der Compiler macht RVO, nur IntelliSense meckert 
#endif
			return result;
#ifdef __INTELLISENSE__
#pragma diag_default 1776   // Aktiviert die Prüfung für den restlichen Code wieder
#endif
		}
	}

}

