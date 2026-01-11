//
// Created by palulukan on 1/9/26.
//

#ifndef TASKSCHEDULER_H_E04721D98EF3432A924B2A1490A018A4
#define TASKSCHEDULER_H_E04721D98EF3432A924B2A1490A018A4

#include <list>
#include <memory>
#include <functional>

typedef unsigned long ts_task_id;

#define TS_NULL_TASK 0

#define TS_SECONDS      1000000000L
#define TS_MILLISECONDS 1000000L
#define TS_MICROSECONDS 1000L

class TaskScheduler {
  public:
    ts_task_id SetTimeout(const std::function<void()>& cb, long int timeout_ns);
    ts_task_id SetInterval(const std::function<void()>& cb, long int interval_ns);

    void Cancel(ts_task_id taskId);

    long int Run();

    size_t Tasks() const { return _tasks.size(); }

  private:
    struct Task {
      std::function<void()> cb;
      long int period;
      long int at;
      ts_task_id id;
    };

  private:
    ts_task_id _newTaskId();
    void _schedule(std::unique_ptr<Task> task);

  private:
    ts_task_id _nextId = TS_NULL_TASK + 1;
    std::list<std::unique_ptr<Task>> _tasks;
};

#endif //TASKSCHEDULER_H_E04721D98EF3432A924B2A1490A018A4
