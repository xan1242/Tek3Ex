# File mapping

TODO: create a table or a spreadsheet or something, this is a mess...

## Tekken 3 SCPS-45213 / SLPS-01300 (NTSC-J)

### Embus

- file 1 - regular embu with primary character skins

- file 2 - embu with secondary character skins

- file 3 - bonus embu with goofy characters

## Code overlays

TODO: analyze where these are loaded in memory exactly!



file 0 - unknown, contains copyright of Namco

### Arcade mode / regular game

file 5

### Practice mode

file 6

### Tekken Force

file 7

### Tekken Ball

file 8

### Character select

file 9 

### Title screen & main menu

file 10

## Other resources

### Stage Textures

files 36 to 55?

### Stage Models

files 56 to 70?

### Characters

Each character comes with a pack of 4 files;

- 3d model

- VAB header

- archive with texture, sound, name index (maybe other stuff)

- bin file (probably some code overlay) - this one isn't being loaded during normal gameplay

Starts with file 71 and ends with 277.

Enumeration follows the hardcoded logic (this is valid for stages too!), which goes as follows:

- PAUL

- LAW

- LEI

- KING

- YOSHIMITSU

- NINA

- HWOARANG

- XIAOYU

- EDDY

- TIGER

- JIN

- JIN (yes, twice)

- JULIA

- KUMA

- PANDA

- BRYAN

- HEIHACHI

- OGRE

- MOKUJIN

- GUN JACK

- GON

- ANNA

- DOCTOR.B.

- TRUE OGRE

- CROW

- FALCON

- HAWK

- OWL

To figure more of these out, or the ones you may need, try using the LBA debug cheat to see which ones are being loaded.

## NOTES

- Swapping character model file works, but you will have broken textures until you replace the arc (or repack the arc with new textures)

- Swapped characters will not change behavior, they will retain the original behavior (e.g. swapping Paul's model with Law's will only affect visuals and sounds, but not animations!)

- Swapping stages is a similar story, you must replace both the model and the texture arcs, otherwise you end up with messed up textures

- Files can indeed be swapped by swapping their LBA positions in memory!

- Animations seem to be exactly the same as the arcade build, so characters with separate fingers still animate properly!

- To view textures, use a tool like TIMViewer by rveach

- To extract full VABs, you must extract the VB out of the arc and append it to the VH

- In case you don't want ot extract full VABs but still want to listen to sounds, use a tool like PSound

## Arc format notes

This is a format that was used since Tekken 1. It is unknown whether or not it was also used in the arcade builds, but it appears it may be (according to memory dumps at least)

It is a very simple format to figure out and follow:

```
uint32_t entrycount;
then for each entry...
uint32_t offset;
uint32_t size;
...then the data starts
```
