#ifndef WFORGE_RENDER_H
#define WFORGE_RENDER_H

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace wf {

class PixelWorld;

class Renderer {
public:
	// pixelScale: how many screen pixels per world pixel
	explicit Renderer(int pixelScale = 4) noexcept;

	// initialize texture for a world of given size
	bool init(int width, int height);

	// upload pixel data from world into internal texture
	void uploadFromWorld(const PixelWorld &world) noexcept;

	// draw to the given render target (window)
	void draw(sf::RenderTarget &target) noexcept;

	// Save current texture to a file (PNG). Returns true on success.
	bool saveFrame(const std::string &path) const noexcept;

private:
	int _width{0};
	int _height{0};
	int _scale{4};

	sf::Texture _texture;
	std::unique_ptr<sf::Sprite> _sprite;
	std::vector<std::uint8_t> _pixels; // RGBA buffer: 4 * width * height
};

} // namespace wf

#endif // WFORGE_RENDER_H
