#include "wforge/colorpalette.h"
#include "wforge/elements.h"
#include "wforge/fallsand.h"
#include <proxy/v4/proxy.h>

namespace wf::element {

PixelTag Stone::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Stone,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Stone1"),
		.thermal_conductivity = 10,
	};
}

PixelElement Stone::create() noexcept {
	return pro::make_proxy_inplace<PixelFacade, Stone>();
}

} // namespace wf::element