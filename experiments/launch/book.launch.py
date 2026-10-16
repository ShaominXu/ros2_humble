"""Two demo components as processes, with talker exit shutting down the launch."""

from launch import LaunchDescription
from launch.actions import EmitEvent, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch_ros.actions import Node


def generate_launch_description():
    talker = Node(
        package='demo_nodes_cpp', executable='talker', namespace='book',
        name='talker', output='screen', remappings=[('chatter', 'book_chatter')])
    listener = Node(
        package='demo_nodes_cpp', executable='listener', namespace='book',
        name='listener', output='screen', remappings=[('chatter', 'book_chatter')])
    on_exit = RegisterEventHandler(OnProcessExit(
        target_action=talker,
        on_exit=[EmitEvent(event=Shutdown(reason='book talker exited'))]))
    return LaunchDescription([on_exit, talker, listener])
