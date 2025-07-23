# MemProfiler View

This project should probably be called "Memory Profiler **WITH** Viewer". I just couldn't be bothered.  
This is a library that helps profile your C++ code in a semi-automatic way.  
It also compiles into an executable to visualize the results of your profiling.

# Usage

> TODO add instalation instructions
> TODO add explanation on how to configure CMAKE
> TODO add explanation on how to use it

## Known Issues

Unless stated otherwise please assume that even if a fix is "planned" I'll probably won't have a due date set as this is mostly a hobby/portfolio piece. If this library somehow becomes popular and gains traction I'll definitely get more involved but for now this is a "for me by me" kind of project.

### Fix planned

- programs explode if the window is closed manually
  - This has to do with how the profiler is set-up. IDK if I'll migrate to module architecture and then fix it or vice-versa. I haven't debuged the lifetime of the profiling objects to entirely tell which makes more sense.

### Fix not planned
- If you overscroll past the end of the timeline you have to scroll back for a while before it actually starts scrolling back
  - This is due to how I handle mouse wheel scrolling. I don't have immediate plans on fixing it rn. It's as simple as signalling the FW class that a module (likely ProfileRenderer) reached the clamping values and reclamp them.
- There is no error checking when files are dropped.
- This library has not been tested outside of Windows.
- The window cannot be resized nor maximized.


## License

This template is licensed under the MIT License. For more info check the license file.
