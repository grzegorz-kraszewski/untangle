# How to check if the objective of the game is achieved?

The objective seems to be easy to define. No two lines should intersect. Soution for the problem of checking intersection of two line segments is well known. It requires 8 multiplications
and about 20 additions, subtractions and comparisions. How many checks has to be performed?

Let's assume a level has 40 dots and on average 5 lines meet in each dot. It gives 100 lines total. If one checks intersection of each line with each, it yields a square matrix of 10 000 elements.
It is obvious that it makes no sense to check intersection of a line with itself. Also relation "A intersects B" is alternating, so it implies "B intersects A". We can skip the matrix diagonal
and whole upper (or lower) triangle submatrix. 4 950 checks left.

In the next step we should observe that the definition of intersection in our game differs from strict mathematic one. The latter says that two line segments intersect if they are at least one
common point. So, by strict definition, segments meeting at a dot intersect. By the game definition – they don't. Unless they lay one on another. This check is faster, only 2 multiplications. How many times
we can use this short path? On average 8 times for each line, but half of them is in the skipped triangle of the matrix, so 400 cases out of 4 950. Still not good.

The best optimization comes from another dimension: the time. Assume that at given moment in the game we know exactly which line segments are intersected by which. Then users moves a dot, so all segments
ending at the dot are also moved. Then we need to recheck intersections **of moved lines only** and update the matrix. In our example it will be on average 5 × 95 = 475 checks.

## Storage of intersection data

In any case full data of intersections have to be stored. I have decided to keep only lower triangle of the square matrix. In theory one bit per intersection is enough, at a cost of more complicated access.
On the other hand if a byte per intersection is used, our example level needs 4950 bytes. Not much even for plain Amiga 500. If a level has *n* lines, (*n*² - *n*) / 2 bytes have to be allocated. Therefore I decided
to use a byte per intersection. In this area rows of the triangle are stored one by one, without any padding. Indexing the structure is easy. I assume that lines in the game level are indexed starting from 0. Then, to
access information byte of intersection between line *a* and line *b* (where *a* < *b*), one accesses byte \[*b* · (*b* - 1) ÷ 2 + *a*\].
