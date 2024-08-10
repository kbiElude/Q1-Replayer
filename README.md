# Q1-Replayer
A tool to capture &amp; analyze GLQuake rendering pipeline.

![The tool in its full glory](https://github.com/kbiElude/Q1-Replayer/blob/main/docs/screenshot.png?raw=true)

# How do I build the tool?
The easy way is to grab build.zip, extract it to any directory you wish and just run it.
Another easy way is to just build it locally. For Visual Studio 2022, you're going to need to:

1. Create a build directory in the root of the project.
2. Go inside it, fire cmake with --G"Visual Studio 17" -AWin32 ..
3. Open the result .sln file. Build the executable.

You need the executable to be 32-bit because, well, that's what was the only x86 arch around when Q1 was released, hence the funny -AWin32 bit. Don't forget it or you'll be sorry.

# How do I use the tool?
1. Install Quake 1. Steam distribution is recommended since it comes with GLQuake attached.
2. Run Launcher.exe. Point the tool to the directory where GLQuake.exe lives.
3. Once the game starts, capture a frame using F11. No worries, you can do this as many times as you please while the game executes.
4. Whenever you capture a frame, the API call window seen on the right will fill with a list of API calls required to render the frame. On the bottom, you can see a replay of the snapshot.

# But why?
This is a hobby project I implemented to get a better understanding of how Q1's rendering pipeline works. Will use this for something more interesting in a future project.

# Does this work for modded Quake builds?
One of the thing the tool does is analyzing the API call flow used to render the frame. Should your mod alter it in a significant manner, all hell may break loose because it might screw up with predefined heuristics.

# Oy, implement feature X for me plox.
See that pull request button? Feel free to propose new features if you fancy :) 
