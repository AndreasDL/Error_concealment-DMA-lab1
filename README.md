DMA
===

#Design of multimedia applications Lab Session1

## What
This is a lab session / project of the course 'Design of multimedia application' which is part of The ir. Computer science @UGent.
The program will simulate missing macroblocks and try to conceal them as good as possible.

## Run
Decoder_with_error_concealment.exe <enc file> <output file> <error pattern> <method>
* enc file: the encoded file (see 'Encoding' on how to encode video files)
* output file: the output in yuv format (you can use the Test_environment\YUVDisplay.exe to open it)
* error pattern: the error pattern to simulate the missing macroblocks
There're two error patterns: simple and complex. Simple means that a missing block will always have all his neighbours. 
This is no longer the case with complex.
* method: the concealment method Multiple are available
0:Interpolate the surrounding blocks, using the two closest neighbours
1:Interpolate the surrounding blocks, using the available neightbours
2:Use edge detection and try to conceal block by drawing the edges
3:No motion estimation. Block missing? Use the previous
4:Try to interpolate the motion vectors and use that to conceal the missing block. 
This is done by using a subsize of 2, the method may fall back on method 1 oif the error is too big.
5:same as 4, but with subsize of 4
6:same as 4, but with subsize of 8
7:same as 4, but with subsize of 16 (== size of a macroblock)
8:Motion intelligently optimized to tackle the complex pattern. (uses a combination of 9 and 1)
9:same as 4, but with a dynamical subsize.

##notes:
* mehtod 8 will always call 1 first and then try to improve the solution.

# Contributing information

## Directory structure
* Assignment => Contains the assignment
* Test_environment => exe's for encoding and measurement
* err_correction_src_vs13 => source code for vs2013
* data => sample error patterns

## Encoding 
* use Test_environment\Encoder.exe 
* Dont forget that the width and height should be mentioned in macroblocks (size= 16x16)

### Common bitstream1
Test_environment\Encoder.exe data\beowulf_848x352.yuv 53 22 20 20 solutions\common_natural_8.enc

### Common bitstream2
Test_environment\Encoder.exe data\elephants_dream_816x576.yuv 51 36 25 10 solutions\common_synthetic_8.enc

### Uncommon bitstream
Test_environment\Encoder.exe data\o_jerusalem_480x192.yuv 30 12 24 14 solutions\group_8.enc

## Decoding - Coding assignment
1. In VS press f7 or do a make
2. go to error_correction_src_vs13\Debug in your terminal.
3. Run the decoder as follows: Decoder_with_error_concealment.exe ..\..\solutions\common_natural_8.enc test.yuv ..\..\data\error_pattern_complex_beowulf.txt 1
4. Open yuv file with Test_environment\YUVDisplay.exe
5. set height & width (depends on video, look in Assigment\test_files_overview.pdf)