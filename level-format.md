# File format of levels
Untangle levels are stored in an IFF container of type `UNTG`. All multibyte numbers are big endian. Level FORM contains two chunks:
* `DOTS` chunk containing coordinates of dots.
* `LINE` chunk containing indexes of start dot and end dot of each line.
  
`DOTS` chunk **must** be placed before `LINE` chunk. Number of lines and dots in a level is calculated from chunk lengths.

## DOTS
The chunk stores coordinates of dots in vitrual coordinates system. The system is a square of 32768 by 32768 units (indexed from 0 to 32767). (0, 0) point is the upper left corner. Each dot is stored as a record:
```
int16 x;
int16 y;
```
Each dot record occupies 4 bytes in a file. Negative coordinates are not allowed.

## LINES
The chunk stores lines defined by index of start and end dot. Each index is an unsigned byte and corresponds to position of the dot record in `DOTS` chunk, starting from 0.
```
uint8 start;
uint8 end;
```
It does not matter which dot is specified as line start and which as line end. No specific ordering of lines is required or assumed. Indexes higher or equal to number of dots are not allowed. Each line record occupies 2 bytes in a file.

## Limits
Format of `LINE` chunk limits number of dots to 256.

## Remarks
The format itself does not exclude duplicated dots or duplicated lines. From the point of view of the game, duplicated dots (dots with the same coordinates) are OK in harder levels. Duplicated lines (joining the same two dots) should never appear, as it makes a level unsolvable. Check for duplicated lines should be probably implemented in the level loader.

## Sets of levels
A set of level is stored as IFF LIST file. Payload of the LIST is just a concatenation of FORM-s described above.
