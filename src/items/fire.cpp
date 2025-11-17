#include "wforge/fallsand.h"
#include "wforge/level.h"

namespace wf {
namespace item {

FireBrush::FireBrush() noexcept: BrushSizeChangableItem(3) {}

bool FireBrush::use(Level &level, int x, int y, int scale) noexcept {
	auto &world = level.fallsand;
	auto [tx, ty] = brushTopLeft(x, y, scale);
	auto size = brushSize();
	for (int dy = 0; dy < size; dy++) {
		for (int dx = 0; dx < size; dx++) {
			auto &tag = world.tagOf(tx + dx, ty + dy);
			tag.heat = PixelTag::heat_max;
		}
	}
	return true;
}

std::string_view FireBrush::name() const noexcept {
	return "Fire";
}

Item FireBrush::create() noexcept {
	return pro::make_proxy<ItemFacade, FireBrush>();
}

} // namespace item
} // namespace wf