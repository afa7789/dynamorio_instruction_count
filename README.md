![!Theme Image](resources/tool_header.png)
# Dynamo Tool
## Instruction Count 
Simple instrumentation for instruction counting made on c++ to be used with [dynamorio](https://dynamorio.org/).

## Disclaimer

These instructions tools have a hashmap folder, it is a c library made with [Andrei Rimsa](http://rimsa.com.br/page/) `mostly him` :rofl: . Andrei is my professor and thesis tutor from CEFET-MG.

## How to Install

### Install Dynamorio.

```bash
git clone https://github.com/DynamoRIO/dynamorio.git DRIO
cd DRIO
git checkout cronbuild-8.0.18789
```

### This tool package

```bash
# go into the correct folder
cd DRIO/api/samples
# clone to the here
git clone https://github.com/afa7789/dynamorio_instruction_count.git tool_folder
# extract it
mv tool_folder/* .
rm -rf tool_folder
# Add line to CMakeLists
SAMPLE='add_sample_client(bbbuf       "bbbuf.c"         "drmgr;drreg;drx")'
NEW_LINE='add_sample_client(instruction_count "instruction_count.c;utils.c;hashmap/hash.c" "drmgr;drreg;drx")'
sed -i 's@'$SAMPLE'@'$SAMPLE'\n'$NEW_LINE'@' CMakeLists.txt
```

## Running the tool

### Make first

#### Make Dynamorio

```bash
cd dynamorio
DYNAMORIO_HOME=$(pwd);
mkdir build && cd build
cmake  -DBUILD_DOCS=no .. -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake /path/to/tool_folder
# make -j
```

#### make tool

```bash
cd build
make instruction_count
```
### Running it

#### instruction_count.c

```bash
$DYNAMORIO_HOME/bin64/drrun -c bin/libinstruction_count.so -- ls 
$DYNAMORIO_HOME/bin64/drrun -c bin/libinstruction_count.so arquivo_test.txt -- ls 
```
