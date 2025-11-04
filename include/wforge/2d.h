#ifndef WFORGE_2D_H
#define WFORGE_2D_H

#include <array>
#include <generator>

namespace wf {

enum class Direction {
	Left,
	Right,
	Up,
	Down,
};

Direction oppositeDirection(Direction dir) noexcept;
int xDeltaOf(Direction dir) noexcept;
int yDeltaOf(Direction dir) noexcept;

template<typename T>
class DirectionArray {
public:
	auto &&operator[](this auto &&self, Direction dir) noexcept {
		return std::forward<decltype(self)>(self)
			._data[static_cast<size_t>(dir)];
	}

	decltype(auto) begin(this auto &&self) noexcept {
		return std::forward<decltype(self)>(self)._data.begin();
	}

	decltype(auto) end(this auto &&self) noexcept {
		return std::forward<decltype(self)>(self)._data.end();
	}

private:
	std::array<T, 4> _data;
};

std::generator<std::array<int, 2>> tilesOnSegment(
	std::array<int, 2> start, std::array<int, 2> end
) noexcept;

std::generator<std::array<int, 2>> neighborsOf(
	std::array<int, 2> center, std::array<int, 2> size
) noexcept;

} // namespace wf

#endif // WFORGE_2D_H
