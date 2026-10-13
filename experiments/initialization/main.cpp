#include <iostream>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"

int main(int argc, char ** argv)
{
  const std::string mode = argc > 1 ? argv[1] : "global";
  auto context = std::make_shared<rclcpp::Context>();
  try {
    if (mode != "uninitialized") {
      context->init(argc, argv);
    }
    if (mode == "double-init") {
      try {
        context->init(argc, argv);
      } catch (const rclcpp::ContextAlreadyInitialized & e) {
        std::cout << "second_init_rejected=1 context_valid=" << context->is_valid()
                  << "\nexception=" << e.what() << std::endl;
      }
    }
    rclcpp::NodeOptions options;
    options.context(context);
    options.enable_rosout(false);
    options.start_parameter_services(false);
    options.start_parameter_event_publisher(false);
    options.use_clock_thread(false);
    if (mode == "local" || mode == "overrides") {
      options.arguments({"--ros-args", "-r", "__node:=local_node", "-p", "count:=20"});
    }
    if (mode == "overrides") {
      options.parameter_overrides({rclcpp::Parameter("count", 30)});
    }
    if (mode == "invalid-name") {
      options.use_global_arguments(false);
    }
    auto first = std::make_shared<rclcpp::Node>(
      mode == "invalid-name" ? "bad name" : "first", options);
    std::cout << "first=" << first->get_fully_qualified_name()
              << " has_count_before=" << first->has_parameter("count") << std::endl;
    auto first_count = first->declare_parameter<int64_t>("count", 0);
    std::cout << "first_count=" << first_count << std::endl;
    rclcpp::NodeOptions isolated;
    isolated.context(context);
    isolated.use_global_arguments(false);
    isolated.enable_rosout(false);
    isolated.start_parameter_services(false);
    isolated.start_parameter_event_publisher(false);
    isolated.use_clock_thread(false);
    auto second = std::make_shared<rclcpp::Node>("second", isolated);
    auto second_count = second->declare_parameter<int64_t>("count", 0);
    std::cout << "second=" << second->get_fully_qualified_name()
              << " second_count=" << second_count << std::endl;
    second.reset();
    first.reset();
    context->shutdown("initialization experiment completed");
    return 0;
  } catch (const std::exception & e) {
    std::cerr << "construction_or_init_failed=1 context_valid=" << context->is_valid()
              << "\nexception=" << e.what() << std::endl;
    if (context->is_valid()) {
      context->shutdown("initialization experiment failed as expected");
    }
    return 2;
  }
}
