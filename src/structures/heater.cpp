#include "wforge/fallsand.h"
#include "wforge/structures.h"

namespace wf {
namespace structure {

namespace {

PixelShape &heaterShape() {
	static PixelShape *ptr = nullptr;
	if (ptr == nullptr) {
		ptr = &AssetsManager::instance().getAsset<PixelShape>("heater/shape");
	}
	return *ptr;
}

} // namespace

Heater::Heater(int x, int y): InputElectricalStructure(x, y, heaterShape()) {}

bool Heater::step(PixelWorld &world) noexcept {
	constexpr unsigned int heat_production = 15;

	if (!PixelShapedStructure::step(world)) {
		return false;
	}

	if (!InputElectricalStructure::step(world)) {
		return false;
	}

	if (isPowered()) {
		for (auto [bx, by] : poi) {
			auto &tag = world.tagOf(x + bx, y + by);
			tag.heat = std::min(tag.heat + heat_production, PixelTag::heat_max);
		}
	}

	return true;
}

int Heater::priority() const noexcept {
	return 10;
}

} // namespace structure
} // namespace wf
