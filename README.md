# What is nova?
Nova is a windows only c++ wrapper library that uses SendInput to simulate user input in a programmatic fashion. The library uses scancodes as opposed to virtual keys in order to simulate keyboard and mouse input for 3d applications, like games.
# Why did you make this library?
I wanted to write a bot that can play games. The first stumbling block was that I needed some way to simulate user input.
# Why not just use pyautogui/pyautowin/etc.?
Pyautogui uses virtual key codes, which do not work with directinput games. I also wanted to acclimate myself to Visual Studio, github, windows32 programming, and c++ in general. I could not find a pyautogui library that uses scancodes. The only thing that I found were stackoverflow answers and some blog posts stating how  to re-implement parts of pyautogui in python. I decided that if I am going to reimplement a subset of pyautogui, then it is going to be in a language that I know, using tools that I am familiar with. 
# What are some issues with the library?
* There is no unicode support. By the time I found out that I can change the character encoding, I had already implemented a bunch of functions using regular std::string and the like (instead of std::wstring.) 
* Only scancodes are used. My windows key does not work when I use scancodes. Maybe in the future I will add in a bool parameter to all applicable functions that gives the user the option of using scancodes or virtual keys, rather than forcing them to use one or the other.
