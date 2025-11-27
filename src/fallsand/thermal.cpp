#include "wforge/2d.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace wf {

namespace {

constexpr float heat_transfer_factor = 0.15f;
constexpr float heat_decay_factor = 0.005f;
constexpr std::uint64_t rng_max = Xoroshiro128PP::max();

// Why 4 instead of std::thread::hardware_concurrency()?
// After all threads done their work, the main thread needs to merge
// the results. If there are too many threads, the merging step becomes
// a bottleneck. 2-6 seems to be a good compromise for most systems.
// This value can be adjusted later based on profiling results.
constexpr int num_thermal_analysis_workers = 4;

// Work phase enumeration
enum class WorkPhase {
	Idle,
	HeatTransfer,
	HeatDecay
};

// Thermal analysis worker thread
class ThermalWorker {
public:
	ThermalWorker(int worker_id)
		: _worker_id(worker_id)
		, _rng(Seed::device_random())
		, _phase(WorkPhase::Idle) {
		_thread = std::jthread([this](std::stop_token stoken) {
			workerLoop(stoken);
		});
	}

	// Non-copyable and non-movable
	ThermalWorker(const ThermalWorker &) = delete;
	ThermalWorker &operator=(const ThermalWorker &) = delete;

	void startWork(
		WorkPhase phase, const PixelWorld *world, int width, int height,
		int y_start, int y_end, std::vector<int> *heat_map,
		std::vector<int> *next_heat = nullptr
	) {
		{
			std::lock_guard<std::mutex> lock(_work_mutex);
			_phase = phase;
			_world = world;
			_width = width;
			_height = height;
			_y_start = y_start;
			_y_end = y_end;
			_heat_map = heat_map;
			_next_heat = next_heat;
			_work_ready = true;
		}
		_cv.notify_one();
	}

	void waitForCompletion() {
		std::unique_lock<std::mutex> lock(_work_mutex);
		_cv_done.wait(lock, [this] {
			return !_work_ready;
		});
	}

	~ThermalWorker() noexcept {
		_thread.request_stop();
		_cv.notify_one();
		_thread.join();
	}

private:
	void workerLoop(std::stop_token stoken) {
		while (!stoken.stop_requested()) {
			try {
				std::unique_lock<std::mutex> lock(_work_mutex);
				_cv.wait(lock, [this, &stoken] {
					return _work_ready || stoken.stop_requested();
				});

				if (stoken.stop_requested()) {
					break;
				}

				WorkPhase current_phase = _phase;
				lock.unlock();

				// Execute actual work
				if (current_phase == WorkPhase::HeatTransfer) {
					doHeatTransfer();
				} else if (current_phase == WorkPhase::HeatDecay) {
					doHeatDecay();
				}

				lock.lock();
				_work_ready = false;
				lock.unlock();
				_cv_done.notify_one();
			} catch (const std::exception &e) {
				std::cerr << "Fatal error in thermal worker " << _worker_id
						  << ": " << e.what() << std::endl;
				std::abort();
			} catch (...) {
				std::cerr << "Fatal unknown error in thermal worker "
						  << _worker_id << std::endl;
				std::abort();
			}
		}
	}

	void doHeatTransfer() {
		std::array<int, 2> world_dim{_width, _height};
		for (int y = _y_start; y < _y_end; ++y) {
			for (int x = 0; x < _width; ++x) {
				auto tag = _world->tagOf(x, y);

				if (tag.heat == 0 || tag.thermal_conductivity == 0) {
					(*_heat_map)[y * _width + x] += tag.heat;
					continue;
				}

				float total_transfer_amount = 0;

				int total_thermal_conductivity = std::round(
					tag.heat
					* (PixelTag::thermal_conductivity_max
				       - tag.thermal_conductivity)
					/ heat_transfer_factor
				);
				for (auto [nx, ny] : neighbors4({x, y}, world_dim)) {
					auto ntag = _world->tagOf(nx, ny);
					total_thermal_conductivity += std::max<int>(
													  0, tag.heat - ntag.heat
												  )
						* std::min(tag.thermal_conductivity,
					               ntag.thermal_conductivity);
				}

				for (auto [nx, ny] : neighbors4({x, y}, world_dim)) {
					auto ntag = _world->tagOf(nx, ny);
					int conductivity = std::max<int>(0, tag.heat - ntag.heat)
						* std::min(tag.thermal_conductivity,
					               ntag.thermal_conductivity);

					float transfer_amount = 1.f * tag.heat * conductivity
						/ total_thermal_conductivity;

					int received_heat = std::floor(transfer_amount);
					float frac = (transfer_amount - received_heat) / 2;
					if (_rng()
					    < std::round(frac * static_cast<double>(rng_max))) {
						received_heat += 1;
					}

					total_transfer_amount += transfer_amount;
					(*_heat_map)[ny * _width + nx] += received_heat;
				}
				(*_heat_map)[y * _width + x] += tag.heat
					- std::round(total_transfer_amount);
			}
		}
	}

	void doHeatDecay() {
		for (int y = _y_start; y < _y_end; ++y) {
			for (int x = 0; x < _width; ++x) {
				int i = y * _width + x;
				float delta = (*_next_heat)[i] * heat_decay_factor;
				int nat = std::floor(delta);
				float frac = delta - nat;
				(*_next_heat)[i] -= nat;
				if (_rng() < std::round(frac * static_cast<double>(rng_max))) {
					(*_next_heat)[i] -= 1;
				}
			}
		}
	}

	int _worker_id;
	Xoroshiro128PP _rng;
	std::jthread _thread;
	std::mutex _work_mutex;
	std::condition_variable _cv;
	std::condition_variable _cv_done;
	bool _work_ready = false;

	// Work parameters
	WorkPhase _phase;
	const PixelWorld *_world = nullptr;
	int _width = 0;
	int _height = 0;
	int _y_start = 0;
	int _y_end = 0;
	std::vector<int> *_heat_map = nullptr;
	std::vector<int> *_next_heat = nullptr;
};

// Thread pool manager
class ThermalWorkerPool {
public:
	ThermalWorkerPool() {
		_worker_heat_maps.resize(num_thermal_analysis_workers);
		for (int i = 0; i < num_thermal_analysis_workers; ++i) {
			_workers.emplace_back(std::make_unique<ThermalWorker>(i));
		}
	}

	void executeHeatTransfer(const PixelWorld *world, int width, int height) {
		const int rows_per_worker = (height + num_thermal_analysis_workers - 1)
			/ num_thermal_analysis_workers;

		// Reset and resize heat maps
		for (auto &heat_map : _worker_heat_maps) {
			heat_map.assign(width * height, 0);
		}

		// Distribute tasks
		for (int i = 0; i < num_thermal_analysis_workers; ++i) {
			int y_start = i * rows_per_worker;
			int y_end = std::min(y_start + rows_per_worker, height);
			_workers[i]->startWork(
				WorkPhase::HeatTransfer, world, width, height, y_start, y_end,
				&_worker_heat_maps[i]
			);
		}

		// Wait for completion
		for (auto &worker : _workers) {
			worker->waitForCompletion();
		}
	}

	void executeHeatDecay(
		const PixelWorld *world, int width, int height,
		std::vector<int> &next_heat
	) {
		const int rows_per_worker = (height + num_thermal_analysis_workers - 1)
			/ num_thermal_analysis_workers;

		// Distribute tasks
		for (int i = 0; i < num_thermal_analysis_workers; ++i) {
			int y_start = i * rows_per_worker;
			int y_end = std::min(y_start + rows_per_worker, height);
			_workers[i]->startWork(
				WorkPhase::HeatDecay, world, width, height, y_start, y_end,
				nullptr, &next_heat
			);
		}

		// Wait for completion
		for (auto &worker : _workers) {
			worker->waitForCompletion();
		}
	}

	const std::vector<std::vector<int>> &getHeatMaps() const {
		return _worker_heat_maps;
	}

private:
	std::vector<std::unique_ptr<ThermalWorker>> _workers;

	// It seems that there is no data-race for heat maps since each worker
	// only writes to its own rows. Why not just allocate a single heat map
	// instead of having one per worker?
	// Well, having one per worker avoids false sharing and cache
	// contention, which, although consumes more memory, can significantly
	// improve performance. The memory usage is not that large anyway. Only
	// ~200KB per worker for a standard world. Closing a chrome tab frees much
	// more memory than that :)
	std::vector<std::vector<int>> _worker_heat_maps;
};

// Global thread pool instance
ThermalWorkerPool &getThermalWorkerPool() {
	static ThermalWorkerPool pool;
	return pool;
}

} // namespace

void PixelWorld::thermalAnalysisStep() noexcept {
	auto &pool = getThermalWorkerPool();
	std::vector<int> next_heat(_width * _height, 0);

	// Heat transfer - parallel computation using thread pool
	pool.executeHeatTransfer(this, _width, _height);

	// Merge results from all workers
	const auto &heat_maps = pool.getHeatMaps();
	for (int worker_id = 0; worker_id < num_thermal_analysis_workers;
	     ++worker_id) {
		for (int i = 0; i < _width * _height; ++i) {
			next_heat[i] += heat_maps[worker_id][i];
		}
	}

	// Heat decay - parallel computation using thread pool
	pool.executeHeatDecay(this, _width, _height, next_heat);

	// Apply final results
	for (int i = 0; i < _width * _height; ++i) {
		_tags[i].heat = std::clamp<int>(next_heat[i], 0, PixelTag::heat_max);
	}
}

} // namespace wf
