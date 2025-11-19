#include "wforge/fallsand.h"
#include "wforge/level.h"

namespace wf::item {

FireBrush::FireBrush() noexcept: BrushSizeChangableItem(3) {}

bool FireBrush::use(Level &level, int x, int y, int scale) noexcept {
	auto &world = level.fallsand;
	auto [tx, ty] = brushTopLeft(x, y, scale);
	auto size = brushSize();
	for (int dy = 0; dy < size; dy++) {
		for (int dx = 0; dx < size; dx++) {
			int wx = tx + dx;
			int wy = ty + dy;
			if (wx < 0 || wx >= world.width() || wy < 0
			    || wy >= world.height()) {
				continue;
			}

			auto &tag = world.tagOf(wx, wy);
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

} // namespace wf::item
