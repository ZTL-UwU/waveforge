#include "wforge/assets.h"
#include "wforge/elements.h"
#include "wforge/fallsand.h"
#include "wforge/level.h"
#include "wforge/structures.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <algorithm>
#include <proxy/proxy.h>
#include <proxy/v4/proxy.h>

std::filesystem::path wf::_executable_path;
int main(int argc, char **argv) {
	if (argc > 0) {
		wf::_executable_path = std::filesystem::absolute(argv[0]);
	} else {
		// fallback: current working directory
		wf::_executable_path = std::filesystem::current_path();
	}

	constexpr int width = 280;
	constexpr int height = 210;
	constexpr int scale = 6; // screen pixels per world pixel

	wf::AssetsManager::loadAllAssets();
	wf::Level level(width, height);
	auto &world = level.fallsand;

	// create simple stone platform
	int platform_y = height - 10;
	for (int x = 10; x < width - 10; ++x) {
		world.replacePixel(
			x, platform_y,
			pro::make_proxy<wf::PixelFacade, wf::element::Stone>()
		);
	}
	level.duck.position = sf::Vector2f(width / 2., platform_y - 50);

	level.goal.x = width - level.goal.width();
	level.goal.y = 20;
	int goal_platform_y = level.goal.y + level.goal.height();
	for (int x = level.goal.x - 5; x < width; ++x) {
		world.replacePixel(
			x, goal_platform_y,
			pro::make_proxy<wf::PixelFacade, wf::element::Stone>()
		);
	}

	// Create a laser emitter and a pressure plate for testing
	auto laser_emitter = pro::make_proxy<
		wf::StructureEntityFacade, wf::structure::LaserEmitter>(
		20, platform_y - 30, wf::FacingDirection::East
	);
	world.addStructure(std::move(laser_emitter));

	auto pressure_plate = pro::make_proxy<
		wf::StructureEntityFacade, wf::structure::PressurePlate>(
		width - 45, platform_y - 30
	);
	world.addStructure(std::move(pressure_plate));

	wf::LevelRenderer renderer(level, scale);

	sf::RenderWindow window(
		sf::VideoMode(sf::Vector2u(width * scale, height * scale)),
		"Waveforge Demo Sandbox", sf::Style::Titlebar | sf::Style::Close
	);

	window.setFramerateLimit(24);

	auto &background_music = wf::AssetsManager::instance().getAsset<sf::Music>(
		"music/Pixelated Paradise-X"
	);
	background_music.setLooping(true);
	background_music.play();

	int frame = 0;

	// brush settings
	int brush_size = 1; // in world pixels, square brush
	const int brush_min = 1;
	const int brush_max = 64;

	bool left_down = false;
	bool right_down = false;

	// pause / single-step controls: Space toggles pause, when paused 'n' steps
	// once
	bool paused = false;
	bool step_once = false;

	auto paint_at = [&](int world_x, int world_y, int brush_id) {
		int half = brush_size / 2;
		for (int dy = -half; dy <= half; ++dy) {
			for (int dx = -half; dx <= half; ++dx) {
				int x = world_x + dx;
				int y = world_y + dy;
				if (x < 0 || x >= world.width() || y < 0
				    || y >= world.height()) {
					continue;
				}

				switch (brush_id) {
				case 1: // Sand
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Sand>()
					);
					break;

				case 2: // Water
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Water>()
					);
					break;

				case 3: // Oil
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Oil>()
					);
					break;

				case 4: // Stone
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Stone>()
					);
					break;

				case 5: // Wood
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Wood>()
					);
					break;

				case 6: // Copper
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Copper>()
					);
					break;

				case 7: // Heat brush: don't change pixel, increase heat
					world.tagOf(x, y).heat = wf::PixelTag::heat_max;
					break;

				case 0: // Erase (Air)
					world.replacePixelWithAir(x, y);
					break;

				default:
					break;
				}
			}
		}
	};

	int current_brush = 1; // default to Sand

	while (window.isOpen()) {
		while (auto ev = window.pollEvent()) {
			// window closed
			if (ev->is<sf::Event::Closed>()) {
				window.close();
				background_music.stop();
				break;
			}

			// mouse button pressed
			if (ev->is<sf::Event::MouseButtonPressed>()) {
				const auto *mb = ev->getIf<sf::Event::MouseButtonPressed>();
				if (mb) {
					auto pos = mb->position;
					int wx = static_cast<int>(pos.x) / scale;
					int wy = static_cast<int>(pos.y) / scale;
					if (mb->button == sf::Mouse::Button::Left) {
						left_down = true;
						paint_at(wx, wy, current_brush);
					} else if (mb->button == sf::Mouse::Button::Right) {
						right_down = true;
						// right button erases (brush id 0)
						paint_at(wx, wy, 0);
					}
				}
			}

			// mouse button released
			if (ev->is<sf::Event::MouseButtonReleased>()) {
				const auto *mb = ev->getIf<sf::Event::MouseButtonReleased>();
				if (mb) {
					if (mb->button == sf::Mouse::Button::Left) {
						left_down = false;
					}
					if (mb->button == sf::Mouse::Button::Right) {
						right_down = false;
					}
				}
			}

			// mouse moved (support dragging)
			if (ev->is<sf::Event::MouseMoved>()) {
				const auto *mm = ev->getIf<sf::Event::MouseMoved>();
				if (mm) {
					int wx = static_cast<int>(mm->position.x) / scale;
					int wy = static_cast<int>(mm->position.y) / scale;
					if (left_down) {
						paint_at(wx, wy, current_brush);
					}
					if (right_down) {
						paint_at(wx, wy, 0);
					}
				}
			}

			// mouse wheel to adjust brush size
			if (ev->is<sf::Event::MouseWheelScrolled>()) {
				const auto *mw = ev->getIf<sf::Event::MouseWheelScrolled>();
				if (mw) {
					if (mw->delta > 0) {
						brush_size = std::min(brush_size + 1, brush_max);
					} else if (mw->delta < 0) {
						brush_size = std::max(brush_size - 1, brush_min);
					}
				}
			}
			// keyboard: number keys select brush
			if (ev->is<sf::Event::KeyPressed>()) {
				const auto *k = ev->getIf<sf::Event::KeyPressed>();
				if (k) {
					// Space: toggle pause. When paused, pressing 'N' steps one
					// frame.
					if (k->code == sf::Keyboard::Key::Space) {
						paused = !paused;
						if (paused) {
							std::puts("Simulation paused (Space)");
						} else {
							std::puts("Simulation resumed (Space)");
						}
					} else if (k->code == sf::Keyboard::Key::N) {
						if (paused) {
							step_once = true;
							std::puts("Stepped 1 frame (N)");
						}
					} else if (k->code == sf::Keyboard::Key::Num1) {
						current_brush = 1; // Sand
						std::puts("Brush: Sand (1)");
					} else if (k->code == sf::Keyboard::Key::Num2) {
						current_brush = 2; // Water
						std::puts("Brush: Water (2)");
					} else if (k->code == sf::Keyboard::Key::Num3) {
						current_brush = 3; // Oil
						std::puts("Brush: Oil (3)");
					} else if (k->code == sf::Keyboard::Key::Num4) {
						current_brush = 4; // Stone
						std::puts("Brush: Stone (4)");
					} else if (k->code == sf::Keyboard::Key::Num5) {
						current_brush = 5; // Wood brush
						std::puts("Brush: Wood (5)");
					} else if (k->code == sf::Keyboard::Key::Num6) {
						current_brush = 6; // Copper brush
						std::puts("Brush: Copper (6)");
					} else if (k->code == sf::Keyboard::Key::Num7) {
						current_brush = 7; // Heat brush
						std::puts("Brush: Heat (7)");
					}
				}
			}
		}
		if (!paused || step_once) {
			level.step();
			// clear single-step flag after stepping once
			step_once = false;
		}

		// clear to black background
		window.clear(sf::Color::White);
		renderer.render(window);

		// draw brush outline (red) at current mouse/world position
		sf::Vector2i mpos = sf::Mouse::getPosition(window);
		int mouse_wx = mpos.x / scale;
		int mouse_wy = mpos.y / scale;
		int half = brush_size / 2;
		float brush_outline_size = (brush_size + 1 - brush_size % 2) * scale;
		sf::RectangleShape brushRect({brush_outline_size, brush_outline_size});
		brushRect.setFillColor(sf::Color::Transparent);
		brushRect.setOutlineColor(sf::Color::Red);
		brushRect.setOutlineThickness(1.0f);
		// top-left in pixels
		brushRect.setPosition(
			sf::Vector2f((mouse_wx - half) * scale, (mouse_wy - half) * scale)
		);
		window.draw(brushRect);
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

	return 0;
}
