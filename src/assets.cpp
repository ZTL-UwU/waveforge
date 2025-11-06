#include "wforge/assets.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics/Image.hpp>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <string_view>
#include <utility>

#ifndef NDEBUG
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#endif

namespace wf {

PixelShape::PixelShape(const sf::Image &img) noexcept
	: _width(img.getSize().x)
	, _height(img.getSize().y)
	, _data(img.getPixelsPtr()) {}

bool PixelShape::hasPixel(int x, int y) const noexcept {
#ifndef NDEBUG
	if (x < 0 || x >= _width || y < 0 || y >= _height) {
		std::cerr << std::format(
			"PixelMap::hasPixel: index out of bounds: x = {}, y = {}, width = "
			"{}, height = {}\n",
			x, y, _width, _height
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif
	// Data: size is _width * _height * 4, each pixel is 4 bytes (RGBA)
	// We only check if the alpha channel is not fully transparent
	return _data[(y * _width + x) * 4 + 3] != 0;
}

sf::Image trimImage(const sf::Image &img) noexcept {
	int xmin = img.getSize().x;
	int xmax = -1;
	int ymin = img.getSize().y;
	int ymax = -1;

	for (unsigned int y = 0; y < img.getSize().y; ++y) {
		for (unsigned int x = 0; x < img.getSize().x; ++x) {
			auto color = img.getPixel({x, y});
			if (color.a != 0) {
				xmin = std::min<int>(xmin, x);
				xmax = std::max<int>(xmax, x);
				ymin = std::min<int>(ymin, y);
				ymax = std::max<int>(ymax, y);
			}
		}
	}

	if (xmax < xmin || ymax < ymin) {
		// fully transparent
		return {};
	}

	unsigned int width = xmax - xmin + 1;
	unsigned int height = ymax - ymin + 1;
	sf::Image trimmed({width, height}, sf::Color(0, 0, 0, 0));
	if (!trimmed.copy(
			img, {0, 0},
			sf::IntRect(
				{xmin, ymin},
				{static_cast<int>(width), static_cast<int>(height)}
			),
			true
		)) {
#ifndef NDEBUG
		std::cerr << "trimImage: failed to copy trimmed image\n";
		cpptrace::generate_trace().print();
		std::abort();
#endif
		std::unreachable();
	}
	return trimmed;
}

AssetsManager &AssetsManager::instance() noexcept {
	static AssetsManager mgr;
	return mgr;
}

void *AssetsManager::getAssetRaw(const std::string &id) noexcept {
	auto it = _asset_cache.find(id);
	if (it == _asset_cache.end()) {
		std::cerr << "AssetsManager::getAsset: asset not found: " << id << "\n";
#ifndef NDEBUG
		cpptrace::generate_trace().print();
#endif
		std::abort();
	}

	return it->second;
}

void AssetsManager::cacheAssetRaw(const std::string &id, void *asset) noexcept {
	if (_asset_cache.find(id) != _asset_cache.end()) {
#ifndef NDEBUG
		std::cerr << "AssetsManager::cacheAsset: asset already cached: " << id
				  << "\n";
		cpptrace::generate_trace().print();
		std::abort();
#endif
		std::unreachable();
	}
	_asset_cache[id] = asset;
}

using AssetLoaderStepFunc = void (*)(AssetsManager &mgr);

namespace {

void loadDuckRawPNG(AssetsManager &mgr) {
	auto img = new sf::Image();
	if (!img->loadFromFile("assets/duck.png")) {
		throw std::runtime_error("Failed to load asset: assets/duck/raw.png");
	}
	mgr.cacheAsset("duck/raw", img);
}

void trimDuckRNG(AssetsManager &mgr) {
	auto raw_img = mgr.getAsset<sf::Image>("duck/raw");
	auto trimmed_img = new sf::Image(trimImage(*raw_img));
	mgr.cacheAsset("duck/shape", new PixelShape(*trimmed_img));
	mgr.cacheAsset("duck/image", trimmed_img);
}

void createDuckSprite(AssetsManager &mgr) {
	auto img = mgr.getAsset<sf::Image>("duck/image");
	// allocate texture on heap and keep ownership in asset manager
	auto texture = new sf::Texture();
	texture->setSmooth(false);
	if (!texture->loadFromImage(*img)) {
		delete texture;
		throw std::runtime_error("Failed to create texture for duck sprite");
	}

	// cache texture first so sprite can safely reference it
	mgr.cacheAsset("duck/texture", texture);
	mgr.cacheAsset("duck/sprite", new sf::Sprite(*texture));
}

void loadMusicPPX(AssetsManager &mgr) {
	auto music = new sf::Music("assets/Pixelated Paradise-X.mp3");
	mgr.cacheAsset("music/Pixelated Paradise-X", music);
}

} // namespace

void AssetsManager::loadAllAssets() {
	struct LoadStep {
		std::string_view description;
		AssetLoaderStepFunc func;
	};

	constexpr LoadStep steps[] = {
		{"Loading duck PNG", loadDuckRawPNG},
		{"Processing duck image and calculating bounding box", trimDuckRNG},
		{"Creating duck sprite from processed image", createDuckSprite},
		{"Loading Pixelated Paradise-X music", loadMusicPPX},
	};

	auto &mgr = AssetsManager::instance();
#ifndef NDEBUG
	CPPTRACE_TRY {
#else
	try {
#endif
		for (const auto &step : steps) {
			std::cout << step.description << "...\n";
			step.func(mgr);
		}
#ifndef NDEBUG
	}
	CPPTRACE_CATCH(const std::exception &e) {
		std::cerr << "Exception during asset loading: " << e.what() << "\n";
		cpptrace::from_current_exception().print();
		std::abort();
	}
#else
	} catch (const std::exception &e) {
		std::cerr << "Exception during asset loading: " << e.what() << "\n";
		std::abort();
	}
#endif
}

} // namespace wf
