#include "wforge/fallsand.h"
#include "wforge/render.h"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <filesystem>
#include <proxy/proxy.h>

int main() {
	// Toggle this to enable per-frame image output for debugging
	constexpr bool SAVE_FRAMES = false;
	constexpr int width = 200;
	constexpr int height = 150;
	constexpr int scale = 4; // screen pixels per world pixel

	wf::PixelWorld world(width, height);

	// create simple stone platform
	int platform_y = height - 10;
	for (int x = 10; x < width - 10; ++x) {
		world.replacePixel(
			x, platform_y,
			pro::make_proxy<wf::PixelFacade, wf::element::Stone>()
		);
	}

	// generator positions
	int sand_x = width / 2 - 8;
	int water_x = width / 2 + 8;
	int gen_y = platform_y - 40;

	// renderer
	wf::Renderer renderer(scale);
	renderer.init(width, height);

	sf::RenderWindow window(
		sf::VideoMode(
			sf::Vector2u(
				static_cast<unsigned int>(width * scale),
				static_cast<unsigned int>(height * scale)
			)
		),
		"Waveforge - fallsand demo"
	);
	window.setFramerateLimit(20);

	int frame = 0;

	if (SAVE_FRAMES) {
		// ensure output directory exists
		std::filesystem::create_directories("frames");
	}

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
		world.step();

		renderer.uploadFromWorld(world);

		window.clear(sf::Color::Black);
		renderer.draw(window);

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

		if (SAVE_FRAMES) {
			char filename[256];
			std::snprintf(
				filename, sizeof(filename), "frames/frame_%06d.png", frame
			);
			if (!renderer.saveFrame(filename)) {
				std::fprintf(
					stderr, "Failed to save frame %d to %s\n", frame, filename
				);
			}
		}

		++frame;
	}

	return 0;
}
