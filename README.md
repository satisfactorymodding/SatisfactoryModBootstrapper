# SatisfactoryModBootstrapper
xinput1_3.dll hook for Satisfactory to load other modules and resolve their symbols in runtime via PDB.

It will load all modules in the directory "loaders" located in the game's root directory, resolve all
of their UnrealEngine & Satisfactory function import dependencies, and call BootstrapModule
on each of the modules. It provides a low-level API to load additional DLLs which will have their
exports resolved too, which can be used for creating fully functional mod loaders or core mods
dependent only on the game's and Unreal Engine's code.
Module loading order is defined by the alphabetical ordering of their names.

BootstrapModule signature as follows:
```
#include "bootstrapper_exports.h"

extern "C" DLLEXPORT void BootstrapModule(BootstrapAccessors& accessors);
```
Note that you should copy `src/exports.h` to your project and rename it to something
like `bootstrapper_exports.h` to use functions provided by the bootstrapper.

BootstrapAccessors provide access to various functions of the bootstrapper,
including checking if module is loaded, loading another module, resolving it's
addresses, retrieving root game directory path.
List of the exposed properties can be modified in future releases,
but changes are guaranteed to be backwards compatible.

