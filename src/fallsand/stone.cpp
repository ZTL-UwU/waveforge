#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

PixelTag Stone::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Stone,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Stone1"),
	};
}

} // namespace element
} // namespace wf