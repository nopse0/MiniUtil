#include "form_cache.h"
#include "jcontainers_wrapper.h"
#include "stat_manager.h"

using namespace slavetats_ng::jcwrapper;

namespace stats {

	float stat_manager::get_last_event_time(RE::Actor* a_actor) {
		return JFormDB::getFlt(a_actor, ".MiniUtil.LastEventTime");
	}

	void stat_manager::set_last_event_time(RE::Actor* a_actor, float a_time) {
		JFormDB::setFlt(a_actor, ".MiniUtil.LastEventTime", a_time);
	}

}
