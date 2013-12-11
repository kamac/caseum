#ifndef cas_ERROR_H
#define cas_ERROR_H

#include <iostream>
#include <vector>

class Error {
public:
	unsigned int currLine;
	unsigned int errCounter;
	struct file {
		std::string name;
		unsigned int lines;
	};
	std::vector<file*> files;
	Error() {
		currLine = 1;
		errCounter = 0;
	}
	void Throw(const std::string &err) {
		unsigned int trueLine = currLine+1;
		std::string &filename = files[0]->name;
		for (unsigned int i = 0; i < files.size(); i++) {
			if (trueLine > files[i]->lines) {
				trueLine -= files[i]->lines;
			} else {
				filename = files[i]->name;
				break;
			}
		}
		std::cout << "[" << trueLine << "]" << filename << ": " << err << std::endl;
		errCounter++;
	}
	~Error() {
		for (unsigned int i = 0; i < files.size(); i++)
			delete files[i];
	}
};

#endif