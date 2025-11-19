#include "wforge/structures.h"

namespace wf {
namespace structure {

namespace {

static PixelShape &loadPressurePlateShape() noexcept {
	static PixelShape *shape = nullptr;
	if (shape == nullptr) {
		shape = &AssetsManager::instance().getAsset<PixelShape>(
			"pressure-plate/shape"
		);
	}
	return *shape;
}

static PixelShape &loadHeavyPressurePlateShape() noexcept {
	static PixelShape *shape = nullptr;
	if (shape == nullptr) {
		shape = &AssetsManager::instance().getAsset<PixelShape>(
			"heavy-pressure-plate/shape"
		);
	}
	return *shape;
}

} // namespace

PressurePlate::PressurePlate(int x, int y) noexcept
	: OutputElectricalStructure(x, y, loadPressurePlateShape()) {}

bool PressurePlate::step(PixelWorld &world) noexcept {
	if (!PixelShapedStructure::step(world)) {
		return false;
	}

	bool powered = false;
	for (const auto &[px, py] : poi) {
		auto tag = world.tagOf(x + px, y + py);
		if (tag.pclass == PixelClass::Solid
		    || tag.pclass == PixelClass::Fluid) {
			powered = true;
			break;
		}

		auto stag = world.staticTagOf(x + px, y + py);
		if (stag.external_entity_present) {
			powered = true;
			break;
		}
	}

	if (powered) {
		if (!OutputElectricalStructure::step(world)) {
			return false;
		}
	}
	return true;
}

int PressurePlate::priority() const noexcept {
	return 10;
}

HeavyPressurePlate::HeavyPressurePlate(int x, int y) noexcept
	: OutputElectricalStructure(x, y, loadHeavyPressurePlateShape()) {}

bool HeavyPressurePlate::step(PixelWorld &world) noexcept {
	if (!PixelShapedStructure::step(world)) {
		return false;
	}

	bool powered = false;
	for (const auto &[px, py] : poi) {
		auto stag = world.staticTagOf(x + px, y + py);
		if (stag.external_entity_present) {
			powered = true;
			break;
		}
	}

	if (powered) {
		if (!OutputElectricalStructure::step(world)) {
			return false;
		}
	}
	return true;
}

int HeavyPressurePlate::priority() const noexcept {
	return 10;
}

} // namespace structure
} // namespace wf
