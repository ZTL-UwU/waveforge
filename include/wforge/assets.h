#ifndef WFORGE_ASSETS_H
#define WFORGE_ASSETS_H

#include "wforge/2d.h"
#include "wforge/fallsand.h"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cstdint>
#include <generator>
#include <map>
#include <string>
#include <string_view>

namespace wf {

struct PixelTypeAndColor {
	PixelType type : 8;
	unsigned int color_index : 8;
};

// Determine pixel type and color index from a color
// Returns {PixelType::Decoration, 255} for not recognized colors
PixelTypeAndColor pixelTypeFromColor(const sf::Color &color) noexcept;

struct Font {
	Font(
		int char_width, int char_height, std::string_view charset,
		const sf::Image &img
	);

	int charWidth() const noexcept {
		return _char_width;
	}

	int charHeight() const noexcept {
		return _char_height;
	}

	void renderText(
		sf::RenderTarget &target, std::string_view text, sf::Color color, int x,
		int y, int scale
	) const;

	std::generator<std::array<int, 2>> textBitmap(
		std::string_view text
	) const noexcept;

	bool hasChar(char c) const noexcept;

private:
	struct CharInfo {
		int x = -1;
		int y = -1;

		bool isValid() const noexcept;
	};

	CharInfo _getCharInfo(char c) const noexcept;

	int _char_width;
	int _char_height;
	const sf::Image &_image;
	sf::Texture _texture;
	CharInfo _char_info[128]; // ASCII
};

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

protected:
	int _width;
	int _height;
	const std::uint8_t *_data; // no ownership, ~static
};

// Trim fully-transparent borders from an image
sf::Image trimImage(const sf::Image &img);

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
	T &getAsset(const std::string &id) {
		return *static_cast<T *>(_getAssetRaw(id));
	}

	// asset ownership is transferred to AssetsManager
	template<typename T>
	void cacheAsset(const std::string &id, T *asset) {
		_cacheAssetRaw(id, static_cast<void *>(asset));
	}

private:
	AssetsManager() = default;

	void *_getAssetRaw(const std::string &id);
	void _cacheAssetRaw(const std::string &id, void *asset);

	std::map<std::string, void *> _asset_cache; // owned pointers
};

// Checkpoint area sprite, which animates based on progress
struct CheckpointSprite {
	CheckpointSprite(sf::Image &checkpoint_1, sf::Image &checkpoint_2);

	int width() const noexcept;
	int height() const noexcept;

	void render(
		sf::RenderTarget &target, int x, int y, int progress, int scale
	) const;

private:
	sf::Texture _ckeckpoint_1, _checkpoint_2;
};

// defined and set at program start, used to locate assets directory
extern std::filesystem::path _executable_path;

} // namespace wf

#endif // WFORGE_ASSETS_H
