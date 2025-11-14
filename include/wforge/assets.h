#ifndef WFORGE_ASSETS_H
#define WFORGE_ASSETS_H

#include "wforge/2d.h"
#include "wforge/fallsand.h"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cstdint>
#include <map>
#include <string>

namespace wf {

// Bitmap shape of a pixel-based entity
struct PixelShape {
	PixelShape(const sf::Image &img) noexcept;
	PixelShape() noexcept;

	int width() const noexcept {
		return _width;
	}

	int height() const noexcept {
		return _height;
	}

	// not fully transparent at (x, y)
	bool hasPixel(int x, int y) const noexcept;
	sf::Color colorOf(int x, int y) const noexcept;

	bool isPOIPixel(int x, int y) const noexcept;
	PixelType pixelTypeOf(int x, int y) const noexcept;

protected:
	int _width;
	int _height;
	const std::uint8_t *_data; // no ownership, ~static
};

// Trim fully-transparent borders from an image
sf::Image trimImage(const sf::Image &img) noexcept;

// Assumes the input image is facing North
sf::Image rotateImageTo(const sf::Image &img, FacingDirection dir) noexcept;

// Cache of loaded assets, singleton
class AssetsManager {
public:
	static AssetsManager &instance() noexcept;

	// Load all assets from the `assets/` directory, respecting manifest.json
	// See assets/README.md and assets/manifest.json for details
	static void loadAllAssets();

	// throws for unrecognized asset ID
	// WARNING: no check for type correctness, always ensure T is correct!
	template<typename T>
	T &getAsset(const std::string &id) noexcept {
		return *static_cast<T *>(_getAssetRaw(id));
	}

	// asset ownership is transferred to AssetsManager
	template<typename T>
	void cacheAsset(const std::string &id, T *asset) noexcept {
		_cacheAssetRaw(id, static_cast<void *>(asset));
	}

private:
	AssetsManager() = default;

	void *_getAssetRaw(const std::string &id) noexcept;
	void _cacheAssetRaw(const std::string &id, void *asset) noexcept;

	std::map<std::string, void *> _asset_cache; // owned pointers
};

// Goal area sprite, which animates based on progress
struct GoalSprite {
	GoalSprite(sf::Image &goal_1, sf::Image &goal_2);

	int width() const noexcept;
	int height() const noexcept;

	void render(
		sf::RenderTarget &target, int x, int y, int progress, int scale
	) const noexcept;

private:
	sf::Texture _goal_1, _goal_2;
};

// defined and set at program start, used to locate assets directory
extern std::filesystem::path _executable_path;

} // namespace wf

#endif // WFORGE_ASSETS_H
