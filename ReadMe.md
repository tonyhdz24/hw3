# CS 452: Homework #3 : Execution Integrity and Do Process
## Antonio Hernandez (114225246)
### 1. Code Overview
For this homework assignment we were tasked with fixing and implementing supplemental features to a basic shell. The end goal was to implement a shell that worked similarly to bash shell. As stated we were given a basic shell that could run some commands correctly and could run some commands with a few bugs. After fixing the bugs in the starter code we were tasked with implementing some simple functionality in our shell such as being able to use pipes |, redirects > <, and the ability to have processes run in the background or foreground.

Overall I feel that this homework assignment while challenging did a good job of helping me to understand how processes work and just how shells like bash work in general. It was actually kinda fun to build a shell that does so much. The hardest part was definitly implementing the pipes. However I felt that the provided assignment handout did a good job at guiding me to be able to break things down and complete the homework

### 2. How to Run Test Suite
*NOTE:* Some of the test I wrote rely on being run from my directory such as the `pwd` command test checks if the output is equal to my current directory

To run the test suite run the following commands. There will be no need to input anything and the test results will be displayed in the terminal. Any tests that fail will have a `failed` flag after the test name. All test that passed will just display the test name

1. Open a terminal in Onyx
2. cd into the folder contain all relevant files
```
cd /path-to-files/hw3
```
3. Once there compile all the files
```
make clean
make
```
4. Then run the test script
```
Test/run
```
5. No futher input will be needed and the results of the test suite will be displayed in the terminal

### 3. Results
Below are my test suite results

Test_echo
Test_input_redir
Test_output_redir
Test_pipeline
Test_pipeline_wc
Test_pwd
Test_sequence
Test_sequence_2

### 4. Sources Used
[Notes provided in class](https://github.com/BoiseState/CS453-resources/tree/master/buff/classes/452/pub)
[Execution Integrity and Do Process Starter code](https://github.com/BoiseState/CS453-resources/tree/master/buff/classes/452/pub)