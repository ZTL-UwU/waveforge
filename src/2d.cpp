#include "wforge/2d.h"
#include <cmath>

namespace wf {

std::generator<std::array<int, 2>> tilesOnSegment(
	std::array<int, 2> start, std::array<int, 2> end
) noexcept {
	co_yield start;
	if (start == end) {
		co_return;
	}

	int xDiff = end[0] - start[0];
	int yDiff = end[1] - start[1];
	bool xDiffIsLarger = std::abs(xDiff) > std::abs(yDiff);
	int xModifier = (xDiff < 0) ? -1 : 1;
	int yModifier = (yDiff < 0) ? -1 : 1;

	int longerSideLength = std::max(std::abs(xDiff), std::abs(yDiff));
	int shorterSideLength = std::min(std::abs(xDiff), std::abs(yDiff));
	float slope = (shorterSideLength == 0 || longerSideLength == 0)
		? 0
		: 1.0f * shorterSideLength / longerSideLength;

	for (int i = 1; i <= longerSideLength; ++i) {
		int shorterSideIncrease = std::round(i * slope);
		int dx, dy;
		if (xDiffIsLarger) {
			dx = i;
			dy = shorterSideIncrease;
		} else {
			dy = i;
			dx = shorterSideIncrease;
		}

		int x = start[0] + (dx * xModifier);
		int y = start[1] + (dy * yModifier);
		co_yield {x, y};
	}
}

} // namespace wf