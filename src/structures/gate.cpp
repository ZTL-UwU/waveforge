#include "wforge/elements.h"
#include "wforge/fallsand.h"
#include "wforge/structures.h"

namespace wf::structure {

namespace {

constexpr int gate_length = 20;
constexpr int gate_open_speed = 3; // ticks per pixel, i.e. larger is slower

PixelShape &gateShape(FacingDirection dir) noexcept {
	static PixelShape *ptr = nullptr;
	if (ptr == nullptr) {
		ptr = AssetsManager::instance()
				  .getAsset<std::array<PixelShape, 4>>("gate/shapes")
				  .data();
	}
	return ptr[static_cast<std::uint8_t>(dir)];
}

} // namespace

Gate::Gate(int x, int y, FacingDirection dir)
	: InputElectricalStructure(x, y, gateShape(dir))
	, _dir(dir)
	, _open_state(0) {
	if (poi.size() == 0) {
		throw std::runtime_error("Gate: expected at least 1 POI, got 0\n");
	}
}

void Gate::setup(PixelWorld &world) {
	PixelShapedStructure::setup(world);

	int dx = xDeltaOf(_dir);
	int dy = yDeltaOf(_dir);
	for (int i = 0; i < gate_length; ++i) {
		for (auto [bx, by] : poi) {
			int wx = x + bx + i * dx;
			int wy = y + by + i * dy;
			if (!world.inBounds(wx, wy)) {
				continue;
			}

			auto &tag = world.tagOf(wx, wy);
			if (tag.pclass == PixelClass::Solid) {
				throw std::runtime_error(
					std::format(
						"Gate setup failed: blocking pixel at ({}, {})\n", wx,
						wy
					)
				);
			}

			int old_heat = tag.heat;
			world.replacePixel(wx, wy, element::Stone::create());
			tag.heat = old_heat;
		}
	}
}

int Gate::_openProgress() const noexcept {
	return _open_state / gate_open_speed;
}

bool Gate::step(PixelWorld &world) noexcept {
	if (!PixelShapedStructure::step(world)) {
		return false;
	}

	if (!InputElectricalStructure::step(world)) {
		return false;
	}

	int old_progress = _openProgress();
	if (isPowered()) {
		if (_canMoveFurther(world, 1)) {
			_open_state += 1;
		}
	} else {
		if (_canMoveFurther(world, -1)) {
			_open_state -= 1;
		}
	}

	int new_progress = _openProgress();
	if (new_progress > old_progress) {
		_openFurther(world);
	} else if (new_progress < old_progress) {
		_closeFurther(world);
	}
	return true;
}

int Gate::priority() const noexcept {
	return 5; // must be earlier than lasers and pressure plates
}

void Gate::_openFurther(PixelWorld &world) noexcept {
	int dx = xDeltaOf(_dir);
	int dy = yDeltaOf(_dir);
	int progress = _openProgress();

	// Remove pixels at the front
	for (auto [bx, by] : poi) {
		int wx = x + bx + (gate_length - progress) * dx;
		int wy = y + by + (gate_length - progress) * dy;
		if (!world.inBounds(wx, wy)) {
			continue;
		}

		auto &tag = world.tagOf(wx, wy);
		int old_heat = tag.heat;
		world.replacePixelWithAir(wx, wy);
		tag.heat = old_heat;
	}

	// Restore pixels at the back
	for (auto [bx, by] : poi) {
		int wx = x + bx + (-progress) * dx;
		int wy = y + by + (-progress) * dy;
		if (!world.inBounds(wx, wy)) {
			continue;
		}

		auto &tag = world.tagOf(wx, wy);
		int old_heat = tag.heat;
		world.replacePixel(wx, wy, element::Stone::create());
		tag.heat = old_heat;
	}
}

void Gate::_closeFurther(PixelWorld &world) noexcept {
	int dx = xDeltaOf(_dir);
	int dy = yDeltaOf(_dir);
	int progress = _openProgress();

	// Restore pixels at the front
	for (auto [bx, by] : poi) {
		int wx = x + bx + (gate_length - progress - 1) * dx;
		int wy = y + by + (gate_length - progress - 1) * dy;
		if (!world.inBounds(wx, wy)) {
			continue;
		}

		auto &tag = world.tagOf(wx, wy);
		int old_heat = tag.heat;
		world.replacePixel(wx, wy, element::Stone::create());
		tag.heat = old_heat;
	}

	// Remove pixels at the back
	for (auto [bx, by] : poi) {
		int wx = x + bx + (-progress - 1) * dx;
		int wy = y + by + (-progress - 1) * dy;
		if (!world.inBounds(wx, wy)) {
			continue;
		}

		auto &tag = world.tagOf(wx, wy);
		int old_heat = tag.heat;
		world.replacePixelWithAir(wx, wy);
		tag.heat = old_heat;
	}
}

bool Gate::_canMoveFurther(
	const PixelWorld &world, int direction
) const noexcept {
	int next_progress = _openProgress() + direction;
	if (next_progress < 0 || next_progress >= gate_length) {
		return false;
	}

	int dx = xDeltaOf(_dir);
	int dy = yDeltaOf(_dir);
	for (auto [bx, by] : poi) {
		int wx = x + bx;
		int wy = y + by;

		if (direction < 0) {
			wx += (gate_length - next_progress) * dx;
			wy += (gate_length - next_progress) * dy;
		} else {
			wx += (-next_progress) * dx;
			wy += (-next_progress) * dy;
		}

		if (!world.inBounds(wx, wy)) {
			continue;
		}

		if (world.classOfIs(wx, wy, PixelClass::Solid)) {
			return false;
		}
	}
	return true;
}

bool Gate::_checkIntegrity(const PixelWorld &world) const noexcept {
	int dx = xDeltaOf(_dir);
	int dy = yDeltaOf(_dir);
	for (int i = 0; i < gate_length; ++i) {
		for (auto [bx, by] : poi) {
			int wx = x + bx + i * dx;
			int wy = y + by + i * dy;
			if (!world.inBounds(wx, wy)) {
				continue;
			}

			if (!world.typeOfIs(wx, wy, PixelType::Stone)) {
				return false;
			}
		}
	}
	return true;
}

} // namespace wf::structure
