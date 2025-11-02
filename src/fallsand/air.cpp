#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

std::size_t Air::hash() const noexcept {
	// Some magic number for air since air is stateless
	return ('A' << 16) | ('I' << 8) | 'R';
}

PixelTag Air::defaultTag() const noexcept {
	return PixelTag{
		.type = PixelType::Air,
		.pclass = PixelClass::Gas,
		.color_index = colorIndexOf("Air"),
	};
}

} // namespace element
} // namespace wf