/*
 * update_logger.cpp
 *
 *  Created on: Apr 13, 2016
 *      Author: Jorge Nicho
 */

#include <stomp_moveit/smoothers/update_logger.h>
#include <boost/filesystem.hpp>
#include <ros/console.h>
#include <pluginlib/class_list_macros.h>
#include <ros/package.h>
#include <Eigen/Core>

PLUGINLIB_EXPORT_CLASS(stomp_moveit::smoothers::UpdateLogger,stomp_moveit::smoothers::SmootherInterface);

namespace stomp_moveit
{
namespace smoothers
{

UpdateLogger::UpdateLogger():
    name_("UpdateLogger")
{

}

UpdateLogger::~UpdateLogger()
{
  // TODO Auto-generated destructor stub
}

bool UpdateLogger::initialize(moveit::core::RobotModelConstPtr robot_model_ptr,
                        const std::string& group_name,XmlRpc::XmlRpcValue& config)
{
  format_ = Eigen::IOFormat(Eigen::StreamPrecision,0," ","\n");
  group_name_ = group_name;
  return configure(config);
}

bool UpdateLogger::configure(const XmlRpc::XmlRpcValue& config)
{
  XmlRpc::XmlRpcValue c = config;
  try
  {
    filename_ = static_cast<std::string>(c["filename"]);
    directory_ = static_cast<std::string>(c["directory"]);
    package_ = static_cast<std::string>(c["package"]);
  }
  catch(XmlRpc::XmlRpcException& e)
  {
    ROS_ERROR("%s failed to find the required parameters",getName().c_str());
    return false;
  }

  return true;
}

bool UpdateLogger::setMotionPlanRequest(const planning_scene::PlanningSceneConstPtr& planning_scene,
                 const moveit_msgs::MotionPlanRequest &req,
                 const stomp_core::StompConfiguration &config,
                 moveit_msgs::MoveItErrorCodes& error_code)
{
  using namespace boost::filesystem;

  stomp_config_ = config;

  std::string full_dir_name = ros::package::getPath(package_) + "/" + directory_;
  full_file_name_ = full_dir_name + "/" + filename_;
  path dir_path(full_dir_name);

  if(!boost::filesystem::is_directory(dir_path))
  {
    // create directory
    if(!boost::filesystem::create_directory(dir_path))
    {
      ROS_ERROR("Unable to create the update logging directory in the path %s",full_dir_name.c_str());
      return false;
    }
  }

  // open file
  file_stream_.open(full_file_name_);
  if(stream_ < 0)
  {
    ROS_ERROR("Unable to create/open update log file %s",full_file_name_.c_str());
    return false;
  }

  // clear stream
  stream_.str("");

  return true;
}

bool UpdateLogger::smooth(std::size_t start_timestep,
                    std::size_t num_timesteps,
                    int iteration_number,
                    Eigen::MatrixXd& updates)
{

  stream_<<updates.format(format_)<<std::endl;
  return true;
}

void UpdateLogger::done(bool success, int total_iterations,double final_cost)
{
  // creating header
  std::string header = R"(# num_iterations: @iterations
# num_timesteps: @timesteps
# num_dimensions: @dimensions
# matrix_rows: @rows
# matrix_cols: @cols)";

  // replacing values into header
  int rows = stomp_config_.num_dimensions * total_iterations;
  int cols = stomp_config_.num_timesteps;
  header.replace(header.find("@iterations"),std::string("@iterations").length(),std::to_string(total_iterations));
  header.replace(header.find("@timesteps"),std::string("@timesteps").length(),std::to_string(stomp_config_.num_timesteps));
  header.replace(header.find("@dimensions"),std::string("@dimensions").length(),std::to_string(stomp_config_.num_dimensions));
  header.replace(header.find("@rows"),std::string("@rows").length(),std::to_string(rows));
  header.replace(header.find("@cols"),std::string("@cols").length(),std::to_string(cols));

  // writing to file stream
  file_stream_<<header<<std::endl;
  file_stream_<<stream_.str();
  file_stream_.close();

  // clear
  stream_.str("");

  ROS_INFO("Saved update log file %s, read with 'numpy.loadtxt(\"%s\")'",full_file_name_.c_str(),filename_.c_str());
}

} /* namespace smoothers */
} /* namespace stomp_moveit */
