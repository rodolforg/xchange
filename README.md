XChange - Hexadecimal editor
===============================

XChange is a binary editor that allows you to open any file - of any size -
and view it and edit it as you wish: replacing data, inserting some bytes, or
deleting another ones. All of this without need to laod the whole file into
memory.

Features
-------------------------------
*  Replace, insert or delete byte section
*  Replace, insert or delete bytes as text
*  Support for charsets as ISO-8859-*, UTF-8, Shift-JIS, EUC, etc. (via iconv)
*  Support for custom charset table files, like Thingy
*  Find bytes or strings
*  Quick switch between three tables
*  Full support for undo/redo
*  Customize GUI colors
*  Customize text rendering of raw bytes (those out of charset)
*  Customize how many bytes are shown per line
*  File/table handling (undo/redo included) and GUI are separated
    If you want to use it somewhere, then go just for the core library!
*  Code written in standard C: portable!
*  GUI done by GTK+: it is portable!

TODO
-------------------------------
Please check [TODO file](TODO.md).

License
-------------------------------
 Copyright 2010-2015 Rodolfo Ribeiro Gomes

  XChange is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  XChange is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with XChange.  If not, see <http:www.gnu.org/licenses/>.

