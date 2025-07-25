# MemProfiler View

This project should probably be called "Memory Profiler **WITH** Viewer". I just couldn't be bothered.  
This is a library that helps profile your C++ code in a semi-automatic way.  
It also compiles into an executable to visualize the results of your profiling.

# Usage

> TODO add instalation instructions
> TODO add explanation on how to configure CMAKE
> TODO add explanation on how to use it

## Roadmap-ish

This is mostly a portfolio piece, please do not expect very active development. Unless it becomes super popular, in which case I'd be willing to put more time into it. That being said here's how I would like to add more features to this.

1. Add floating/click pane that shows the stack trace on a given memory address.
2. Make it so that you can profile both time and memory simultaneously.

### Wishlist

These are things that I would like to add but IDK if they're feasible.

1. If C++ ever gets a good-ish reflectiion system I would like to add the type of the variable to the log/address list.

## Known Issues

Unless stated otherwise please assume that even if a fix is "planned" I'll probably won't have a due date set as this is mostly a hobby/portfolio piece. If this library somehow becomes popular and gains traction I'll definitely get more involved but for now this is a "for me by me" kind of project.

### Fix planned

### Fix not planned
- If the viewer is opened and closed immediately without reading a results file, its own results file marks all memory as leaked but if a file is analyzed it gets marked as clean
- You can only profile execution time or memory usage but not both at once.
- If you overscroll past the end of the timeline you have to scroll back for a while before it actually starts scrolling back
  - This is due to how I handle mouse wheel scrolling. I don't have immediate plans on fixing it rn. It's as simple as signalling the FW class that a module (likely ProfileRenderer) reached the clamping values and reclamp them.
- There is no error checking when files are dropped.
- This library has not been tested outside of Windows.
- The window cannot be resized nor maximized.


## License

This template is licensed under the MIT License. For more info check the license file.
