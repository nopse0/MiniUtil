#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <SimpleIni.h>

using namespace std::string_view_literals;

//using namespace std;


#define MINIUTILS_MAKE_RUNTIME_LOGGER(a_func, a_level_val)                                              \
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
	MINIUTILS_MAKE_RUNTIME_LOGGER(info, spdlog::level::info);
	MINIUTILS_MAKE_RUNTIME_LOGGER(error, spdlog::level::err);
	MINIUTILS_MAKE_RUNTIME_LOGGER(debug, spdlog::level::debug);
	MINIUTILS_MAKE_RUNTIME_LOGGER(trace, spdlog::level::trace);
	MINIUTILS_MAKE_RUNTIME_LOGGER(warn, spdlog::level::warn);

}

// namespace logger = SKSE::log;

namespace util {
	using SKSE::stl::report_and_fail;
}

