#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <SimpleIni.h>
#include <SKSEMenuFramework.h>
#include <spdlog/pattern_formatter.h>

using namespace std::string_view_literals;

//using namespace std;


#define MINIUTILS_MAKE_DEFAULT_LOGGER(a_func, a_level_val)                                              \
                                                                                      \
	template <class... Args>                                                          \
	struct [[maybe_unused]] a_func                                                    \
	{                                                                                 \
		a_func() = delete;                                                            \
                                                                                      \
		explicit a_func(                                                              \
			/*spdlog::level::level_enum a_level,*/                                    \
			fmt::format_string<Args...> a_fmt,                                        \
			Args&&... a_args,                                                         \
			std::source_location a_loc = std::source_location::current()) \
		{                                                                             \
			auto logger = spdlog::default_logger();                                   \
			if (logger && logger->should_log(a_level_val)) {                          \
				spdlog::log(                                                          \
					spdlog::source_loc{                                               \
						a_loc.file_name(),                                            \
						static_cast<int>(a_loc.line()),                               \
						a_loc.function_name() },                                      \
					a_level_val,                                                      \
					a_fmt,                                                            \
					std::forward<Args>(a_args)...);                                   \
			}                                                                         \
		}                                                                             \
	};                                                                                \
                                                                                      \
	template <class... Args>                                                          \
	a_func(fmt::format_string<Args...>, Args&&...) -> a_func<Args...>;

namespace logger
{
	MINIUTILS_MAKE_DEFAULT_LOGGER(info, spdlog::level::info);
	MINIUTILS_MAKE_DEFAULT_LOGGER(error, spdlog::level::err);
	MINIUTILS_MAKE_DEFAULT_LOGGER(debug, spdlog::level::debug);
	MINIUTILS_MAKE_DEFAULT_LOGGER(trace, spdlog::level::trace);
	MINIUTILS_MAKE_DEFAULT_LOGGER(warn, spdlog::level::warn);

}

#define MINIUTILS_MAKE_CUSTOM_LOGGER(a_func, a_logger_name, a_level_val)              \
                                                                                      \
	template <class... Args>                                                          \
	struct [[maybe_unused]] a_func                                                    \
	{                                                                                 \
		a_func() = delete;                                                            \
                                                                                      \
		explicit a_func(                                                              \
			fmt::format_string<Args...> a_fmt,                                        \
			Args&&... a_args,                                                         \
			std::source_location a_loc = std::source_location::current())             \
		{                                                                             \
			auto logger = spdlog::get(a_logger_name);                                 \
			if (!logger) {                                                            \
				auto def_logger = spdlog::default_logger();                           \
				if (def_logger) {													  \
					logger = std::make_shared<spdlog::logger> (                       \
						a_logger_name,                                                \
						def_logger->sinks().begin(),                                  \
						def_logger->sinks().end());									  \
					/*logger->set_formatter(std::make_unique<spdlog::pattern_formatter>("[%T.%f] [%=5t] [%n] [%L] [%s:%# %!] %v"));*/\
					spdlog::initialize_logger(logger);                                \
					/*spdlog::register_logger(logger);*/                              \
				}                                                                     \
			}                                                                         \
			if (logger && logger->should_log(a_level_val)) {                          \
				logger->log(                                                          \
					spdlog::source_loc{                                               \
						a_loc.file_name(),                                            \
						static_cast<int>(a_loc.line()),                               \
						a_loc.function_name() },                                      \
					a_level_val,                                                      \
					a_fmt,                                                            \
					std::forward<Args>(a_args)...);                                   \
			}                                                                         \
		}                                                                             \
	};                                                                                \
                                                                                      \
	template <class... Args>                                                          \
	a_func(fmt::format_string<Args...>, Args&&...) -> a_func<Args...>;


namespace event_chain {
	namespace logger_custom {
		MINIUTILS_MAKE_CUSTOM_LOGGER(error, "event_chain", spdlog::level::err);
		MINIUTILS_MAKE_CUSTOM_LOGGER(debug, "event_chain", spdlog::level::debug);
	}
}

namespace fsm {
	namespace logger_custom {
		MINIUTILS_MAKE_CUSTOM_LOGGER(trace, "fsm", spdlog::level::trace);
		MINIUTILS_MAKE_CUSTOM_LOGGER(debug, "fsm", spdlog::level::debug);
	}
}

namespace logger_test {
	//MINIUTILS_MAKE_CUSTOM_LOGGER(error, "custom", spdlog::level::err);
	//MINIUTILS_MAKE_CUSTOM_LOGGER(debug, "custom", spdlog::level::debug);
}

namespace util {
	using SKSE::stl::report_and_fail;
}

template <typename T>
class singleton {
public:
	static T& get_instance() {
		static T instance;
		return instance;
	}

	// Kopieren und Zuweisen strikt verbieten
	singleton(const singleton&) = delete;
	singleton& operator=(const singleton&) = delete;
	singleton(singleton&&) = delete;
	singleton& operator=(singleton&&) = delete;

protected:
	singleton() = default;
	virtual ~singleton() = default;
};
