![!Theme Image](resources/icounter_header.png)
# Dynamo Tool
## Block counter
Simple instrumentation for instruction counting made on c++ to be used with [dynamorio](https://dynamorio.org/).

## Disclaimer

The instructions tool was made with the tutoring and help of [Andrei Rimsa](http://rimsa.com.br/page/). Andrei is my professor and thesis tutor from CEFET-MG.

## How to Install

### Install Dynamorio.

```bash
git clone https://github.com/DynamoRIO/dynamorio.git
cd dynamorio
DYNAMORIO_HOME=$(pwd);
```

### This tool package

```bash
# go into the correct folder
cd api/samples
# clone to the here
git clone https://github.com/afa7789/dynamorio_instruction_count.git dynamo_tool
# extract it
cp ./{dyn.patch,bbl_count.cpp} .
# Add line to CMakeLists
patch < dyn.patch
```
## Running the tool

### Make first

#### Make Dynamorio

make sure you have set DYNAMORIO_HOME env var at the root of the dynamorio folder.

```bash
cd $DYNAMORIO_HOME
mkdir build && cd build
cmake  -DBUILD_DOCS=no ..
make -j
```

#### make tool

```bash
cd build
make bbl_count
```
### Running it
Commands to run this code
#### bbl_count.c

```bash
$DYNAMORIO_HOME/bin64/drrun -c bin/libbbl_count.so -- ls 
# how to use to get the output to a file.
$DYNAMORIO_HOME/bin64/drrun -c bin/libbbl_count.so arquivo_test.txt -- ls 
# Example on how to run it with a full path
# By doing like this you can from anywhere in your computer
$DYNAMORIO_HOME/build/bin64/drrun -c /home/afa/Documents/code/TCC/dynamorio/dynamorio/build/api/samples/../bin/libbbl_count.so -- ls 
```
