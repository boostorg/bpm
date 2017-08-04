# bpm

`bpm`, the Boost package manager, is an experimental utility for installing parts of Boost from modular "releases".

To build it, this repository needs to be cloned into the `tools/bpm` subdirectory of the Boost superproject, then `b2 tools/bpm/build` will place `bpm` into `dist/bin`.

To try it out, make an empty subdirectory and create a text file `bpm.conf` there with the following contents:

```
package_path=http://www.pdimov.com/tmp/pkg-develop-1612497/
```

then to install, for example, the `filesystem` library along with its required dependencies, execute `bpm` from that directory with

```
bpm install filesystem
```

You can also run `bpm` without arguments, and it will display a description of the commands and options it takes.

The "release" specified in `package_path` above has been prepared by running `tools/bpm/scripts/package.bat` at the root of the Boost source tree, revision `develop-1612497`.
