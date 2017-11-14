https://www.cs.utexas.edu/~theshark/courses/cs354/assignments/assignment_5.shtml
# Getting Started

## Build

You are already quite familar with the graphics projects, so
TL;DR:

```
mkdir build
cd build
cmake ..
make -j8
```

## Execution

```
cd build
./bin/skinning ../assets/pmd/Meiko_Sakine.pmd
```

You need to provide a .pmd file to launche the skinning code. A set of PMD
files have been shipped under assets/pmd directory.

## Notes about the skeletion code

The skeleton code is trimmed from the reference code, which has a RenderClass
to simplify the multi-pass rendering. However, this class is somewhat
sophisticated. If you find it is quite hard to understand the RenderClass, do
NOT use it. Testing your C++ skill is not a part of this assignment.

## Package for submission

The submitted package assumes the same file structure as the published one.
You should check your submission with the command

```
./skinning_testpackage.sh <your package file name>
```

You should get a "Build successfully" message from the script, and a
skinning.bin as the build result.

skinning_testpackage.sh is a script under the skinning/ directory.

WARNING: THIS SCRIPT RUNS ``rm -rf skinning`` AS THE LAST STEP FOR CLEAN UP.
BACKUP YOUR CODE AND RUN IT IN A SAFE PLACE (LIKE $HOME/tmp) TO AVOID ANY
POSSIBLE DISASTER.

# Acknowledgement 

This bone animation code is based on the skinning project written by
Randall Smith for 2015 Spring Graphics Course.

The PMD parser library is written by
[itsuhane](https://github.com/itsuhane/libmmd), licensed under Boost Software
License.

The author of PMD models is Animasa, you can download the model with the
official editor from his webpage [VPVP](http://www.geocities.jp/higuchuu4/index_e.htm).
