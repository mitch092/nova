#pragma once
// Nova is a wrapper over the win32 SendInput function
// Virtual keycodes are on the website https://docs.microsoft.com/en-us/windows/desktop/inputdev/virtual-key-codes
// The vk constants are defined in windows.h, so if you want to type VK_CONTROL instead of 0x11 when using press_vk to press the control key,
// then you need to manually include the windows.h. But if you don't want to do that, then nova won't include windows., so your global namespace 
// stays nice and unpolluted. Also, nova only calls SendInput with scancodes, never virtual key codes. Nova functions
// convert virtual keys to scancodes so that input simulation will work in directx apps (games, 3d apps, etc.)
// Unfortunately, I could not get the windows key to work using scancodes. In the future I might extend the 
// function parameters to include a bool that controls whether or not virtual keys are converted to scancodes.
namespace nova {
	//---//Functions for using the keyboard
	namespace keybd {
		// Types out a list of characters.
		void typewriter(const char* _input);
		// Presses an alphanumeric character.
		void press_char(char in, bool is_down);
		// For all of the other keys, press_vk uses the virtual key constants from Microsoft's website.
		void press_vk(unsigned short virtual_key, bool is_down);
		// Holds an alphanumeric character for an arbitrary number of milliseconds, then release.
		void hold_char(char in, int msec);
		// Holds a virtual key, presses the character, then releases both. Simulates ctrl+a, ctrl+v, etc.
		void shortcut_keys(unsigned short virtual_key, const char* _keys);
		// Checks to see if a certain key is being pressed down.
		bool is_pressed_char(char in);
		// Checks to see if a certain key is being pressed down. Uses vk constants instead of chars.
		bool is_pressed_vk(unsigned short in);
		// Checks if the capslock is on.
		bool is_caps_on();
	}

	//---//Functions for using mouse
	namespace mouse {
		// for relative mouse moving, dx and dy are in units of pixels.
		void move_rel(long _dx, long _dy);
		// for absolute mouse moving, dx and dy are in units from 1,1 to 65535,65535 
		void move_abs(long _dx, long _dy);
		// similar to move_abs, but uses the dimensions of the screen (ie 1,1 to 1920,1080)
		void move_abs_rect(long _dx, long _dy);
		// Preses or releases one of the five mouse buttons. 
		void press_mouse(int button, bool is_down);
		// click_mouse calls press_mouse twice, pushing the button down and releasing it.
		void click_mouse(int button);
		// Checks to see if mouse buttons 1-5 are being pressed down.
		bool is_pressed_mouse(int button);
		// Moves the mouse's scroll wheel. wheel_clicks is the number of mouse clicks to scroll. Positive is forward, negative is back.
		void scroll_wheel(int wheel_clicks);
	}

	//---//Functions for grabbing the window
	namespace wnd {
		// Takes the name of an executable file and tries to alt-tab into it's window.
		// Ignores child windows and only checks top windows.
		// If there are multiple executable files that match the name to be searched, it alt-tabs into the first window.
		bool activate_window_by_name(const char* _exe_name);
	}
	
}