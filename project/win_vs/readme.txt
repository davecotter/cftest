note that even on windows, this is a Bundled application

that means it lives inside a “Folder” with the “.app” extension

so when building, Visual Studio will build the exe into the $(TargetDir) directory, and the 
default will be it will try to run it from there.

however, all the related CoreFoundation DLLs will have been staged into the app bundle, 
and also, the exe itself will have been copied there by the post-build step (running post_build.bat)

so you have to edit your project and tell it to debug from the app bundle directory!

to do so:

    a) select "CFTest", right click it and pick "Properties"
    b) in the "Configuration:" menu, pick "All Configurations"
    c) go to "configuration properties->debugging"
    d) set "Command" to "$(SolutionDir)build\$(TargetName).app\Contents\Windows\$(TargetFileName)"

note that d) only seems to work on VS2008

I have found that on VS2010 (and greater?), you have to just specify the whole path like this:

debug:
"$(SolutionDir)build\CFTest Debug.app\Contents\Windows\CFTest Debug.exe”

release:
"$(SolutionDir)build\CFTest.app\Contents\Windows\CFTest.exe”

if it's not staging correctly, ensure that the "post build event" for "CFTest" project has this:
"python ../post_build_cftest.py VS 32 $(ConfigurationName)"
for both debug and release