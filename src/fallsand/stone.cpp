#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

std::size_t Stone::hash() const noexcept {
	// For now, stone is stateless, so we return a fixed hash value
	return ('S' << 24) | ('T' << 16) | ('O' << 8) | 'N';
}

PixelTag Stone::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Stone,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Stone1"),
	};
}

} // namespace element
} // namespace wf