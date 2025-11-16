#include "wforge/assets.h"
#include "wforge/level.h"
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

void entry(
	std::string_view level_id, bool screenshot_mode = false,
	int forced_scale = 0
);

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

	program.add_argument("--screenshot")
		.help("Save screenshots of play session to 'screenshots/' directory")
		.default_value(false)
		.implicit_value(true);

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
		entry(
			program.get<std::string>("level"),
			program.get<bool>("--screenshot"), program.get<int>("--scale")
		);
	}
	CPPTRACE_CATCH(const std::exception &e) {
		std::cerr << "Unhandled exception: " << e.what() << "\n";
		cpptrace::from_current_exception().print();
		return 1;
	}

	return 0;
}

void entry(std::string_view level_id, bool screenshot_mode, int forced_scale) {
	auto level = wf::Level::loadFromAsset(std::format("level/{}", level_id));

	std::cerr << std::format(
		"Loaded level '{}' of size {}x{}\n", level_id, level.width(),
		level.height()
	);

	level.selectItem(0);

	auto player_screen_size = sf::VideoMode::getDesktopMode().size;
	const int scale = forced_scale
		? forced_scale
		: std::min(
			  player_screen_size.x / level.width() - 1,
			  player_screen_size.y / level.height() - 1
		  );

	wf::LevelRenderer renderer(level, scale);

	sf::Vector2u window_size(level.width() * scale, level.height() * scale);

	sf::RenderWindow window(
		sf::VideoMode(window_size), "Waveforge Demo",
		sf::Style::Titlebar | sf::Style::Close
	);
	sf::RenderTexture render_texture(window_size);

	window.setFramerateLimit(24);

	auto &background_music = wf::AssetsManager::instance().getAsset<sf::Music>(
		"music/Pixelated Paradise-X"
	);
	background_music.setLooping(true);
	background_music.play();

	if (screenshot_mode) {
		std::filesystem::create_directories("screenshots");
	}

	int frame = 0;

	while (window.isOpen()) {
		auto mouse_pos = sf::Mouse::getPosition(window);
		while (auto ev = window.pollEvent()) {
			// window closed
			if (ev->is<sf::Event::Closed>()) {
				window.close();
				background_music.stop();
				break;
			}

			if (auto mw = ev->getIf<sf::Event::MouseWheelScrolled>()) {
				if (mw->delta > 0) {
					level.changeActiveItemBrushSize(1);
				} else if (mw->delta < 0) {
					level.changeActiveItemBrushSize(-1);
				}
			}

			if (auto mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
				if (mb->button == sf::Mouse::Button::Left) {
					level.useActiveItem(mouse_pos.x, mouse_pos.y, scale);
				}
			}
		}

		level.step();

		// clear to white background
		window.clear(sf::Color::White);
		renderer.render(window, mouse_pos.x, mouse_pos.y);

		if (screenshot_mode) {
			render_texture.clear(sf::Color::White);
			renderer.render(render_texture, mouse_pos.x, mouse_pos.y);

			// save screenshot
			sf::Image screenshot = render_texture.getTexture().copyToImage();
			auto screenshot_path = std::format(
				"screenshots/{}_frame_{:05}.png", level_id, frame
			);
			if (!screenshot.saveToFile(screenshot_path)) {
				std::cerr << "Warning: failed to save screenshot to "
						  << screenshot_path << "\n";
			}
		}

		window.display();

		++frame;

		if (level.isCompleted()) {
			std::puts("Level completed! Congratulations!");
			window.close();
			background_music.stop();
			break;
		}

		if (level.isFailed()) {
			std::puts("You lost the duck! Level failed.");
			window.close();
			background_music.stop();
			break;
		}
	}
}
