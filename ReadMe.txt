# HPG Assessment 1
# George Tattersall : 201467425

------------------------
| Running and Building |
------------------------

First of all, I use CMake, MSBuild files are evil, and looking at how
VS itself is embracing CMake, I'm pretty sure even they agree. To
compile this project you'll need to open the CMakeLists.txt at the root
of the project from "File -> Open -> CMake...".

These should be able to compile on Windows as long as the "VULKAN_SDK"
enviromental variable is defined (and of course, correct!) and you have
your SDK installed in a sensible CMake findable location. If not, I'll
see you both on Monday to be mocked for failing to make it compilable.

Once you have the project open properly you'll want to hit the dropdown
arrow next to the run button (top center) to select the project to run.
Or just use the "Build" tab at the top to build everything and run thing
manually by going into the build folder (or indeed install folder if you
choose to install).

I will include the CMakeSettings.json in the submission. Ninja and Visual
Studio generators are not equivalent, and so it's best those settings are
kept the same too. I also haven't tested Release builds, so have disabled
them.

-------------
| All Tasks |
-------------

Assets for the coursework are in the assets folder, the tasks are in the
tasks folders.

Although I had set up CMake to track the shader files, I then broke it.
If you change shader changes (for whatever reason) you'll need to REBUILD.

----------------
| Task 1 and 2 |
----------------
location:
	./task_1_and_2
description:
	To keep thing just like the tutorial while working through I did keep
	things in a monolithic file. I found it quite hard to comment a lot
	of line seeing as the Vulkan function names are quite descriptive
	themselves!

----------
| Task 3 |
----------
location:
	./task_3
description:
	I also kept the refactoring to a minimum here, again, to keep the
	code as similar to online resources as possible. Helped a lot while
	developing.

----------
| Task 4 |
----------
location:
	./task_4
description:
	At last, some refactoring. Although still minor...
	For a first journey into any new thing the pain of breaking it
	is sometimes too much to be worth making it neat. I at least
	have a main.cpp this time!

-------
| End |
-------
The stuff at the top about how to use CMake in VS was not intended to
patronise, but if you either of you aren't familiar with CMake in VS,
it may be helpful. Also I was scolded in a previous assessment for
using "over-complicated" CMake, so thought I would make it as clear a
process as possible.

I can't think of anything else to put in here. Probably not as breif
a ReadMe as was desired, sorry.