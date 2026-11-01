#pragma once

namespace equip {

	class equip_event_listener : public RE::BSTEventSink<RE::TESEquipEvent>, singleton<equip_event_listener>
	{
	public:
		virtual ~equip_event_listener() = default;

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*) override;

		static void Register();
	};

}
