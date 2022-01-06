
MAr documentation (beta)
========================

Introduction
------------

MAr is the archiver that comes along with the mlib distribution. The name MAr
stands for "Melting-Pot ARchiver". MAr archives are directly readable by
MLib internal functions as if they were directories containing files.
You create with MAr an archive containing all the files you need and just tell
MLib the archive name before your first file access; simple, eh ?
That way, you can save disk space by having your files stored in a compressed
form. Once you created the archive, only that file is needed so it simplifies
both backups and displacements of your data, as well as release of your demo.

Overview
--------

Features
--------

In order to fulfill MLib requirements (like portability), MAr handles:

- little/big endian byte orders (you can create an archive on a computer and
  extract/use it safely on any other machine, whatever byte order it is base
  on)

Limitations
-----------

Due to its full ANSI-C compliance, MAr is unable to deal with the following:

- archiving/extracting (sub)directories, you have to specify all the file names
- keeping track of file informations such as file types (links, etc...),
  owner/group, permissions, last modification date

Additionnal limitations come from my own lazyness compliance (this may be
fixed in future versions; the limitations I mean, not my compliance):

- no streaming is possible: files and archive are accessed by names, and can't
  be fetched from/sent to standard input/output

Usage
-----

General MAr usage is:

```
mar [<options>] <command> <archive> [<files>]
    <option>  = -a <algorithm> | -o <order> | -t [<delta>] | -v [<level>]
    <command> = (a)dd | (c)reate | (d)elete | (l)ist | (t)est | e(x)tract
    <algorithm> = store | lzh | bwt | ppm

```
File names may include the '*' and '?' wildcard characters.

Usage by command:

* (C)reate archive

  `mar [<options>] c <archive> <files>`

  Creates a *new* archive (from scratch) and puts files in it. If the archive
  already existed before the (C)reate action, it is destroyed.

  Useful options are:

  - `-a <algorithm>` (store/lzh/bwt/ppm, default = lzh, store = no compression)

    Selects the compression algorithm. As a general rule, lzh is better for
    compression speed, and bwt is better for compression ratio; ppm should
    be avoided since it's slow at decompressing and needs *much* memory. If
    you suspect that the file is already in a compressed form, use "-a store".

  - `-g`

    Turns on huffman blocks grouping for better compression ratio. This affects
    both the lzh and bwt algorithms. This needs additional time.

  - `-o <order>` (1 to 5, default = 3)

    Selects the PPM order (number of previous bytes that are used for
    prediction). The higher, the better the compression ratio but also the more
    memory you will need.

  - "-t [<delta>]" (0 to 4, default = 0, no delta)

    Selects the delta encoding distance. If delta if different from 0, delta
    encoding is performed in a byte per byte basis (no carry). This can be
    useful for compression ratio of some so-called "multimedia files" like
    pictures or sound. General advice is:

    - use 1 for 8-bit mono sound or 8-bit pictures (grey scale or colormap)
    - use 2 for 8-bit stereo/16-bit mono sound and 16-bit true color pictures
    - use 3 for 24-bit true color pictures
    - use 4 for 16-bit stereo sound and 32-bit true color pictures (with alpha)

    "-t 4" may also help compressing structured data files containing integers
           or floating point numbers tables.

* (A)dd archive

  `mar [<options>] a <archive> <files>`

  Appends files to an *already existing* archive. The file(s) must not be
  already present in the archive. If you want to update (a) files(s), use
  the (D)elete command first.
  Options are the same than for the (C)reate command.

* (D)elete archive

  `mar [<options>] d <archive> [<files>]`

  Deletes files from an existing archive. The files don't need to be already
  present in the archive.

* (L)ist archive [<files>]

  `mar l <archive>`

  Displays archive content into standard output. Here are the fields meaning:

  - Size    : (uncompressed) file size
  - Crunched: compressed file size
  - Algo    : compression algorithm (STORE = no compression)
  - D       : delta encoding distance (0 = no delta)
  - Ratio   : percentage of the file that was saved by compression (0 = none)
  - BPB     : compression ratio in Bits Per Byte unit (8 = no gain)
  - CRC     : Adler32 of the file
  - Name    : file name

* (T)est archive [<files>]

  `mar t <archive>`

  Decompresses each file in memory, and checks that the decompression is OK
  by comparing the CRCs. Complains only if something goes wrong.

* e(X)tract archive

  `mar x <archive> [<files>]`

  Extract the archive files into the current directory. Note that the archive
  is not modified in any way.

Note
-------
This is the original documentation from when this tool was first created.
This tool is placed on Github so that the MAr archive format can continue
to be extracted in the future. 
  
Minor tweaks were made to the Makefile for it to compile w/gcc in 2022.

Original Author
-------

Xann / Melting-Pot (xann [ at ] melting-pot [.] org).

