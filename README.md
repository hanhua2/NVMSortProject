## Running the code

**Note: Please change the target file paths in ```GenerateData.cpp``` and ```sortMain.cpp``` before attempting to run the bash scripts**

### 1. Generating data to be sorted in NVM
**Generated data should be on NVM, and is currently set to "/optane/hanhua/UNSORTED_KEYS" by default**. By default, the script creates a 16GB (16777216 items) sized file of 32-byte sized Records (key, val) in NVM.\
Usage:\
```bash GenerateData.sh```

### 2. Running the algorithm on the generated data
**Target file to sort should be on NVM, and needs to match the generated data's file path.** The number of items to be sorted needs to be specified as command line arguments to the program. Please edit the ```sort.sh``` bash file directly if you wish to run different experimental setups.\
Usage:\
```bash sort.sh``` 