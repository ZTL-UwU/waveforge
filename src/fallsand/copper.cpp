#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

PixelTag Copper::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Copper,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Copper"),
		.thermal_conductivity = 60,
	};
}

} // namespace element
} // namespace wf
