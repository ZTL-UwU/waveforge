#include "wforge/assets.h"
#include "wforge/level.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <cctype>
#include <iostream>
#include <proxy/proxy.h>

#ifndef NDEBUG
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <cpptrace/from_current_macros.hpp>
#endif

void entry();

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
	wf::AssetsManager::loadAllAssets();

#ifndef NDEBUG
	CPPTRACE_TRY {
#endif
		entry();
#ifndef NDEBUG
	}
	CPPTRACE_CATCH(const std::exception &e) {
		std::cerr << "Unhandled exception: " << e.what() << "\n";
		cpptrace::from_current_exception().print();
		return 1;
	}
#endif

	return 0;
}

void entry() {
	auto level = wf::Level::loadFromAsset("level/demo");
	level.selectItem(0);

	auto player_screen_size = sf::VideoMode::getDesktopMode().size;
	const int scale = std::min(
		player_screen_size.x / level.width() - 1,
		player_screen_size.y / level.height() - 1
	);

	wf::LevelRenderer renderer(level, scale);

	sf::RenderWindow window(
		sf::VideoMode(
			sf::Vector2u(level.width() * scale, level.height() * scale)
		),
		"Waveforge Demo", sf::Style::Titlebar | sf::Style::Close
	);

	window.setFramerateLimit(24);

	auto &background_music = wf::AssetsManager::instance().getAsset<sf::Music>(
		"music/Pixelated Paradise-X"
	);
	background_music.setLooping(true);
	background_music.play();

	int frame = 0;

	const auto &font = wf::AssetsManager::instance().getAsset<wf::Font>("font");

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

		if (auto item_stack = level.activeItemStack()) {
			constexpr sf::Color text_color{255, 255, 255, 200};
			auto display_text = std::format(
				"{} ({})", item_stack->item->name(), item_stack->amount
			);
			for (auto &c : display_text) {
				c = std::toupper(c);
			}
			font.renderText(window, display_text, text_color, 50, 50, scale);
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
