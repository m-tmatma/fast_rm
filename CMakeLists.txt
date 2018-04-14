cmake_minimum_required(VERSION 2.6.4)

# define a variable of project name
set(project_name "fast_rm")

# set the project as the startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${project_name})

# define a project name
project (${project_name})

# define a variable SRC with file GLOB
file(GLOB SRC *.cpp)

# define sources files of an executable
add_executable(${project_name} ${SRC})
