#pragma once

#include <memory>

class Demo {

public:
	virtual void draw() = 0;
	virtual void update() = 0;
	virtual void init() = 0;
	virtual void openLinkAt(int x, int y) = 0;
	
	static std::unique_ptr<Demo> newInstance();
};
