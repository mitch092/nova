#include <windows.h>
#include <thread>
#include <chrono>
#include "nova.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow){
	// Check if a notepad is open. If it is, focus the window. Otherwise, do nothing.
	if (nova::wnd::activate_window_by_name("notepad.exe")) {
		// Hold the control key and press a and then x. (ctrl+a, ctrl+x)
		// This deletes the entire notepad, so close all of your important notepads before running.
		nova::keybd::shortcut_keys(VK_CONTROL, "ax");
		// Types "Hello world!" on the notepad. Slow typing has not been implemented (yet), so it types things, 
		// hits buttons, clicks the mouse etc. extremely and un-humanly fast.
		nova::keybd::typewriter("Hello world!");
	}
	return 0;
}