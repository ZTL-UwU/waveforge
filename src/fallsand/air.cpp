#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

PixelTag Air::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Air,
		.pclass = PixelClass::Gas,
		.color_index = colorIndexOf("Air"),
	};
}

} // namespace element
} // namespace wf