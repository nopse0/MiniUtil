#pragma once

namespace sl_scenes {

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
