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


	struct sl_scene_hook_names {
		RE::BSFixedString _sl_hook_postfix;
		RE::BSFixedString _sl_hook_end_name;
	};
	
	class sl_scene_starter {
	private:
		uint8_t _sl_hook_id = 0;
	
	public:
		inline sl_scene_hook_names create_hook_names() {
			if (_sl_hook_id < 255)
				_sl_hook_id++;
			else
				_sl_hook_id = 1;
			
			auto postfix = std::format("MiniMolest{:02X}", _sl_hook_id);
			return sl_scene_hook_names{ postfix, "AnimationEnd_" + postfix };
		}
	
	};



}
