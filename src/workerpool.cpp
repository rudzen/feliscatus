#include "workerpool.h"

void WorkerPool::set(const std::size_t v) {

  if (!empty())
  {
    stop();

    while (!empty())
      pop_back();
  }

  if (v > 0) {

    while (size() < v)
      emplace_back(std::make_unique<Worker>(size()));
    clear();
  }
}

void WorkerPool::stop() {

  for (auto &worker : *this)
    worker->stop();
}
