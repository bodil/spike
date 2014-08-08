# Spike

Spike is a simple application launcher, inspired by
[dmenu](http://tools.suckless.org/dmenu/).

## Installation

To install Spike, first install
[Qt version 5.3](http://qt-project.org/qt5/qt53) or higher. It might
work with earlier Qt 5.x versions, but these are unsupported.

To compile and install:

```sh
$ qmake
$ make
$ sudo install build/spike /usr/local/bin/spike # or wherever you want it
```

## Running

Simply launch the `spike` executable, and you'll be presented with a
menu of installed XDG applications. Launching `spike -s path` will
instead present you with all executables available on your system
path.

Start typing to narrow your search, navigate through displayed options
using the left and right arrow keys, and launch the current selection
with the return key. You can cancel without launching anything using
the Escape, Ctrl-C or Ctrl-G keys.

## License

Copyright (C) 2014 Bodil Stokke

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see
[http://www.gnu.org/licenses/](http://www.gnu.org/licenses/).
