# Compaq Portable III and Portable 386 mode change utility

Compaq Portable III and Portable 386 are very similar, except the CPU used. Both can change video-adapter used: MDA or CGA, CPU speed and several other options.

MODE\.COM utility provided with the Compaq DOS can change this options, but it is linked with exact DOS version. While writing an [article about those computers](), (in Ukrainian, [though more or less translatable by the Google]()), I decided to recreate some of it's functionality to check if I understand corresponding "APIs" correctly.

Released under the [MIT License](LICENSE). No guaranties, as always. Could work to some extent with other similar Compaq models.

## Usage

Currently is supported:

- retrieving information about the CPU speed, video-adapter usage and capabilities:
  - ``setmode print=CPU|video|all``
- changing CPU speed:
  - ``setmode CPU=common|fast|high|auto|toggle|<1-50>``
  - Numerical codes are ignored on the Portable III (286), but acknowledged on the 386.
- changing video-adapter used:
  - ``setmode video=CGA|MDA``
- untested (and, because of long way debugging other features, possibly -- not working) feature -- switching to external monitor and back:
  - ``setmode monitor=external|internal``

Ability to configure other properties could be added in the future.

QEMM386 and similar DOS extenders are detected -- they preclude correct operations. 

## Using MODE\.COM

Using SETVER enables usage of the Compaq MODE\.COM from the DOS 3.31 (and other versions, I hope): ``MODE SPEED=HIGH``.

## Compilation

Compiles using the Open Watcom 2, project files are provided. If required, I can describe building from the command-line -- feel free to fire an issue.

Additional ASM utilities provided are compiled by the TASM (version 3.1 was used accidentally):

- SetCGA.ASM -- use CGA adapter,
- SetMDA.ASM -- use MDA adapter,
- SetHIGH.ASM -- set speed to HIGH,
- SetALL.ASM -- set CGA and maximal speed.

Code is rather quick-and-dirty.

All binaries are provided.
