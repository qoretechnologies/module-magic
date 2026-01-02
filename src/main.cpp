/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  libmagic Qore wrapper

  Qore Programming Language

  Copyright 2012 - 2022 Qore Technologies, s.r.o.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/** @mainpage %Qore Magic Module

\tableofcontents

This is a module for recognizing the type of data contained in a computer
file using so called magic numbers. It is a wrapper around <a href="http://darwinsys.com/file/">\c libmagic C
library</a> (covered by the standard Berkeley Software Distribution copyright).

This module is released under the <a href="http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html">LGPL 2.1</a>
and is tagged as such in the module's header (meaning it can be loaded unconditionally
regardless of how the %Qore library was initialized).

This module provides the following class:
- @ref Qore::Magic::Magic "Magic"

The <i>Single Unix Specification (SUS)</i> specifies that a series of tests are
performed on the file specified on the command line:
- if the file cannot be read, its status undetermined, or its type undetermined,
  @ref Qore::Magic::Magic "Magic" will indicate that the file was processed and its type was undetermined.
- file must be able to determine the types directory, FIFO, socket, block
  special file, and character special file
- zero-length files are identified as such
- an initial part of file is considered and @ref Qore::Magic::Magic "Magic" is to use position-sensitive tests
- the entire file is considered and @ref Qore::Magic::Magic "Magic" is to use context-sensitive tests
- the file is identified as a data file

@ref Qore::Magic::Magic "Magic" position-sensitive tests are normally implemented by matching various
locations within the file against a textual database of magic numbers.
This differs from other simpler methods such as file extensions and
schemes like MIME.

In most implementations, the file command uses a database to drive
the probing of the lead bytes. That database is implemented in
file called \c magic, whose location is usually in \c /etc/magic,
\c /usr/share/file/magic or a similar location.

To use the module in a %Qore script, use the \c %%requires directive as in the following example:
@code{.py}
%new-style
%strict-args
%require-types
%enable-all-warnings

# import the magic module API
%requires magic

# Get MIME type only
Magic m(MAGIC_MIME_TYPE);
printf("MIME type: %y\n", m.file("/etc/hosts"));

# Get MIME type with encoding
Magic m2(MAGIC_MIME);
printf("MIME with encoding: %y\n", m2.file("/etc/hosts"));

# Analyze buffer data
printf("Buffer type: %y\n", m.buffer("Hello, World!"));
@endcode
The above command would result in the following output when executed on a standard UNIX or UNIX-like system:
@verbatim
MIME type: "text/plain"
MIME with encoding: "text/plain; charset=us-ascii"
Buffer type: "text/plain"
@endverbatim

    @subsection magic_1_0_1 magic Module Version 1.0.1
    - aligned release with qpp from %Qore

    @subsection magic_1_0_0 magic Module Version 1.0.0
    - initial release
*/
