# Felis Catus v2022.1

UCI Chess engine based on [Bobcat](https://github.com/Bobcat/bobcat) 8.0 by Gunnar Harms
Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
Copyright (C) 2017      FireFather (Tomcat author)
Copyright (C) 2020-2025 Rudy Alex Kohn

## Overview

go wtime 122000 btime 120000 winc 2000 binc 2000

- Complete refactored codebase
- Tuner is separated from engine and support CLI to tune parameters
- C++23
- CMake
- Portable

### Requires

- [CMake](https://cmake.org/) (or [CLion](https://www.jetbrains.com/clion/) / [VSCode](https://code.visualstudio.com/))
- GCC 15.2.0 or newer (On Windows something like [MSYS2](https://www.msys2.org/) would do the trick)
