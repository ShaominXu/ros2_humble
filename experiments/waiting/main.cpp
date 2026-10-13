#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/wait_set.hpp"

const char * kind_name(rclcpp::WaitResultKind kind)
{
  return kind == rclcpp::WaitResultKind::Ready ? "Ready" :
    kind == rclcpp::WaitResultKind::Timeout ? "Timeout" : "Empty";
}

int main(int argc, char ** argv)
{
  using namespace std::chrono_literals;
  const std::string mode = argc > 1 ? argv[1] : "timer";
  if (mode != "timer" && mode != "timeout" && mode != "guard") { return 2; }
  try {
    rclcpp::init(argc, argv);
    {
      rclcpp::WaitSet wait_set;
      if (mode == "guard") {
        auto guard = std::make_shared<rclcpp::GuardCondition>();
        wait_set.add_guard_condition(guard);
        std::thread notifier([guard]() {
          std::this_thread::sleep_for(200ms);
          guard->trigger();
        });
        try {
          auto result = wait_set.wait(1s);
          std::cout << "wait=" << kind_name(result.kind()) << " user_callbacks=0" << std::endl;
        } catch (...) { notifier.join(); throw; }
        notifier.join();
      } else {
        int callbacks = 0;
        rclcpp::NodeOptions options;
        options.enable_rosout(false).start_parameter_services(false)
          .start_parameter_event_publisher(false).use_clock_thread(false);
        auto node = std::make_shared<rclcpp::Node>("waiting_probe", options);
        auto timer = node->create_wall_timer(500ms, [&]() { ++callbacks; });
        wait_set.add_timer(timer);
        timer->reset();
        auto result = wait_set.wait(mode == "timeout" ? 100ms : 1000ms);
        std::cout << "wait=" << kind_name(result.kind())
                  << " callbacks_before_dispatch=" << callbacks << std::endl;
        if (mode == "timer" && result.kind() == rclcpp::WaitResultKind::Ready &&
          timer->is_ready() && timer->call())
        {
          timer->execute_callback();
        }
        std::cout << "callbacks_after_dispatch=" << callbacks << std::endl;
      }
    }
    rclcpp::shutdown();
    return 0;
  } catch (const std::exception & e) {
    std::cerr << "error=" << e.what() << std::endl;
    if (rclcpp::ok()) { rclcpp::shutdown(); }
    return 2;
  }
}
