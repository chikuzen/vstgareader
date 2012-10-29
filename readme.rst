=================================================
vstgareader - TARGA image reader for VapourSynth
=================================================

TARGA(Truevision Advanced Raster Graphics Adapter) image reader for VapourSynth.

Usage:
------
    >>> import vapoursynth as vs
    >>> core = vs.Core()
    >>> core.std.LoadPlugin('/path/to/vstgareader.dll')

    - read single file:
    >>> clip = core.tgar.Read(['/path/to/file.tga'])

    - read two or more files:
    >>> srcs = ['/path/to/file1.tga', '/path/to/file2.tga', ... ,'/path/to/fileX.tga']
    >>> clip = core.tgar.Read(srcs)

    - read image sequence:
    >>> import os
    >>> dir = '/path/to/the/directory/'
    >>> srcs = [dir + src for src in os.listdir(dir) if src.endswith('.tga')]
    >>> clip = core.tgar.Read(srcs)

Note:
-----
    - Only 24bit/32bit-RGB(uncompressed or RLE) are supported. Color maps are not.

    - All alpha channel data will be stripped.

    - When reading two or more images, all those width and height need to be the same.

    - This plugin is using part of libtga's source code.

How to compile:
---------------
    on unix system(include mingw/cygwin), type as follows::

    $ git clone git://github.com/chikuzen/vstgareader.git
    $ cd ./vstgareader
    $ ./configure
    $ make

    if you want to use msvc++, then

    - rename *.c to *.cpp
    - create vcxproj yourself

Link:
------
    vstgareader source code repository:
        https://github.com/chikuzen/vstgareader/

    libtga:
        http://tgalib.sourceforge.net/

Author: Oka Motofumi (chikuzen.mo at gmail dot com)
