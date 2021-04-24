/* * @Author: Beal.MS
   * @Date: 2021-04-21 10:55:36
   * @Last Modified by:   Beal.MS
   * @Last Modified time: 2021-04-21 10:55:36
   * @Description: This is for Hardware to control exoskeleton
*/

#include <iostream>
#include "exoskeleton_control/Admittance_control.hpp"
#include <unistd.h>
#include <math.h>
#include <memory>
#include <chrono>
#include <functional>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

using std::placeholders::_1;
using namespace std::chrono_literals;

Eigen::Matrix<float,1,4> Feedback_Angle_;
Eigen::Matrix<float,1,4> Expected_Angle_;
Eigen::Matrix<float,1,4> Expected_Velocity_;
Eigen::Matrix<float,1,4> Expected_Acceleration_;
Eigen::Matrix<float,6,1> Force_;
bool flag_;

#define Joint_Subscription_Topic    "Joint_State_Accept"
#define Sensor_Subscription_Topic   "Sensor"
#define Joint_Publisher_Topic       "Joint_State_Send"

#define Joint_Thigh_Offset          -0.7046042369967704
#define JOint_Calf_Offset           -0.7046042369967704+0.4744191163880517

class Admittance_Control_Subscription :
    public rclcpp::Node,
    public Admittance_control
{
public:
    Admittance_Control_Subscription()
    : Node("Admittance_Control_Subscription"),Admittance_control()
    {
        /*Subscription Node Initialized*/
        // Subscription 1
        // Joint_Subscription = this->create_subscription<std_msgs::msg::Float64MultiArray>(
        //     Joint_Subscription_Topic,
        //     10,
        //     std::bind(&Admittance_Control_Subscription::Joint_Callback, this, _1)
        // );
        // Subscription 2
        Sensor_Subscription = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            Sensor_Subscription_Topic,
            10,
            std::bind(&Admittance_Control_Subscription::Sensor_Callback, this, _1)
        );
        /*Publish Node Initialized*/
        Joint_Publisher = this->create_publisher<std_msgs::msg::Float64MultiArray>(Joint_Publisher_Topic, 1);
    }

private:

    void Joint_Callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg)
    {
        auto position = msg->data;
        //Feedback Joint Message
        RCLCPP_INFO(this->get_logger(),
        "Left: '%f','%f','%f','%f' Right: '%f','%f','%f','%f'",
        position[0],position[1],position[2],position[3],
        position[0],position[1],position[2],position[3]);

        Feedback_Angle_ << position[0], position[1], position[2], position[3];
        Sensor_Angle_Thigh = Sensor_Angle_Thigh + 90;
        Sensor_Angle_Calf  = Sensor_Angle_Calf  + 92;
        Expected_Angle_(0,1) = Sensor_Angle_Thigh;
        Expected_Angle_(0,2) = Sensor_Angle_Thigh - Sensor_Angle_Calf;
        if(Expected_Angle_(0,2) < 0)
        {
            Expected_Angle_(0,2) = 0;
        }

        Force_ << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;

        Left_Angle = main(
            Feedback_Angle_,
            Expected_Angle_,
            Expected_Velocity_,
            Expected_Acceleration_,
            Force_
            );
        Right_Angle = main(
            Feedback_Angle_,
            Expected_Angle_,
            Expected_Velocity_,
            Expected_Acceleration_,
            Force_
            );

        auto message = std_msgs::msg::Float64MultiArray();
        // message.data = {    Left_Angle(0,0),    Left_Angle(1,0),    Left_Angle(2,0),    Left_Angle(3,0),
        //                     Right_Angle(0,0),   Right_Angle(1,0),   Right_Angle(2,0),   Right_Angle(3,0) };
        message.data = {
            Left_Angle(1,0),
            Left_Angle(2,0),
            Right_Angle(1,0),
            Right_Angle(2,0)
        };
        Joint_Publisher->publish(message);

        std::cout<<"Left: " << Left_Angle(0,0) <<
        " " << Left_Angle(1,0)*180/PI <<
        " " << Left_Angle(2,0)*180/PI <<
        " " << Left_Angle(3,0)  <<
        " ,Published!!!"<<std::endl;
        std::cout<<"Right: "<<  Right_Angle(0,0) <<
        " " << Right_Angle(1,0)*180/PI <<
        " " << Right_Angle(2,0)*180/PI <<
        " " << Right_Angle(3,0) <<
        " ,Published!!!"<<std::endl;
        usleep(30000);
    }

    void Sensor_Callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg)
    {
        auto Sensor_Data = msg->data;

        //TODO: Force
        Sensor_Angle_Thigh = Sensor_Data[3]+90;
        Sensor_Angle_Calf  = Sensor_Data[6]+90;
        Exoskeleton_Angle_Thigh = Sensor_Data[9]-90;
        Exoskeleton_Angle_Calf  = Sensor_Data[12]-90;

        Feedback_Angle_ << 0.0, Exoskeleton_Angle_Thigh/180*PI, Exoskeleton_Angle_Calf/180*PI, 0.0;

        Expected_Angle_(0,1) = Sensor_Angle_Thigh/180*PI;
        Expected_Angle_(0,2) = (Sensor_Angle_Thigh - Sensor_Angle_Calf)/180*PI;
        if(Expected_Angle_(0,2) < 0)
        {
            Expected_Angle_(0,2) = 0;
        }

        Force_ << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
        std::cout<<"Expected_Angle: "<<Expected_Angle_<<std::endl;
        std::cout<<"Feedback_Angle: "<<Feedback_Angle_<<std::endl;
        Left_Angle = main(
            Feedback_Angle_,
            Expected_Angle_,
            Expected_Velocity_,
            Expected_Acceleration_,
            Force_
            );
        Right_Angle = main(
            Feedback_Angle_,
            Expected_Angle_,
            Expected_Velocity_,
            Expected_Acceleration_,
            Force_
            );

        auto message = std_msgs::msg::Float64MultiArray();
        // message.data = {    Left_Angle(0,0),    Left_Angle(1,0),    Left_Angle(2,0),    Left_Angle(3,0),
        //                     Right_Angle(0,0),   Right_Angle(1,0),   Right_Angle(2,0),   Right_Angle(3,0) };
        if (Left_Angle(2,0)<0)
        {
            Left_Angle(2,0) = 0;
        }
        if (Right_Angle(2,0)<0)
        {
            Right_Angle(2,0)=0;
        }
        message.data = {
            Left_Angle(1,0),
            Left_Angle(2,0),
            Right_Angle(1,0),
            Right_Angle(2,0)
        };
        Joint_Publisher->publish(message);

        std::cout<<"Left: " << Left_Angle(0,0) <<
        " " << Left_Angle(1,0)*180/PI <<
        " " << Left_Angle(2,0)*180/PI <<
        " " << Left_Angle(3,0)  <<
        " ,Published!!!"<<std::endl;
        std::cout<<"Right: "<<  Right_Angle(0,0) <<
        " " << Right_Angle(1,0)*180/PI <<
        " " << Right_Angle(2,0)*180/PI <<
        " " << Right_Angle(3,0) <<
        " ,Published!!!"<<std::endl;
    }
private:
    /*Subscription Node*/
    // rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr Joint_Subscription;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr Sensor_Subscription;

    /*Publisher Node*/
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr Joint_Publisher;

    /*Data*/
    Eigen::Matrix<float,4,1> Left_Angle, Right_Angle;
    float Force;

    float Sensor_Angle_Thigh;
    float Sensor_Angle_Calf;
    float Exoskeleton_Angle_Thigh;
    float Exoskeleton_Angle_Calf;
};

int main(int argc, char * argv[])
{

    Expected_Angle_ << 0.0,0.0,0.0,0.0;
    Expected_Velocity_ << 0.0,0.0,0.0,0.0;
    Expected_Acceleration_ << 0.0,0.0,0.0,0.0;

    /*ROS Node*/

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Admittance_Control_Subscription>());
    rclcpp::shutdown();
    return 0;
}