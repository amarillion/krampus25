#include "openLink.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>

void openLink(const std::string& url) {
	std::wstring wurl(url.begin(), url.end());
	ShellExecuteW(
		nullptr,
		L"open",
		wurl.c_str(),
		nullptr,
		nullptr,
		SW_SHOWNORMAL
	);
}
#elif __APPLE__
#include <cstdlib>

void openLink(const std::string& url) {
	std::string cmd = "open \"" + url + "\"";
	std::system(cmd.c_str());
}
#elif __EMSCRIPTEN__
#include <emscripten/emscripten.h>
void openLink(const std::string& url) {
	std::string cmd = "window.open('" + url + "', '_blank');";
	emscripten_run_script(cmd.c_str());
}
#elif __linux__
#include <cstdlib>
void openLink(const std::string& url) {
	std::string cmd = "xdg-open \"" + url + "\"";
	std::system(cmd.c_str());
}
#else
#include <iostream>
void openLink(const std::string& url) {
	// Unsupported platform
	std::cerr << "Opening links is not supported on this platform." << std::endl;
}
#endif
