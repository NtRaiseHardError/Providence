# Providence
Kernel-mode file scanner.

Based heavily on https://github.com/Microsoft/Windows-driver-samples/tree/ba25139ebc243c1a79dcd6168cc95563a467b7ef/filesys/miniFilter/scanner.

Detects file creation in a kernel-mode driver and then sends the file path to the user-mode application for signature scanning. Also prevents file deletion of its own files.

## TODO

1. Remove hard-coded file path of own critical files and place into registry to be used by `RegistryPath` in `DriverEntry`.
2. Integrate YARA C library.
3. Add driver loading into the user-mode application.
4. Change user-mode application into DLL.
5. Complete C# GUI to be used with the user-mode DLL.
