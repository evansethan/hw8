#ifndef MY_PROMISE_H
#  define MY_PROMISE_H
#include<memory>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<exception>
#include<stdexcept>
#include<variant>
#include<optional>
using std::shared_ptr;
using std::make_shared;
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

// overloaded helper
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// replaced enum+unique_ptr+exception_ptr with optional<variant>
template<class T>
struct SharedState {
  std::optional<std::variant<T, std::exception_ptr>> result;
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
    // wait until result is set (nullopt = empty)
    sharedState->cv.wait(lck, 
		[&] {return sharedState->result.has_value(); });

    // use visit+overloaded instead of switch on enum
    return std::visit(overloaded{
        [](T& v) -> T { return std::move(v); },
        [](exception_ptr& e) -> T { rethrow_exception(e); throw; }
        }, *sharedState->result);

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

  // assign T directly into optional
  void set_value(T const &value) {
    lock_guard lck(sharedState->mtx);
    sharedState->result = value;
    sharedState->cv.notify_one();
  }

  // assign exception_ptr into optional
  void set_exception(exception_ptr exc) {
    lock_guard lck(sharedState->mtx);
    sharedState->result = exc;
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
