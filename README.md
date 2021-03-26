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
    ros2 run dev_cpp_pkg dev_cpp_node . ./gps.csv
    ```
I have seen problems when opening from different pathes with the original data recorded by an older version of ROS2.
The program worked when entering the directory where the bag is and referencing with '.'
For recordings with foxy it worked with absolute or relative pathes.
