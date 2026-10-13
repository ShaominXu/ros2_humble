#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "rclcpp/rclcpp.hpp"

int main(int argc, char ** argv)
{
  using namespace std::chrono_literals;
  const std::string mode = argc > 1 ? argv[1] : "single";
  if (mode != "single" && mode != "shared" && mode != "separate" && mode != "reentrant") {
    std::cerr << "Usage: scheduling_probe single|shared|separate|reentrant" << std::endl;
    return 2;
  }
  try {
    rclcpp::init(argc, argv);
    {
      rclcpp::NodeOptions options;
      options.enable_rosout(false).start_parameter_services(false)
        .start_parameter_event_publisher(false).use_clock_thread(false);
      auto node = std::make_shared<rclcpp::Node>("scheduling_probe", options);
      auto first_group = node->create_callback_group(
        mode == "reentrant" ? rclcpp::CallbackGroupType::Reentrant :
        rclcpp::CallbackGroupType::MutuallyExclusive);
      auto second_group = mode == "separate" ? node->create_callback_group(
        rclcpp::CallbackGroupType::MutuallyExclusive) : first_group;
      std::atomic<int> active{0}, maximum{0}, calls{0};
      std::mutex output_mutex;
      const auto origin = std::chrono::steady_clock::now();
      auto log = [&](const char * name, const char * state, int n) {
        std::lock_guard<std::mutex> lock(output_mutex);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - origin).count();
        std::cout << ms << "ms " << name << " " << state << " active=" << n
                  << " thread=" << std::this_thread::get_id() << std::endl;
      };
      auto callback = [&](const char * name) {
        int current = active.fetch_add(1) + 1;
        int observed = maximum.load();
        while (observed < current && !maximum.compare_exchange_weak(observed, current)) {}
        calls.fetch_add(1);
        log(name, "begin", current);
        std::this_thread::sleep_for(300ms);
        current = active.fetch_sub(1) - 1;
        log(name, "end", current);
      };
      auto first = node->create_wall_timer(100ms, [&]() { callback("A"); }, first_group);
      auto second = node->create_wall_timer(100ms, [&]() { callback("B"); }, second_group);
      std::unique_ptr<rclcpp::Executor> executor;
      if (mode == "single") {
        executor.reset(new rclcpp::executors::SingleThreadedExecutor());
      } else {
        executor.reset(new rclcpp::executors::MultiThreadedExecutor(rclcpp::ExecutorOptions(), 2));
      }
      executor->add_node(node);
      std::thread stopper([&]() {
        std::this_thread::sleep_for(3s);
        executor->cancel();
      });
      try {
        executor->spin();
      } catch (...) {
        stopper.join();
        throw;
      }
      stopper.join();
      executor->remove_node(node);
      std::cout << "mode=" << mode << " max_active=" << maximum.load()
                << " calls=" << calls.load() << std::endl;
    }
    rclcpp::shutdown();
    return 0;
  } catch (const std::exception & e) {
    std::cerr << "error=" << e.what() << std::endl;
    if (rclcpp::ok()) { rclcpp::shutdown(); }
    return 2;
  }
}
