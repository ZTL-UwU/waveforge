#include "wforge/assets.h"
#include "wforge/level.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <proxy/proxy.h>

int main() {
	constexpr int width = 320;
	constexpr int height = 240;
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

	auto &duck_sprite = *wf::AssetsManager::instance().getAsset<sf::Sprite>(
		"duck/sprite"
	);
	duck_sprite.setScale({scale, scale});
	level.duck.position = sf::Vector2f(width / 2., platform_y - 50);

	// render pixel world
	auto pixel_buffer = std::make_unique<std::uint8_t[]>(width * height * 4);
	sf::Texture world_texture;
	if (!world_texture.resize({width, height})) {
		std::cerr << "Failed to create world texture\n";
		return EXIT_FAILURE;
	}
	world_texture.setSmooth(false);
	auto world_sprite = sf::Sprite(world_texture);
	world_sprite.setScale({scale, scale});

	sf::RenderWindow window(
		sf::VideoMode(
			sf::Vector2u(
				static_cast<unsigned int>(width * scale),
				static_cast<unsigned int>(height * scale)
			)
		),
		"Waveforge - fallsand demo"
	);
	window.setFramerateLimit(24);

	auto background_music = wf::AssetsManager::instance().getAsset<sf::Music>(
		"music/Pixelated Paradise-X"
	);
	background_music->setLooping(true);
	background_music->play();

	int frame = 0;

	// brush settings
	int brush_size = 1; // in world pixels, square brush
	const int brush_min = 1;
	const int brush_max = 64;

	bool left_down = false;
	bool right_down = false;

	auto paint_at = [&](int world_x, int world_y, wf::PixelType ptype) {
		int half = brush_size / 2;
		for (int dy = -half; dy <= half; ++dy) {
			for (int dx = -half; dx <= half; ++dx) {
				int x = world_x + dx;
				int y = world_y + dy;
				if (x < 0 || x >= world.width() || y < 0
				    || y >= world.height()) {
					continue;
				}

				switch (ptype) {
				case wf::PixelType::Sand:
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Sand>()
					);
					break;
				case wf::PixelType::Water:
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Water>()
					);
					break;
				case wf::PixelType::Stone:
					world.replacePixel(
						x, y,
						pro::make_proxy<wf::PixelFacade, wf::element::Stone>()
					);
					break;
				case wf::PixelType::Air:
					world.replacePixelWithAir(x, y);
					break;
				default:
					break;
				}
			}
		}
	};

	// current brush selection: 1=Sand, 2=Water, 3=Stone
	wf::PixelType current_brush = wf::PixelType::Sand;

	while (window.isOpen()) {
		// SFML variant in this tree returns std::optional<Event>
		while (auto ev = window.pollEvent()) {
			// window closed
			if (ev->is<sf::Event::Closed>()) {
				window.close();
				background_music->stop();
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
						// right button erases
						paint_at(wx, wy, wf::PixelType::Air);
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
						paint_at(wx, wy, wf::PixelType::Air);
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
					if (k->code == sf::Keyboard::Key::Num1) {
						current_brush = wf::PixelType::Sand;
						std::puts("Brush: Sand (1)");
					} else if (k->code == sf::Keyboard::Key::Num2) {
						current_brush = wf::PixelType::Water;
						std::puts("Brush: Water (2)");
					} else if (k->code == sf::Keyboard::Key::Num3) {
						current_brush = wf::PixelType::Stone;
						std::puts("Brush: Stone (3)");
					}
				}
			}
		}
		level.step();

		// clear to black background
		window.clear(sf::Color::White);
		world.renderToBuffer({pixel_buffer.get(), width * height * 4});
		world_texture.update(pixel_buffer.get());
		window.draw(world_sprite);

		// draw the duck sprite at rounded world position to keep pixel-perfect
		// look
		{
			sf::Vector2f wp = level.duck.position;
			int rx = std::round(wp.x);
			int ry = std::round(wp.y);
			// set integer pixel position (world -> screen)
			duck_sprite.setPosition(
				sf::Vector2f(
					static_cast<float>(rx * scale),
					static_cast<float>(ry * scale)
				)
			);
			window.draw(duck_sprite);
		}

		// draw brush outline (red) at current mouse/world position
		sf::Vector2i mpos = sf::Mouse::getPosition(window);
		int mouse_wx = mpos.x / scale;
		int mouse_wy = mpos.y / scale;
		int half = brush_size / 2;
		sf::RectangleShape brushRect(
			sf::Vector2f(
				static_cast<float>(brush_size * scale),
				static_cast<float>(brush_size * scale)
			)
		);
		brushRect.setFillColor(sf::Color::Transparent);
		brushRect.setOutlineColor(sf::Color::Red);
		brushRect.setOutlineThickness(1.0f);
		// top-left in pixels
		brushRect.setPosition(
			sf::Vector2f(
				static_cast<float>((mouse_wx - half) * scale),
				static_cast<float>((mouse_wy - half) * scale)
			)
		);
		window.draw(brushRect);
		window.display();

		++frame;
	}

	return 0;
}
