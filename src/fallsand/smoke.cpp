#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"

namespace wf {
namespace element {

PixelTag SmokeElement::newTag() const noexcept {
	auto rand = Xoroshiro128PP::globalInstance().next() % 3;
	return PixelTag{
		.type = PixelType::Smoke,
		.pclass = PixelClass::Gas,
		.color_index = rand ? colorIndexOf("Smoke1") : colorIndexOf("Smoke2"),
		.is_free_falling = true,
		.thermal_conductivity = 5,
	};
}

} // namespace element
} // namespace wf