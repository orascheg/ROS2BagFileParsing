# ROS2BagFileParsing
ROS2 Bag file parsing

# Getting started

1. Install ros1 noetic
2. Install ros2 foxy
3. Install rosbag2 packages

    ```bash
    sudo apt install ros-foxy-rosbag2* ros-foxy-ros2bag*
    ```
4. Build
    
    ```bash
    cd dev_cpp_pkg
    colcon build
    ```

5. Run package

    ```bash
    . install/setup.bash
    ros2 run dev_cpp_pkg dev_cpp_node .
    ```
I have seen problems when opening from different pathes. The program worked when entering the directory where the bag is and referencing with '.'
