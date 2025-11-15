#include "wforge/2d.h"
#include "wforge/assets.h"
#include "wforge/colorpalette.h"
#include "wforge/elements.h"
#include "wforge/fallsand.h"
#include "wforge/level.h"
#include "wforge/structures.h"
#include <SFML/Graphics/Image.hpp>
#include <array>
#include <format>
#include <proxy/v4/proxy.h>

namespace wf {

namespace {

constexpr sf::Color poi_marker_color = colorOfName("POIMarker");
constexpr sf::Color duck_marker_color{250, 200, 46, 231};
constexpr sf::Color checkpoint_marker_color{89, 241, 255, 231};
constexpr sf::Color laser_emitter_marker_color{51, 255, 184, 231};
constexpr sf::Color pressure_plate_marker_color{240, 34, 159, 231};

std::array<int, 2> convertBottomCenterToTopLeft(
	int x, int y, int shape_width, int shape_height
) noexcept {
	return {x - shape_width / 2, y - shape_height + 1};
}

template<typename T>
void setPositionAtBottomCenter(T &entity, int x, int y) noexcept {
	auto [top_left_x, top_left_y] = convertBottomCenterToTopLeft(
		x, y, entity.width(), entity.height()
	);
	entity.setPosition(top_left_x, top_left_y);
}

Item constructItemByName(const std::string &name) {
	using constructor_ptr = Item (*)() noexcept;
	static const std::unordered_map<std::string, constructor_ptr> constructors{
		{"water_brush", item::WaterBrush::create}
	};

	auto it = constructors.find(name);
	if (it == constructors.end()) {
		throw std::runtime_error(
			std::format("Failed to load level: unknown item '{}'", name)
		);
	}

	return (it->second)();
}

template<typename T>
StructureEntity constructStructureWithoutDirection(
	const sf::Image &img, unsigned int x, unsigned int y
) {
	return pro::make_proxy<StructureEntityFacade, T>(x, y);
}

template<typename T>
StructureEntity constructStructureWithDirection(
	const sf::Image &img, unsigned int x, unsigned int y
) {
	constexpr unsigned int dx[] = {1, 1, 0};
	constexpr unsigned int dy[] = {0, 1, 1};
	constexpr FacingDirection directions[] = {
		FacingDirection::East, FacingDirection::South, FacingDirection::West
	};

	auto dir = FacingDirection::North;
	bool dir_set = false;
	for (int i = 0; i < 3; ++i) {
		sf::Color check_color = img.getPixel({x + dx[i], y + dy[i]});
		if (check_color == poi_marker_color) {
			dir = directions[i];
			if (dir_set) {
				throw std::runtime_error(
					std::format(
						"Failed to load level: multiple direction markers for "
						"structure at ({}, {})",
						x, y
					)
				);
			}
			dir_set = true;
			break;
		}
	}

	return pro::make_proxy<StructureEntityFacade, T>(x, y, dir);
}

} // namespace

Level Level::loadFromAsset(const std::string &level_id) {
	LevelMetadata metadata = AssetsManager::instance().getAsset<LevelMetadata>(
		level_id
	);

	const auto &image = AssetsManager::instance().getAsset<sf::Image>(
		metadata.map_id
	);

	auto width = image.getSize().x;
	auto height = image.getSize().y;

	Level level(width, height);
	auto &world = level.fallsand;

	constexpr int structure_marker_alpha = 231;

	std::vector<StructureEntity> structures;

	bool duck_placed = false, checkpoint_placed = false;
	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			sf::Color color = image.getPixel({x, y});
			if (color.a != structure_marker_alpha) {
				auto ptype_color = pixelTypeFromColor(color);
				world.replacePixel(
					x, y, constructElementByType(ptype_color.type)
				);
				if (ptype_color.color_index != 255) {
					world.tagOf(x, y).color_index = ptype_color.color_index;
				}
				continue;
			}

			if (y + 1 >= height || x + 1 >= width) {
				throw std::runtime_error(
					"Failed to load level map: structure marker at border"
				);
			}

			switch (color.toInteger()) {
			case duck_marker_color.toInteger():
				if (duck_placed) {
					throw std::runtime_error(
						"Failed to load level map: multiple duck markers found"
					);
				}
				setPositionAtBottomCenter(level.duck, x, y);
				duck_placed = true;
				break;

			case checkpoint_marker_color.toInteger():
				if (checkpoint_placed) {
					throw std::runtime_error(
						"Failed to load level map: multiple checkpoint markers "
						"found"
					);
				}
				setPositionAtBottomCenter(level.checkpoint, x, y);
				checkpoint_placed = true;
				break;

			case laser_emitter_marker_color.toInteger():
				structures.push_back(
					constructStructureWithDirection<structure::LaserEmitter>(
						image, x, y
					)
				);
				break;

			case pressure_plate_marker_color.toInteger():
				structures.push_back(
					constructStructureWithoutDirection<
						structure::PressurePlate>(image, x, y)
				);
				break;

			case poi_marker_color.toInteger():
				// POI marker are for assitances of loading other structures
				break;

			default:
				throw std::runtime_error(
					std::format(
						"Failed to load level map: unknown structure marker "
						"color at ({}, {})",
						x, y
					)
				);
			}
		}
	}

	if (!duck_placed) {
		throw std::runtime_error(
			"Failed to load level map: no duck marker found"
		);
	}

	if (!checkpoint_placed) {
		throw std::runtime_error(
			"Failed to load level map: no checkpoint marker found"
		);
	}

	for (auto &s : structures) {
		world.addStructure(std::move(s));
	}

	for (const auto &[item_name, item_count] : metadata.items) {
		level.items.emplace_back(constructItemByName(item_name), item_count);
	}

	level.metadata = std::move(metadata);
	return level;
}

} // namespace wf
