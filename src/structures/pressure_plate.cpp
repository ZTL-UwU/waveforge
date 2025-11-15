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

} // namespace

PressurePlate::PressurePlate(int x, int y) noexcept
	: OutputElectricalStructure(x, y, loadPressurePlateShape()) {}

bool PressurePlate::step(PixelWorld &world) noexcept {
	if (!PixelShapedStructure::step(world)) {
		return false;
	}

	bool powered = false;
	for (const auto &[px, py] : poi) {
		auto &tag = world.tagOf(x + px, y + py);
		if (tag.pclass == PixelClass::Solid
		    || tag.pclass == PixelClass::Fluid) {
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

} // namespace structure
} // namespace wf
