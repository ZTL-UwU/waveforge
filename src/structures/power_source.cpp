#include "wforge/structures.h"

namespace wf {
namespace structure {

namespace {

PixelShape &loadPowerSourceShape() {
	static PixelShape *shape = nullptr;
	if (shape == nullptr) {
		shape = &AssetsManager::instance().getAsset<PixelShape>(
			"power-source/shape"
		);
	}
	return *shape;
}

} // namespace

PowerSource::PowerSource(int x, int y)
	: OutputElectricalStructure(x, y, loadPowerSourceShape()) {}

bool PowerSource::step(PixelWorld &world) noexcept {
	if (!PixelShapedStructure::step(world)) {
		return false;
	}

	if (!OutputElectricalStructure::step(world)) {
		return false;
	}
	return true;
}

int PowerSource::priority() const noexcept {
	return 10;
}

} // namespace structure
} // namespace wf
