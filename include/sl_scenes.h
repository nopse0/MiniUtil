#pragma once

namespace sl_scenes {

	class sl_scene : public singleton<sl_scene>
	{
		float _handle = -1.f;
		std::vector<RE::Actor*> _actors;
		std::vector<RE::Actor*> _submissive_actors;
		uint32_t _sl_thread_id = 0;
		bool _error = false;

	public:
		void revert();

		bool set_thread_id(float a_handle, uint32_t a_sl_thread_id);
		bool set_error(float a_handle);
		inline bool get_error() { return _error; }
		bool get_scene_parameters(RE::TESQuest* a_this, float a_handle);
		bool trigger_scene(std::vector<RE::Actor*> a_actors, std::vector<RE::Actor*> a_submissive_actors, const std::string& a_hook_postfix);
	};

}
