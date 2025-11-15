#include "wforge/2d.h"
#include "wforge/assets.h"
#include "wforge/fallsand.h"
#include "wforge/structures.h"
#include <format>
#include <iostream>

#ifndef NDEBUG
#include <cpptrace/cpptrace.hpp>
#endif

namespace wf {
namespace structure {

namespace {

PixelShape &laserEmitterShape(FacingDirection dir) noexcept {
	static PixelShape *ptr = nullptr;
	if (ptr == nullptr) {
		ptr = AssetsManager::instance()
				  .getAsset<std::array<PixelShape, 4>>("laser-emitter/shapes")
				  .data();
	}
	return ptr[static_cast<std::uint8_t>(dir)];
}

PixelShape &laserReceiverShape(FacingDirection dir) noexcept {
	static PixelShape *ptr = nullptr;
	if (ptr == nullptr) {
		ptr = AssetsManager::instance()
				  .getAsset<std::array<PixelShape, 4>>("laser-receiver/shapes")
				  .data();
	}
	return ptr[static_cast<std::uint8_t>(dir)];
}

} // namespace

LaserEmitter::LaserEmitter(int x, int y, FacingDirection dir) noexcept
	: InputElectricalStructure(x, y, laserEmitterShape(dir)), _dir(dir) {
	if (poi.size() != 1) {
		std::cerr << std::format(
			"LaserEmitter: expected 1 POI, got {} POIs\n", poi.size()
		);
#ifndef NDEBUG
		cpptrace::generate_trace().print();
#endif
		std::abort();
	}
}

bool LaserEmitter::step(PixelWorld &world) noexcept {
	if (!PixelShapedStructure::step(world)) {
		return false;
	}

	if (!InputElectricalStructure::step(world)) {
		return false;
	}

	if (!isPowered()) {
		return true;
	}

	int dx = xDeltaOf(_dir);
	int dy = yDeltaOf(_dir);
	int poi_x = x + poi[0][0];
	int poi_y = y + poi[0][1];
	for (int cur_x = poi_x, cur_y = poi_y; cur_x >= 0 && cur_x < world.width()
	     && cur_y >= 0 && cur_y < world.height();
	     (cur_x += dx), (cur_y += dy)) {
		auto pixel_tag = world.tagOf(cur_x, cur_y);

		// Solid & smoke can block laser beam
		if (pixel_tag.pclass == PixelClass::Solid
		    || pixel_tag.type == PixelType::Smoke) {
			break;
		}

		// Activate laser
		world.staticTagOf(cur_x, cur_y).laser_active = true;
	}
	return true;
}

int LaserEmitter::priority() const noexcept {
	return 50;
}

LaserReceiver::LaserReceiver(int x, int y, FacingDirection dir) noexcept
	: OutputElectricalStructure(x, y, laserReceiverShape(dir)) {
	if (poi.size() != 1) {
		std::cerr << std::format(
			"LaserReceiver: expected 1 POI, got {} POIs\n", poi.size()
		);
#ifndef NDEBUG
		cpptrace::generate_trace().print();
#endif
		std::abort();
	}
}

bool LaserReceiver::step(PixelWorld &world) noexcept {
	if (!PixelShapedStructure::step(world)) {
		return false;
	}

	// Check for laser beam at POI
	int poi_x = x + poi[0][0];
	int poi_y = y + poi[0][1];
	auto static_tag = world.staticTagOf(poi_x, poi_y);
	if (static_tag.laser_active) {
		if (!OutputElectricalStructure::step(world)) {
			return false;
		}
	}
	return true;
}

int LaserReceiver::priority() const noexcept {
	// Must run after LaserEmitter
	return 100;
}

} // namespace structure
} // namespace wf
