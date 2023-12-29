# Tekken 3 BinStream Extractor & Packer

This is a simple utility which allows you to unpack and pack the TEKKEN3.BNS file.

It achieves this by analyzing the game executable to retrieve the LBA table (and can update it accordingly).

## Usage

In order to extract a TEKKEN3.BNS file, you must provide a matching game executable with the file.

Here's a simple usage printout from the utility itself:

```
USAGE: tek3ex BinStreamPath ExePath OutFolder
USAGE (manual pos): tek3ex -m BinStreamPath ExePath LBATablePosInExe FileCount OutFolder
USAGE (pack): tek3ex -p InFolder ExePath OutBinStream
USAGE (manual pos pack): tek3ex -mp InFolder ExePath LBATablePosInExe FileCount OutBinStream
```

Example: `tek3ex TEKKEN3.BNS SLPS_013.00 tekken3bnsfiles` should extract files to a folder "tekken3bnsfiles".

The utility will attempt to autodetect the LBA table, however, in cases where it cannot find it, you may use the manual mode.

### NOTES

- Please note that you cannot add or remove any files. Always use the same number of files that it was extracted with for repacking! In case there's a mismatch, you will be warned accordingly! Things will break if there is a mismatch!

- Keep the filenames numeric! Any non-numeric filename will be ignored! (extensions aren't taken into consideration, however, so you may use any extension)

- Theoretical maximum number of files is UINT16_MAX, or 65535 files

- Location of the TEKKEN3.BNS files isn't hardcoded, the game automatically detects its position! However, keep in mind, for any size changes, you **must** completely rebuild the ISO from scratch!

- Check the  `useful-stuff` folder for more info about files and a way you can see which LBAs game is accessing in realtime (only for JP build for now)

## TODO

- Tekken 2? Appears to use a very similar format.

- Document more of the game stuff in general and make the debug cheat for the US build
