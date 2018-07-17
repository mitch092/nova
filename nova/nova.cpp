// This is Nova's windows implementation file. It does NOT use unicode. Changing the character set to UTF-8 would
// require rewriting many funcitons. 
#define WINVER 0x0500
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include "nova.h"


namespace nova {
	//---//Helper functions and types. These are very windows dependent, so they go in the implementation file.
	namespace util {
		//---//Helper types for keyboard
		static const HKL locale = GetKeyboardLayout(0);

		//---//Helper types for mouse
		//todo

		//---//Helper types for window
		// Here I define a HANDLE wrapper object, which is used in nova::util::retrieve_proc_ids. It can only be moved, not copied.
		class ProcessHandle {
			HANDLE handle;
			//Make sure not to close an invalid HANDLE.
			void close() {
				if (handle != INVALID_HANDLE_VALUE) {
					CloseHandle(handle);
				}
			}
		public:
			//The member functions include: a default constructor, default destructor, move constructor, move assignment, and returning the handle.
			ProcessHandle() : handle(INVALID_HANDLE_VALUE) {}
			~ProcessHandle() {
				close();
			}
			ProcessHandle(HANDLE&& right) : handle(right) {}
			ProcessHandle& operator=(HANDLE&& right) {
				if (handle != right) {
					close();
					handle = right;
				}
				return *this;
			}
			HANDLE getHandle() {
				return handle;
			}
		};
		class EnumWindowsProcHwnds {
			HWND window;
			DWORD proc_id;
			static BOOL CALLBACK StaticWndEnumProc(HWND hwnd, LPARAM lParam) {
				EnumWindowsProcHwnds *pThis = reinterpret_cast<EnumWindowsProcHwnds*>(lParam);
				return pThis->WndEnumProc(hwnd);
			}
			BOOL WndEnumProc(HWND hwnd) {
				DWORD lpdwProcessId;
				GetWindowThreadProcessId(hwnd, &lpdwProcessId);
				if (lpdwProcessId == proc_id)
				{
					window = hwnd;
					return FALSE;
				}
				return TRUE;
			}
			void update() {
				EnumWindows(StaticWndEnumProc, reinterpret_cast<LPARAM>(this));
			}
		public:
			EnumWindowsProcHwnds() : window(nullptr), proc_id(0) {}
			EnumWindowsProcHwnds(DWORD const id) : window(nullptr), proc_id(id) {
				update();
			}
			EnumWindowsProcHwnds(DWORD&& id) : window(nullptr), proc_id(id) {
				update();
			}
			EnumWindowsProcHwnds& operator=(DWORD&& id) {
				proc_id = id;
				update();
				return *this;
			}
			HWND getHwnd() {
				return window;
			}
		};


		//---//helper functions for keyboard
		INPUT make_input_kb(WORD _wVk, WORD _wScan, DWORD _dwFlags, DWORD _time, ULONG_PTR _dwExtraInfo) {
			INPUT ip = {};
			ip.type = INPUT_KEYBOARD;
			ip.ki.wVk = _wVk;
			ip.ki.wScan = _wScan;
			ip.ki.dwFlags = _dwFlags;
			ip.ki.time = _time;
			ip.ki.dwExtraInfo = _dwExtraInfo;
			return ip;
		}
		BOOL is_capital(TCHAR in) {
			return (HIBYTE(VkKeyScanEx(in, locale)) & 1);
		}
		INPUT get_input_char(TCHAR in, bool is_down) {
			// Collect all of the necessary data
			SHORT virtual_key_full = VkKeyScanEx(in, locale);
			UINT virtual_key = LOBYTE(virtual_key_full);
			UINT scan_code = MapVirtualKeyEx(virtual_key, MAPVK_VK_TO_VSC, locale);
			DWORD flags = KEYEVENTF_SCANCODE;
			if (!is_down) {
				flags |= KEYEVENTF_KEYUP;
			}

			// Return a keyboard INPUT struct
			return make_input_kb(0, scan_code, flags, 0, 0);
		}
		INPUT get_input_vk(WORD virtual_key, bool is_down) {
			UINT scan_code = MapVirtualKeyEx(virtual_key, MAPVK_VK_TO_VSC, locale);
			DWORD flags = KEYEVENTF_SCANCODE;
			if (!is_down) {
				flags |= KEYEVENTF_KEYUP;
			}
			return make_input_kb(0, scan_code, flags, 0, 0);
		}

		//---//helper functions for mouse
		INPUT make_input_ms(LONG _dx, LONG _dy, DWORD _mouseData, DWORD _dwFlags, DWORD _time, ULONG_PTR _dwExtraInfo) {
			INPUT ip = {};
			ip.type = INPUT_MOUSE;
			ip.mi.dx = _dx;
			ip.mi.dy = _dy;
			ip.mi.mouseData = _mouseData;
			ip.mi.dwFlags = _dwFlags;
			ip.mi.time = _time;
			ip.mi.dwExtraInfo = _dwExtraInfo;
			return ip;
		}

		//---//helper functions for window
		std::vector<DWORD> retrieve_proc_ids(std::string proc_name) {
			// ProcessHandle is a custom type that cleans up HANDLEs.
			ProcessHandle pe32 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			PROCESSENTRY32 entry;
			std::vector<DWORD> proc_ids;
			entry.dwSize = sizeof(PROCESSENTRY32);

			if ((pe32.getHandle() == INVALID_HANDLE_VALUE)) {
				// Maybe put assertion here.
				return proc_ids;
			}
			if (!Process32First(pe32.getHandle(), &entry)) {
				// Maybe put assertion here.
				return proc_ids;
			}
			do {
				if (std::string(entry.szExeFile) == proc_name) {
					proc_ids.push_back(entry.th32ProcessID);
				}
			} while (Process32Next(pe32.getHandle(), &entry));
			return proc_ids;
		}
		//get_hwnd may return nullptr.
		HWND get_hwnd(DWORD proc_id) {
			EnumWindowsProcHwnds hwnd(proc_id);
			return hwnd.getHwnd();
		}
		std::vector<HWND> get_window_handles(std::vector<DWORD> proc_ids) {
			std::vector<HWND> windows;
			HWND window = nullptr;
			for (DWORD id : proc_ids) {
				window = get_hwnd(id);
				if (window != nullptr) {
					windows.push_back(window);
				}
			}
			return windows;
		}
		// This function does the arcane and convoluted task of moving a window to the front of the desktop(!)
		bool activate_window_by_hwnd(HWND window) {
			HWND hCurWnd = GetForegroundWindow();
			DWORD dwMyID = GetCurrentThreadId();
			DWORD dwCurID = GetWindowThreadProcessId(hCurWnd, nullptr);

			// Here's the magic song and dance that activates/focuses/whatevers the window.
			AttachThreadInput(dwCurID, dwMyID, TRUE);
			SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			SetWindowPos(window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			SetForegroundWindow(window);
			AttachThreadInput(dwCurID, dwMyID, FALSE);
			SetFocus(window);
			SetActiveWindow(window);

			// Now check to see if the focused window is actually in the front.  
			return (GetForegroundWindow() == window) ? true : false;
		}
	}

	//---//Functions for using the keyboard
	namespace keybd {
		void typewriter(const char* _input) {
			std::string input = _input;
			static std::vector<INPUT> ks;
			static const INPUT shift_up = nova::util::get_input_vk(VK_LSHIFT, false);
			static const INPUT shift_down = nova::util::get_input_vk(VK_LSHIFT, true);
			INPUT press, release;
			bool last_cap = false, cur_cap;
			for (const char i : input) {
				press = nova::util::get_input_char(i, true);
				release = nova::util::get_input_char(i, false);
				cur_cap = nova::util::is_capital(i);
				if (!last_cap && cur_cap) {
					ks.push_back(shift_down);
				}
				else if (last_cap && !cur_cap) {
					ks.push_back(shift_up);
				}
				ks.push_back(press);
				ks.push_back(release);
				last_cap = cur_cap;
			}
			// If the last character was capitalized, then un-press shift here.
			if (cur_cap) {
				ks.push_back(shift_up);
			}
			SendInput(ks.size(), &ks.front(), sizeof(INPUT));
			ks.clear();
		}
		void press_char(char in, bool is_down) {
			INPUT ip = nova::util::get_input_char(in, is_down);
			SendInput(1, &ip, sizeof(INPUT));
		}
		void press_vk(unsigned short virtual_key, bool is_down) {
			INPUT ip = nova::util::get_input_vk(virtual_key, is_down);
			SendInput(1, &ip, sizeof(INPUT));
		}
		void hold_char(char in, int msec) {
			if (msec > 0) {
				press_char(in, true);
				std::this_thread::sleep_for(std::chrono::milliseconds(msec));
				press_char(in, false);
			}
			else {
				INPUT ks[2];
				ks[0] = nova::util::get_input_char(in, true);
				ks[1] = nova::util::get_input_char(in, false);
				SendInput(2, ks, sizeof(INPUT));
			}
		}
		void shortcut_keys(unsigned short virtual_key, const char* _keys) {
			std::string keys = _keys;
			std::vector<INPUT> ks; 
			ks.push_back(nova::util::get_input_vk(virtual_key, true));
			for (auto key : keys) {
				ks.push_back(nova::util::get_input_char(key, true));
				ks.push_back(nova::util::get_input_char(key, false));
			}
			ks.push_back(nova::util::get_input_vk(virtual_key, false));
			SendInput(ks.size(), &ks.front(), sizeof(INPUT));
		}
		bool is_pressed_char(char in) {
			WORD virtual_key = LOBYTE(VkKeyScanEx(in, nova::util::locale));
			return (GetAsyncKeyState(virtual_key) & 0x8000);
		}
		bool is_pressed_vk(unsigned short in) {
			return (GetAsyncKeyState(in) & 0x8000);
		}
		bool is_caps_on() {
			return (GetKeyState(VK_CAPITAL) & 0x0001);
		}
	}

	//---//Functions for using mouse
	namespace mouse {
		// for relative mouse moving, dx and dy are in units of pixels 
		void move_rel(long _dx, long _dy) {
			DWORD flags = MOUSEEVENTF_MOVE;
			INPUT ip = nova::util::make_input_ms(_dx, _dy, 0, flags, 0, 0);
			SendInput(1, &ip, sizeof(INPUT));
		}
		// for absolute mouse moving, dx and dy are in units from 1,1 to 65535,65535 
		void move_abs(long _dx, long _dy) {
			DWORD flags = MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_ABSOLUTE;
			INPUT ip = nova::util::make_input_ms(_dx, _dy, 0, flags, 0, 0);
			SendInput(1, &ip, sizeof(INPUT));
		}
		// similar to move_abs, but uses the dimensions of the screen (ie 0,0 to 1919,1079 for a 1920x1080 resolution desktop.)
		void move_abs_rect(long _dx, long _dy) {
			RECT desktop_rect;
			GetClientRect(GetDesktopWindow(), &desktop_rect);
			LONG x = _dx * 65536 / desktop_rect.right;
			LONG y = _dy * 65536 / desktop_rect.bottom;
			DWORD flags = MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_ABSOLUTE;
			INPUT ip = nova::util::make_input_ms(x, y, 0, flags, 0, 0);
			SendInput(1, &ip, sizeof(INPUT));
		}
		// 1 is the left button, 2 is the right button, 3 is the middle button, 
		// 4 is the first side button, 5 is the second side button.
		// If the first input is not an integer from 1-5 inclusive, then the function does nothing.
		void press_mouse(int button, bool is_down) {
			DWORD data = 0;
			DWORD flags = 0;
			switch (button) {
			case 1: {
				flags = (is_down) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
				break;
			}
			case 2: {
				flags = (is_down) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
				break;
			}
			case 3: {
				flags = (is_down) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
				break;
			}
			case 4: {
				flags = (is_down) ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
				data = XBUTTON1;
				break;
			}
			case 5: {
				flags = (is_down) ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
				data = XBUTTON2;
				break;
			}
			default: {
				// Error handling here
				return;
			}
			}
			INPUT ip = nova::util::make_input_ms(0, 0, data, flags, 0, 0);
			SendInput(1, &ip, sizeof(INPUT));
		}
		void click_mouse(int button) {
			press_mouse(button, true);
			press_mouse(button, false);
		}
		bool is_pressed_mouse(int button) {
			bool pressed = false;
			switch (button) {
			case 1: {
				if (nova::keybd::is_pressed_vk(VK_LBUTTON)) pressed = true;
				break;
			}
			case 2: {
				if (nova::keybd::is_pressed_vk(VK_RBUTTON)) pressed = true;
				break;
			}
			case 3: {
				if (nova::keybd::is_pressed_vk(VK_MBUTTON)) pressed = true;
				break;
			}
			case 4: {
				if (nova::keybd::is_pressed_vk(VK_XBUTTON1)) pressed = true;
				break;
			}
			case 5: {
				if (nova::keybd::is_pressed_vk(VK_XBUTTON2)) pressed = true;
				break;
			}
			default: {
				//Error handling here
				break;
			}
			}
			return pressed;
		}
		// The wheel_clicks argument determines how many clicks the mousewheel is scrolled.
		// Positive ints roll the wheel foreward, negative ints roll the wheel backwards.
		void scroll_wheel(int wheel_clicks) {
			DWORD flags = MOUSEEVENTF_WHEEL;
			DWORD data = WHEEL_DELTA * wheel_clicks;
			INPUT ip = nova::util::make_input_ms(0, 0, data, flags, 0, 0);
			SendInput(1, &ip, sizeof(INPUT));
		}
	}

	//---//Functions for grabbing the window
	namespace wnd {
		bool activate_window_by_name(const char* _exe_name) {
			std::string exe_name = _exe_name;
			std::vector<HWND> window_handles = nova::util::get_window_handles(nova::util::retrieve_proc_ids(exe_name));
			// Don't do anything if there are no window handles.
			if (window_handles.empty()) {
				return false;
			}
			// If there are many window handles that match the given exe name, use only the first one.

			// Deprecated. It works (sometimes... I want it to work reliably and all the time.)
			//SwitchToThisWindow(window_handles.front(), TRUE);
			
			return nova::util::activate_window_by_hwnd(window_handles.front());
		}
	}

}