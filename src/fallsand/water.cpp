#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

PixelTag Water::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Water,
		.pclass = PixelClass::Fluid,
		.color_index = colorIndexOf("Water"),
		.thermal_conductivity = 3,
	};
}

} // namespace element
} // namespace wf
