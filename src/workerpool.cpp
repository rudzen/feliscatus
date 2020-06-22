#include "workerpool.h"

#include <numeric>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "search.h"
#include "game.h"

namespace {

constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 5;

std::shared_ptr<spdlog::logger> wp_logger = spdlog::rotating_logger_mt("wp_logger", "logs/wp.txt", max_log_file_size, max_log_files);

}// namespace

Worker::Worker(const std::size_t index) : index_(index) {}

Worker::~Worker() {
  stop();
}

void Worker::start(Game *master) {
  std::unique_lock<std::mutex> locker(mutex_);
  wp_logger->info("Starting worker # {}", index_);
  game_ = std::make_unique<Game>();
  game_->copy(master);
  search_ = std::make_unique<Search>(game_.get(), &pawn_hash_);
  thread_ = std::jthread(&Search::run, search_.get());
}

void Worker::stop() {
  std::unique_lock<std::mutex> locker(mutex_);
  search_->stop();
  thread_.join();
}

void WorkerPool::set(const std::size_t v) {
  if (!empty())
  {
    stop();

    while (!empty())
      delete back(), pop_back();
  }

  if (v > 0)
    while (size() < v)
      emplace_back(new Worker(size()));
}

void WorkerPool::stop() {
  for (auto &worker : *this)
    worker->stop();
}

uint64_t WorkerPool::node_count() const {
  const auto accumulator = [](const uint64_t r, const Worker *w) { return r + w->search_->node_count(); };
  return std::accumulate(cbegin(), cend(), 0ull, accumulator);
}
