#include "wforge/colorpalette.h"
#include "wforge/elements.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

PixelTag Decoration::newTag() const noexcept {
	return {
		.type = PixelType::Decoration,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Ruin"),
		.thermal_conductivity = 25,
	};
}

PixelElement Decoration::create() noexcept {
	return pro::make_proxy_inplace<PixelFacade, Decoration>();
}

} // namespace element
} // namespace wf
