# Imager - Binary Image Creator

Reads one or more binary files and lays them out into a single image, padding
as needed (using a specified PAD byte) to put the source content into a
specific start location in the image.

If a source image is larger than the block defined by its start and the start
of the next source image it can be treated as an error that stops processing
or as a warning with the image being trimmed to the size defined by the start
of the next image.

## Command Line

The image can be constructed from arguments on the command line.

``` shell
imager -opts image_file_name source1[:start1[:end1]] [[source2[:start2[:end2]] [sourcen[:startn[:endn]]]]]
```

opts:

* b - Byte to use for FILL (if not specified, FF hex is used)
* f=file - Control file that specifies the sources and starts
* t - Truncate prior content in the case of overlap with following content
* w - Same as 't', but also print a warning

## Control File

Using the `f=file` option to specify a control file, the control file content
is like the command line with source file, start, and end arguments one per line. Comments
can also be included by preceding them with ';'. All content from the ';' character to the end of the line is ignored.

Example:

``` text
; These are the source files and start locations.
source1[:start1[:end1]]
source2[:start2[:end2]]
...
sourcen[:startn[:endn]]
```

