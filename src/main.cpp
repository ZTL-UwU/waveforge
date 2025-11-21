#include "wforge/assets.h"
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

void entry(std::string_view level_id, int scale_config);

std::filesystem::path wf::_executable_path;
int main(int argc, char **argv) {
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

	CPPTRACE_TRY {
		wf::AssetsManager::loadAllAssets();
	}
	CPPTRACE_CATCH(const std::exception &e) {
		std::cerr << "Failed to load assets: " << e.what() << "\n";
		cpptrace::from_current_exception().print();
		return 1;
	}

	argparse::ArgumentParser program(
		"waveforge", "0.1", argparse::default_arguments::help
	);

	program.add_argument("level")
		.help("Level ID to load (without 'level/' prefix)")
		.default_value(std::string("demo"));

	program.add_argument("--scale")
		.help("Set rendering scale (0 for automatic)")
		.default_value(0)
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

void entry(std::string_view level_id, int scale_config) {
	wf::SceneManager scene_mgr(
		pro::make_proxy<wf::SceneFacade, wf::scene::MainMenu>(scale_config)
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
