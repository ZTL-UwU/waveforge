#include "wforge/structures.h"

namespace wf {
namespace structure {

InputElectricalStructure::InputElectricalStructure(
	int x, int y, const PixelShape &shape
) noexcept
	: PixelShapedStructure(x, y, shape), _power_cap(0) {}

bool InputElectricalStructure::isPowered() const noexcept {
	return _power_cap > 0;
}

bool InputElectricalStructure::step(PixelWorld &world) noexcept {
	if (_power_cap > 0) {
		_power_cap -= 1;
	}

	for (int sy = 0; sy < height(); ++sy) {
		for (int sx = 0; sx < width(); ++sx) {
			int wx = x + sx;
			int wy = y + sy;

			if (world.tagOf(wx, wy).electric_power > 0) {
				_power_cap = power_capacity;
				goto break2;
			}
		}
	}
break2:
	return true;
}

OutputElectricalStructure::OutputElectricalStructure(
	int x, int y, const PixelShape &shape
) noexcept
	: PixelShapedStructure(x, y, shape) {}

bool OutputElectricalStructure::step(PixelWorld &world) noexcept {
	for (int sy = 0; sy < height(); ++sy) {
		for (int sx = 0; sx < width(); ++sx) {
			if (pixelTypeOf(sx, sy) == PixelType::Air) {
				continue;
			}

			int wx = x + sx;
			int wy = y + sy;
			world.chargeElement(wx, wy);
		}
	}

	return true;
}

} // namespace structure
} // namespace wf
