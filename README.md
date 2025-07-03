# Clay CPP Template

This is a template that sets up a C++ project using CMake (>= 3.28).  
The project uses the following libraries:

- Raylib
- Clay (C Layout)
- SDL 3 (but it's compatible with SDL 2)

Please note that this project is managed with jujutsu which, while git compatible, does a good amount of force pushes.

## Starting a project with it

Fork the project on github so you have the template in your profile and make a new repository using it as the template.  
After clonning your project locally make sure to get the submodules with `git submodule update --init`

## Known Issues

The known issues mostly pertain to the profiler right now and they're mentioned in `profiler.h` but I'll keep this section updated if/when it expands.

### root/utils/profiler.h

- The profiling macro needs to be the first thing in the scope to make sure it gets freed last.
  - IDK if there's any way around that
- This has not been tested in multithreaded apps.
  - Even if it could work out of the box IDK what settings should be used.

## License

This template is licensed under the MIT License. For more info check the license file.
