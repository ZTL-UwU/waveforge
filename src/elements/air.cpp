#include "wforge/colorpalette.h"
#include "wforge/elements.h"

namespace wf::element {

PixelTag Air::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Air,
		.pclass = PixelClass::Gas,
		.color_index = colorIndexOf("Air"),
		.thermal_conductivity = 5,
	};
}

PixelElement Air::create() noexcept {
	return pro::make_proxy_inplace<PixelFacade, Air>();
}

} // namespace wf::element
