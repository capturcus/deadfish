#pragma once

#include <vector>
#include <string>

struct Messages {
	bool opened = false;
	bool closed = false;
	std::vector<std::string> data_msgs;
};
