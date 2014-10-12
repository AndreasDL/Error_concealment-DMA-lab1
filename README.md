DMA
===

Design of multimedia applications Lab Session1

## Directory structure
* Assignment => Contains the assignment
* data (gitignore)=> contains all original yuv files & error streams
* solutions (gitignore) => contains all the encoded files
* Test_environment => exe's for encoding and measurement
* err_correction_src_vs13 => source code for vs2013

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