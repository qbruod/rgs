#include <iostream>
#include "RGS/Application.h"

int main()
{
	std::cout << "Hello RGS!" << std::endl;

	RGS::Application app("RGS",800,600);

	app.Run();
}