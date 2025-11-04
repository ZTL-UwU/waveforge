#include "wforge/fallsand.h"

namespace wf {
namespace element {

void SolidElement::dealtPressure(
	PixelWorld &world, int x, int y, float pressure, Direction from_dir
) noexcept {
	// Reflect pressure back
	int dx = xDeltaOf(from_dir);
	int dy = yDeltaOf(from_dir);
	int side_x = x + dx;
	int side_y = y + dy;

	// No out-of-bounds check required
	world.elementOf(side_x, side_y)
		->dealtPressure(
			world, side_x, side_y, pressure, oppositeDirection(from_dir)
		);
}

} // namespace element
} // namespace wf