#include <iostream>
#include <vector>
using namespace std;

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Armor.h"
#include "Serial.h"

#include <fstream>

/* global publisher */
ros::Publisher armor_loc_pub;

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "rm_challenge_camera_node");
    ros::NodeHandle node;

    armor_loc_pub = node.advertise<std_msgs::String>("tpp/armor", 1);
#if PLATFORM == MANIFOLD
    cv::VideoCapture cap(0);
#else
    cv::VideoCapture cap("/home/li/Videos/train.avi");
#endif
    if(!cap.isOpened())
    {
        ROS_INFO("camera not open");
        return -1;
    }
    cv::Mat src;
    cap.read(src);
    //cv::namedWindow("frame", 1);
    //cv::imshow("frame", src);
    //cv::waitKey(0);
    Armor armor;
    armor.setDraw(SHOW_ALL);
    armor.init(src);
    Serial serial;
    serial.init();
    while(ros::ok())
    {
        cap >> src;
        if(src.empty())
           break;
        armor.feedImage(src);
        std::stringstream ss;
        std_msgs::String armor_msg;
        ss << armor.isFound()
        << " " << armor.getTargetX()
        << " " << armor.getTargetY();
        armor_msg.data= ss.str();
        armor_loc_pub.publish(armor_msg);
        serial.sendTarget(armor.getTargetX(), armor.getTargetY(), armor.isFound());

        cv::imshow("frame", src);
        cv::waitKey(0);
        ros::spinOnce();
    }
    cap.release();
}
