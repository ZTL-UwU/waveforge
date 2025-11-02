#include "wforge/fallsand.h"
#include "wforge/render.h"
#include <SFML/Graphics.hpp>
#include <proxy/proxy.h>

int main() {
	constexpr int width = 200;
	constexpr int height = 150;
	constexpr int scale = 4; // screen pixels per world pixel

	wf::PixelWorld world(width, height);

	// create simple stone platform
	int platform_y = height - 10;
	for (int x = 10; x < width - 10; ++x) {
		world.replacePixel(
			x, platform_y,
			pro::make_proxy_inplace<wf::PixelFacade, wf::element::Stone>()
		);
	}

	// generator positions
	int sand_x = width / 2 - 8;
	int water_x = width / 2 + 8;
	int gen_y = platform_y - 50;

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
	while (window.isOpen()) {
		// SFML variant in this tree returns std::optional<Event>
		while (auto ev = window.pollEvent()) {
			if (ev->is<sf::Event::Closed>()) {
				window.close();
			}
		}

		// spawn periodically
		if ((frame % 4) == 0) {
			if (world.typeOfIs(sand_x, gen_y, wf::PixelType::Air)) {
				world.replacePixel(
					sand_x, gen_y,
					pro::make_proxy_inplace<
						wf::PixelFacade, wf::element::Sand>()
				);
			}
		}
		for (int dx = -1; dx <= 1; ++dx) {
			if (world.typeOfIs(water_x + dx, gen_y, wf::PixelType::Air)) {
				world.replacePixel(
					water_x + dx, gen_y,
					pro::make_proxy_inplace<
						wf::PixelFacade, wf::element::Water>()
				);
			}
		}

		world.step();

		renderer.uploadFromWorld(world);

		window.clear(sf::Color::Black);
		renderer.draw(window);
		window.display();

		++frame;
	}

	return 0;
}
