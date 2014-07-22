libjel -- JPEG Embedding Library
==========

This library supplies an API for embedding and extracting bit strings
into / from JPEG images.  The library uses heavily quantized frequency
components to provide storage that is stabilized by the JPEG
compression process.  As a result, embedded bit strings are preserved
under a number of different transformations, including transcoding to
higher quality, DC or low frequency shifts in pixel value, and limited
image rescaling.

Please see *"TRIST: Circumventing Censorship with
Transcoding-Resistant Image Steganography"*
for a detailed explanation on what this tool does and how it does it.
A copy can be found here: 
```
https://github.com/SRI-CSL/jel/blob/master/doc/jpegsteg.pdf?raw=true
```

General Compilation
-------------------


Prepare the raw git repository first and generate configure and Makefile:
```
autoreconf -i
./configure --enable-silent-rules
```

To compile manually:
```
make
```

Debian
------

To make a Debian package:
```
make deb
```

