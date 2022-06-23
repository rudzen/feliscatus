# Felis Catus v2022.1

UCI Chess engine based on [Bobcat](https://github.com/Bobcat/bobcat) 8.0 by Gunnar Harms
Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
Copyright (C) 2017      FireFather (Tomcat author)
Copyright (C) 2020-2022 Rudy Alex Kohn

## Overview

- Complete refactored codebase
- Tuner is separated from engine and support CLI to tune parameters
- C++23
- CMake
- Portable

### Requires

- [CMake](https://cmake.org/) (or [CLion](https://www.jetbrains.com/clion/) / [VSCode](https://code.visualstudio.com/))
- [Conan](https://conan.io/), recommended to install on windows through [Chocolatey](https://chocolatey.org/)
- GCC 12.1.0 or newer (On Windows something like [MSYS2](https://www.msys2.org/) would do the trick)
