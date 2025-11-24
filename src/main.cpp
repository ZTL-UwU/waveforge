#include "wforge/assets.h"
#include "wforge/save.h"
#include "wforge/scene.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <argparse/argparse.hpp>
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <cpptrace/from_current_macros.hpp>
#include <iostream>
#include <proxy/proxy.h>

#ifdef _WIN32
#include <windows.h>
#endif

void entry(const std::string &level_id, int scale_config);

std::filesystem::path wf::_executable_path;
int main(int argc, char **argv) {
#if defined(_WIN32) && defined(NDEBUG)
	// On Windows, disable the console window for release builds
	{
		auto console_window = ::GetConsoleWindow();
		if (console_window) {
			::ShowWindow(console_window, SW_HIDE);
			::FreeConsole();
		}
		::SetErrorMode(
			SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX
			| SEM_NOOPENFILEERRORBOX
		);
	}
#endif

	if (argc > 0) {
		try {
			wf::_executable_path = std::filesystem::absolute(argv[0]);
		} catch (const std::exception &e) {
			std::cerr << "Warning: could not determine executable path: "
					  << e.what() << "\n";
			wf::_executable_path = std::filesystem::current_path();
		}
	} else {
		// fallback: current working directory
		wf::_executable_path = std::filesystem::current_path();
	}

#ifdef NDEBUG
	// Redirect stderr
#ifdef __linux__
	std::freopen("/tmp/waveforge-stderr.log", "w", stderr);
#elif defined(__APPLE__)
	std::freopen("/var/tmp/waveforge-stderr.log", "w", stderr);
#elif defined(_WIN32)
	std::freopen(
		(wf::_executable_path / "waveforge-stderr.log").string().c_str(), "w",
		stderr
	);
#else
	// Unsupported platform, do nothing
#endif
#endif

	CPPTRACE_TRY {
		wf::AssetsManager::loadAllAssets();
		wf::SaveData::instance(); // load / initialize save data
	}
	CPPTRACE_CATCH(const std::exception &e) {
		std::cerr << "Failed to load assets: " << e.what() << "\n";
		cpptrace::from_current_exception().print();
		return 1;
	}

	auto &save = wf::SaveData::instance();

	argparse::ArgumentParser program(
		"waveforge", WAVEFORGE_VERSION, argparse::default_arguments::help
	);

	program.add_argument("level")
		.help("Level ID to load (- for main menu)")
		.default_value("-");

	program.add_argument("--scale")
		.help("Set rendering scale (0 for automatic)")
		.default_value(save.user_settings.scale)
		.scan<'i', int>();

	try {
		program.parse_args(argc, argv);
	} catch (const std::exception &e) {
		std::cerr << e.what() << "\n";
		std::cerr << program;
		return 1;
	}

	CPPTRACE_TRY {
		entry(program.get<std::string>("level"), program.get<int>("--scale"));
	}
	CPPTRACE_CATCH(const std::exception &e) {
		std::cerr << "Unhandled exception: " << e.what() << "\n";
		cpptrace::from_current_exception().print();
		return 1;
	}

	return 0;
}

void entry(const std::string &level_id, int scale_config) {
	wf::SceneManager scene_mgr(
		level_id == "-"
			? pro::make_proxy<wf::SceneFacade, wf::scene::MainMenu>(
				  scale_config
			  )
			: pro::make_proxy<wf::SceneFacade, wf::scene::LevelPlaying>(
				  level_id, scale_config
			  )
	);

	auto &window = scene_mgr.window;

	while (window.isOpen()) {
		while (auto ev = window.pollEvent()) {
			// window closed
			if (ev->is<sf::Event::Closed>()) {
				return;
			}

			scene_mgr.handleEvent(*ev);
		}

		scene_mgr.tick();
	}
}
