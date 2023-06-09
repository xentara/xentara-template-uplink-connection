# Connection Based Data Uplink Template for Xentara
This repository contains skeleton code for a Xentara data uplink with the following features:

Connection based | Batch processing
:--------------: | :--------------:
YES              | NO

## Prerequisites

This skill template requires the Xentara development environment, as well as a Xentara licence. You can get a Xentara
licence in the [Xentara Online Shop](https://www.xentara.io/product/xentara-for-industrial-automation/).

The documentation for Xentara can be found at https://docs.xentara.io/xentara.

This skill template uses the Xentara Utility Library, as well as the Xentara Plugin Framework. The corresponding documentation can be found here:

https://docs.xentara.io/xentara-utils/  
https://docs.xentara.io/xentara-plugin/

## Build Environment

The repository include a [CMakeLists.txt](CMakeLists.txt) file for use with [CMake](https://cmake.org/), which allows you to build the source code
out of the box, as long as the Xentara development environment is installed. If you whish to use a different build system, you must generate the
necessary build configuration file yourself.

## Source Code Documentation

The source code in this repository is documented using [Doxygen](https://doxygen.nl/) comments. If you have Doxygen installed, you can
generate detailed documentation for all classes, functions etc., including a TODO list. To generate the documentation using CMake, just
build the target *docs* by executing the following command in your [build directory](https://cmake.org/cmake/help/latest/manual/cmake.1.html#generate-a-project-buildsystem):

~~~sh
cmake --build . --target docs
~~~

This will generate HTML documentation in the subdirectory *docs/html*.

## Xentara Skill Element Templates

*(See [Skill Elements](https://docs.xentara.io/xentara/xentara_skills.html#xentara_skill_elements) in the [Xentara documentation](https://docs.xentara.io/xentara/))*

### Client Template

[src/TemplateClient.hpp](src/TemplateClient.hpp)  
[src/TemplateClient.cpp](src/TemplateClient.cpp)

The client skill element template provides template code for services that may be connected and disconnected while Xentara is running.
For such service, Xentara will periodically try to reestablish communication to the service instance if it is disconnected.

The template code has the following features:

- The connection to the service instance is established during the [pre-operational stage](https://docs.xentara.io/xentara/xentara_operational_stages.html#xentara_operational_stages_pre_operational),
  and closed during the [post-operational stage](https://docs.xentara.io/xentara/xentara_operational_stages.html#xentara_operational_stages_post_operational).
- The skill element tracks an error code for the communication with the service instance. If communication breaks down, this error code is pushed
  to the transactions.
- The skill element publishes a [Xentara task](https://docs.xentara.io/xentara/xentara_element_members.html#xentara_tasks) called *reconnect*,
  that checks the connection to the service instance, and attempts to reconnect if the communication has broken down.
- The skill element publishes two [Xentara events](https://docs.xentara.io/xentara/xentara_element_members.html#xentara_events) called *connected*
  and *disconnected*, that are fired when the connection to the service instance is establed or lost.

### Transaction Template

[src/TemplateTransaction.hpp](src/TemplateTransaction.hpp)  
[src/TemplateTransaction.cpp](src/TemplateTransaction.cpp)

The transaction skill element template provides template code for transactions that send data to the service instance immediately after collecting it.

The template code has the following features:

- The skill element publishes a [Xentara task](https://docs.xentara.io/xentara/xentara_element_members.html#xentara_tasks) called *send*,
  which collect a set of records and sends them to the service instance.
- The skill element publishes [Xentara events](https://docs.xentara.io/xentara/xentara_element_members.html#xentara_events) to signal when
  a record batch was sent, or if a send error occurred.
- If a communication breakdown is detected when sending the records, the client element is notified, and all other transactions
  are set to the same error state.
- No communication with the service instance is attempted if the connection is not up.
