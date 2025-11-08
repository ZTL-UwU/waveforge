#ifndef WFORGE_ASSETS_H
#define WFORGE_ASSETS_H

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cstdint>
#include <map>
#include <string>

namespace wf {

struct PixelShape {
	PixelShape(const sf::Image &img) noexcept;

	int width() const noexcept {
		return _width;
	}

	int height() const noexcept {
		return _height;
	}

	bool hasPixel(int x, int y) const noexcept;

private:
	int _width;
	int _height;
	const std::uint8_t *_data; // no ownership, ~static
};

sf::Image trimImage(const sf::Image &img) noexcept;

// Cache of loaded assets
class AssetsManager {
public:
	static AssetsManager &instance() noexcept;

	static void loadAllAssets();

	template<typename T>
	T &getAsset(const std::string &id) noexcept {
		return *static_cast<T *>(getAssetRaw(id));
	}

	template<typename T>
	void cacheAsset(const std::string &id, T *asset) noexcept {
		cacheAssetRaw(id, static_cast<void *>(asset));
	}

private:
	AssetsManager() = default;

	void *getAssetRaw(const std::string &id) noexcept;
	void cacheAssetRaw(const std::string &id, void *asset) noexcept;

	std::map<std::string, void *> _asset_cache;
};

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

} // namespace wf

#endif // WFORGE_ASSETS_H
