#ifndef MY_PROMISE_H
#  define MY_PROMISE_H
#include<memory>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<exception>
#include<stdexcept>
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;
using std::move;
using std::mutex;
using std::condition_variable;
using std::lock_guard;
using std::unique_lock;
using std::exception_ptr;
using std::rethrow_exception;
using std::runtime_error;

namespace mpcs {
template<class T> class MyPromise;

enum class State { empty, val, exc };

template<class T>
struct SharedState {
  State state{State::empty};
  unique_ptr<T> value;
  exception_ptr exception;
  mutex mtx;
  condition_variable cv;
};

template<typename T>
class MyFuture {
public:
  MyFuture(MyFuture const &) = delete; // Injected class name
  MyFuture(MyFuture &&) = default;
  // MyFuture(MyFuture &&other) : sharedState{move(other.sharedState)} {}
  T get() {
    unique_lock lck{sharedState->mtx};
    sharedState->cv.wait(lck, 
		[&] {return sharedState->state != State::empty; });
    switch (sharedState->state) {
    case State::val:
      return move(*sharedState->value);
    case State::exc:
      rethrow_exception(sharedState->exception);
    default:
      throw runtime_error{"Invalid state for future"};
    }
  }
private:
  friend class MyPromise<T>;
  MyFuture(shared_ptr<SharedState<T>> &sharedState) 
	  : sharedState(sharedState) {}
  shared_ptr<SharedState<T>> sharedState;
};

template<typename T>
class MyPromise
{
public:
  MyPromise() : sharedState{make_shared<SharedState<T>>()} {}

  void set_value(T const &value) {
    lock_guard lck(sharedState->mtx);
    sharedState->value = make_unique<T>(value);
    sharedState->state = State::val;
    sharedState->cv.notify_one();
  }

  void set_exception(exception_ptr exc) {
    lock_guard lck(sharedState->mtx);
    sharedState->exception = exc;
    sharedState->state = State::exc;
    sharedState->cv.notify_one();
  }

  MyFuture<T> get_future() {
    return sharedState;
  }
private:
  shared_ptr<SharedState<T>> sharedState; 
};
}
#endif
