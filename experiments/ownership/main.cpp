#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

int main(int argc, char ** argv)
{
  using namespace std::chrono_literals;
  using Message = std_msgs::msg::String;
  const std::string mode = argc > 1 ? argv[1] : "on";
  if (mode != "on" && mode != "off" && mode != "two" && mode != "reference") { return 2; }
  try {
    rclcpp::init(argc, argv);
    int received = 0;
    const int expected = mode == "two" ? 2 : 1;
    {
      rclcpp::NodeOptions options;
      options.use_intra_process_comms(mode != "off");
      options.enable_rosout(false).start_parameter_services(false)
        .start_parameter_event_publisher(false).use_clock_thread(false);
      auto sender = std::make_shared<rclcpp::Node>("ownership_sender", options);
      auto receiver = std::make_shared<rclcpp::Node>("ownership_receiver", options);
      auto pub = sender->create_publisher<Message>("ownership", 10);
      std::uintptr_t original = 0;
      auto callback = [&](Message::UniquePtr msg) {
        const auto address = reinterpret_cast<std::uintptr_t>(msg.get());
        ++received;
        std::cout << "received=" << msg->data << " address=" << address
                  << " same_address=" << (address == original) << std::endl;
      };
      auto sub = receiver->create_subscription<Message>("ownership", 10, callback);
      rclcpp::Subscription<Message>::SharedPtr extra;
      if (mode == "two") {
        extra = receiver->create_subscription<Message>("ownership", 10, callback);
      }
      rclcpp::executors::SingleThreadedExecutor executor;
      executor.add_node(sender);
      executor.add_node(receiver);
      const auto discovery_deadline = std::chrono::steady_clock::now() + 5s;
      while (pub->get_subscription_count() < static_cast<std::size_t>(expected) &&
        rclcpp::ok() && std::chrono::steady_clock::now() < discovery_deadline)
      {
        std::this_thread::sleep_for(20ms);
      }
      if (pub->get_subscription_count() < static_cast<std::size_t>(expected)) {
        std::cerr << "discovery timeout" << std::endl;
        rclcpp::shutdown();
        return 1;
      }
      auto msg = std::make_unique<Message>();
      msg->data = "ownership sample";
      original = reinterpret_cast<std::uintptr_t>(msg.get());
      std::cout << "original_address=" << original << " mode=" << mode << std::endl;
      if (mode == "reference") {
        pub->publish(*msg);  // Keep the original object alive for address comparison.
      } else {
        pub->publish(std::move(msg));
      }
      const auto deadline = std::chrono::steady_clock::now() + 5s;
      while (received < expected && rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
        executor.spin_some();
        std::this_thread::sleep_for(10ms);
      }
      std::cout << "received_count=" << received << std::endl;
    }
    rclcpp::shutdown();
    return received == expected ? 0 : 1;
  } catch (const std::exception & e) {
    std::cerr << "error=" << e.what() << std::endl;
    if (rclcpp::ok()) { rclcpp::shutdown(); }
    return 2;
  }
}
