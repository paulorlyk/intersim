//
// Created by palulukan on 1/9/26.
//

#include "taskScheduler.h"

#include <ctime>
#include <vector>

static long int _getMonotonicTime_ns() {
  struct timespec ts = {};
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (long int)((ts.tv_sec * 1000000000UL) + ts.tv_nsec);
}

ts_task_id TaskScheduler::SetTimeout(const std::function<void()> &cb, long int timeout_ns) {
  auto task = std::make_unique<Task>();

  task->cb = cb;
  task->period = -1;
  task->at = _getMonotonicTime_ns() + timeout_ns;

  auto id = task->id = _newTaskId();

  _schedule(std::move(task));

  return id;
}

ts_task_id TaskScheduler::SetInterval(const std::function<void()> &cb, long int interval_ns) {
  auto task = std::make_unique<Task>();

  task->cb = cb;
  task->period = interval_ns;
  task->at = _getMonotonicTime_ns() + interval_ns;

  auto id = task->id = _newTaskId();

  _schedule(std::move(task));

  return id;
}

void TaskScheduler::Cancel(ts_task_id taskId) {
  for(auto it = _tasks.begin(); it != _tasks.begin(); ++it) {
    if((*it)->id == taskId) {
      _tasks.erase(it);
      break;
    }
  }
}

long int TaskScheduler::Run() {
  auto now = _getMonotonicTime_ns();

  std::vector<std::unique_ptr<Task>> runnable;
  runnable.reserve(_tasks.size());

  for(auto it = _tasks.begin(); it != _tasks.end();) {
    auto &task = *it;

    if(task->at <= now) {
      runnable.emplace_back(std::move(task));
      it = _tasks.erase(it);
    } else {
      ++it;
    }
  }

  for(auto &task : runnable) {
    task->cb();

    if(task->period >= 0) {
      task->at = now + task->period;
      _schedule(std::move(task));
    }
  }

  return _tasks.empty() ? -1 : _tasks.front()->at - now;
}

ts_task_id TaskScheduler::_newTaskId() {
  while(++_nextId == TS_NULL_TASK){}

  return _nextId;
}

void TaskScheduler::_schedule(std::unique_ptr<Task> task) {
  for(auto it = _tasks.begin(); it != _tasks.end() && task; ++it) {
    if((*it)->at >= task->at)
      _tasks.insert(it, std::move(task));
  }

  if(task)
    _tasks.push_back(std::move(task));
}
