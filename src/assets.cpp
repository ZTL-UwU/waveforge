#include "wforge/assets.h"
#include "wforge/colorpalette.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics/Image.hpp>
#include <array>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

#ifndef NDEBUG
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <format>
#endif

namespace fs = std::filesystem;

namespace wf {

PixelShape::PixelShape(const sf::Image &img) noexcept
	: _width(img.getSize().x)
	, _height(img.getSize().y)
	, _data(img.getPixelsPtr()) {}

PixelShape::PixelShape() noexcept: _width(0), _height(0), _data(nullptr) {}

bool PixelShape::hasPixel(int x, int y) const noexcept {
	return colorOf(x, y).a != 0;
}

sf::Color PixelShape::colorOf(int x, int y) const noexcept {
#ifndef NDEBUG
	if (x < 0 || x >= _width || y < 0 || y >= _height) {
		std::cerr << std::format(
			"PixelMap::colorOf: index out of bounds: x = {}, y = {}, width = "
			"{}, height = {}\n",
			x, y, _width, _height
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif
	// Data: size is _width * _height * 4, each pixel is 4 bytes (RGBA)
	std::uint8_t r = _data[(y * _width + x) * 4 + 0];
	std::uint8_t g = _data[(y * _width + x) * 4 + 1];
	std::uint8_t b = _data[(y * _width + x) * 4 + 2];
	std::uint8_t a = _data[(y * _width + x) * 4 + 3];
	return sf::Color(r, g, b, a);
}

bool PixelShape::isPOIPixel(int x, int y) const noexcept {
	// Almost transparent red indicates POI
	return colorOf(x, y) == colorOfName("POIMarker");
}

PixelType PixelShape::pixelTypeOf(int x, int y) const noexcept {
	auto color = colorOf(x, y).toInteger();
	switch (color) {
	case packColorByName("Air"):
	case packColorByName("POIMarker"):
		return PixelType::Air;

	case packColorByName("Stone1"):
		return PixelType::Stone;

	case packColorByName("Wood"):
		return PixelType::Wood;

	case packColorByName("Copper"):
		return PixelType::Copper;

	case packColorByName("Sand1"):
	case packColorByName("Sand2"):
		return PixelType::Sand;

	default:
		return PixelType::Decoration;
	}
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

sf::Image rotateImageTo(const sf::Image &img, FacingDirection dir) noexcept {
	auto width = img.getSize().x;
	auto height = img.getSize().y;

	unsigned int rotated_width, rotated_height;
	if (dir == FacingDirection::North || dir == FacingDirection::South) {
		rotated_width = width;
		rotated_height = height;
	} else { // East or West
		rotated_width = height;
		rotated_height = width;
	}

	sf::Image rotated({rotated_width, rotated_height}, sf::Color(0, 0, 0, 0));
	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			sf::Color color = img.getPixel({x, y});
			unsigned int rx, ry;
			switch (dir) {
			case FacingDirection::North:
				rx = x;
				ry = y;
				break;
			case FacingDirection::East:
				rx = height - 1 - y;
				ry = x;
				break;

			case FacingDirection::South:
				rx = width - 1 - x;
				ry = height - 1 - y;
				break;

			case FacingDirection::West:
				rx = y;
				ry = width - 1 - x;
				break;
			}

			rotated.setPixel({rx, ry}, color);
		}
	}
	return rotated;
}

AssetsManager &AssetsManager::instance() noexcept {
	static AssetsManager mgr;
	return mgr;
}

void *AssetsManager::_getAssetRaw(const std::string &id) noexcept {
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

void AssetsManager::_cacheAssetRaw(
	const std::string &id, void *asset
) noexcept {
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

namespace {

std::filesystem::path findAssetsRoot() {
	if (auto env_path = std::getenv("WAVEFORGE_ASSETS_PATH")) {
		fs::path res(env_path);
		fs::path manifest_path = res / "manifest.json";
		if (fs::exists(manifest_path)) {
			return fs::absolute(res);
		}

		std::cerr << std::format(
			"AssetsManager: WAVEFORGE_ASSETS_PATH is set to '{}', but file "
			"'{}' does not exist.\n",
			res.string(), manifest_path.string()
		);
#ifndef NDEBUG
		cpptrace::generate_trace().print();
#endif
		std::abort();
	}

	auto cur_path = fs::current_path();
	fs::path possible_pathes[] = {
		cur_path / "assets",
		cur_path.parent_path() / "assets",
		wf::_executable_path / "assets",
		wf::_executable_path.parent_path() / "assets",
#ifdef __linux__
		fs::path("/usr/share/waveforge/assets"),
		fs::path("/usr/local/share/waveforge/assets"),
#endif
	};

	for (const auto &res : possible_pathes) {
		fs::path manifest_path = res / "manifest.json";
		if (fs::exists(manifest_path)) {
			return fs::absolute(res);
		}
	}

	std::cerr << "AssetsManager: could not find assets root. Tried:\n";
	for (const auto &res : possible_pathes) {
		std::cerr << "  " << res << "\n";
	}
#ifndef NDEBUG
	cpptrace::generate_trace().print();
#endif
	std::abort();
}

constexpr int current_manifest_format = 1;

using operationFunc = void (*)(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
);

void fImage(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
) {
	const std::string &file = entry.at("file");
	auto file_path = assets_root / file;
	const std::string &id = entry.at("id");
	auto img = new sf::Image();
	if (!img->loadFromFile(file_path)) {
		throw std::runtime_error(
			std::format(
				"AssetsManager: failed to load image asset from file '{}'",
				file_path.string()
			)
		);
	}

	mgr.cacheAsset(id, img);
}

void fTexture(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
) {
	const std::string &input_id = entry.at("input");
	const auto &img = mgr.getAsset<sf::Image>(input_id);
	const std::string &id = entry.at("id");

	auto texture = new sf::Texture();
	if (!texture->loadFromImage(img)) {
		throw std::runtime_error(
			std::format(
				"AssetsManager: failed to create texture from image asset '{}'",
				input_id
			)
		);
	}

	mgr.cacheAsset(id, texture);
}

void fMusic(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
) {
	const std::string &file = entry.at("file");
	auto file_path = assets_root / file;
	const std::string &id = entry.at("id");

	auto music = new sf::Music();
	if (!music->openFromFile(file_path)) {
		throw std::runtime_error(
			std::format(
				"AssetsManager: failed to load music asset from file '{}'",
				file_path.string()
			)
		);
	}

	mgr.cacheAsset(id, music);
}

void fTrimImage(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
) {
	const std::string &input_id = entry.at("input");
	const auto &img = mgr.getAsset<sf::Image>(input_id);
	const std::string &id = entry.at("id");
	auto trimmed = new sf::Image(trimImage(img));
	mgr.cacheAsset(id, trimmed);
}

void fImageAllRotated(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
) {
	const std::string &input_id = entry.at("input");
	auto &img = mgr.getAsset<sf::Image>(input_id);
	const std::string &id = entry.at("id");

	auto imgs = new std::array<sf::Image, 4>;
	for (int i = 0; i < 4; ++i) {
		imgs->at(i) = rotateImageTo(
			img, static_cast<FacingDirection>(static_cast<std::uint8_t>(i))
		);
	}

	mgr.cacheAsset(id, imgs);
}

void fPixelShape(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
) {
	const std::string &input_id = entry.at("input");
	auto &img = mgr.getAsset<sf::Image>(input_id);
	const std::string &id = entry.at("id");
	auto shape = new PixelShape(img);
	mgr.cacheAsset(id, shape);
}

void fPixelShapeAllRotated(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
) {
	const std::string &input_id = entry.at("input");
	auto &img = mgr.getAsset<std::array<sf::Image, 4>>(input_id);
	const std::string &id = entry.at("id");

	auto shapes = new PixelShape[4];
	for (int i = 0; i < 4; ++i) {
		shapes[i] = PixelShape(img[i]);
	}

	mgr.cacheAsset(id, shapes);
}

void fGoalSprite(
	const nlohmann::json &entry, const fs::path &assets_root, AssetsManager &mgr
) {
	auto &img1 = mgr.getAsset<sf::Image>("goal/image_1");
	auto &img2 = mgr.getAsset<sf::Image>("goal/image_2");
	const std::string &id = entry.at("id");
	auto goal_sprite = new GoalSprite(img1, img2);
	mgr.cacheAsset(id, goal_sprite);
}

} // namespace

void AssetsManager::loadAllAssets() {
	auto assets_root = findAssetsRoot();
	std::cerr << "AssetsManager: loading assets from " << assets_root << "\n";

	std::ifstream manifest_file(assets_root / "manifest.json");
	if (!manifest_file.is_open()) {
		std::cerr << "AssetsManager: could not open manifest file\n";
		std::abort();
	}

	std::map<std::string, operationFunc> operations = {
		{"image", fImage},
		{"create-texture", fTexture},
		{"music", fMusic},
		{"trim-image", fTrimImage},
		{"create-image-of-all-facings", fImageAllRotated},
		{"calculate-shape", fPixelShape},
		{"create-pixel-shape-of-all-facings", fPixelShapeAllRotated},
		{"create-goal-sprite", fGoalSprite},
	};

#ifndef NDEBUG
	CPPTRACE_TRY {
#else
	try {
#endif
		nlohmann::json manifest = nlohmann::json::parse(manifest_file);
		AssetsManager &mgr = AssetsManager::instance();

		if (manifest.at("format").get<int>() != current_manifest_format) {
			throw std::runtime_error(
				std::format(
					"Unsupported manifest format: {}, expected {}",
					manifest.at("format").get<int>(), current_manifest_format
				)
			);
		}

		const auto &entries = manifest.at("sequence");
		for (const auto &entry : entries) {
			const std::string &op_name = entry.at("type");
			const std::string &description = entry.at("description");
			std::cerr << description << "...\n";
			auto it = operations.find(op_name);
			if (it == operations.end()) {
				throw std::runtime_error(
					std::format("Unknown asset operation type: '{}'", op_name)
				);
			}

			auto func = it->second;
			func(entry, assets_root, mgr);
		}
	}
#ifndef NDEBUG
	CPPTRACE_CATCH(const std::exception &e) {
		std::cerr << "AssetsManager: failed to parse manifest file: "
				  << e.what() << "\n";
		cpptrace::from_current_exception().print();
#else
	catch (const std::exception &e) {
		std::cerr << "AssetsManager: failed to parse manifest file: "
				  << e.what() << "\n";
#endif
		std::abort();
	}
}

} // namespace wf
