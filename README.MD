# PCIEDIT

## What is this?

This is a program for MS-DOS and compatibles (FreeDOS, etc.) to edit PCI configuration registers of any PCI device in the system.

## Usage

`PCIEDIT.EXE <ven> <dev> [register list]`

`ven`: This is the Vendor ID of the requested device. It must be a 16-bit hexadecimal number.

`dev`: This is the Device ID of the requested device. It must be a 16-bit hexadecimal number.

`register list`: (optional) This parameter specifies a file with a list of registers to change for this device. See below for the format of this file.

If the `register list` parameter is omitted, the program just prints the current contents of the PCI device's configuration registers (if found).

### Register list file layout

A register list file is just a text file with register index / value pairs.

Each line is formatted as fullows:

`xx yy`

`xx` is the register index / offset to change.
`yy` is the value to set this register to.

Both components are 8 bit hexadecimal numbers.

#### Example

```
50 df
57 0e
5a ff
```

## `RG2TXT`

RG2TXT is an included program that lets you convert `RG` files output by **WPCREDIT** (a popular Windows-based PCI config register editing tool from back in the day) for use in `PCIEDIT`. Its usage is very simple:

`RG2TXT <rg file> <output file>`

***WARNING***: Be aware that setting all these registers in bulk may very well lead to unintended effects and crashes. You should edit the resulting text file and only keep the register/value pairs that you specifically wish to set.

## Limitations

This program currently only allows you to change the registers of the *first function* (function 0). This will be changed in a future version.

## What can you do with this?

With `PCIEDIT`, you can do register level tweaking of PCI devices to work around hardware or driver problems, or improve performance of a computer in DOS or Windows 3.1/95/98/ME.

For this purpose, you may put `PCIEDIT` in your `AUTOEXEC.BAT` file.

It can be run under Windows 9x as well, since these versions of Windows permit direct hardware port access for DOS applications. Be careful though ;).

# Building

- Install OpenWatcom and set up the default environment as described in its README.
- Run `wmake` in the repository folder
- That's it!

# License

[CC-BY-NC 3.0, click here for info](https://creativecommons.org/licenses/by-nc/3.0)
