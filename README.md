# SatisfactoryModBootstrapper
xinput1_3.dll hook for Satisfactory to load other modules and resolve their symbols in runtime via PDB.

Will load module named "ExampleMod-Win64-Shipping.dll" on game loading, resolving all of it's imports from game PDB file
and will call BootstrapModule(LoadModuleFuncPtr) on the loaded module (it should be exported to work!).
See example of ExampleMod in the SatisfactoryUnrealProject.

LoadModuleFuncPtr allows you to load other modules on demand via the same loading mechanism.
* Note that BootstrapModule won't be called on them, it's your job to do it manually!

