Semantically, I just want to use unordered map (or unordered set.)
When N is large & lookup is done many times, most people would choose unordered map.
But how large is really large? How many times is many times?
Is there a big trade off if I use unordered map when N is small and/or lookup is done only few times?

Experiment plan:

Key type:
 * integer key
 * 32 bit machine  8 chars, 10 chars for short string
                  11 chars, 23 chars for string
 * 64 bit machine  16 chars 22 chars for short string
                  23 chars, 32 chars for long string

N:
 10, 100, 1000, 10000, 100000

# of instance of container:
 16

Size of value:
 4, 16, 64, 256 bytes

Construction
 * Construct container 1 with N elements, container 2 with N elements, ... , container 16 with N elements.
 * Add an element to container 1, container 2, ... , container 16. Repeat N.

Lookup
 * Look up 100000 times for container 1, 100000 times for container 2, ...
 * Look up once in container 1, in container 2, ... container 16, repeat 100000 times

Other assumption:
 Guaranteed to be found.

Candidates
 vector, map, unordered_map,
 stretch 1: flat_map
 stretch 2: QMap w/QString
 stretch 3: CMap w/CString
 stretch 4: Java-HashMap
 stretch 5: C#-Dictionary
 stretch 6: JavaScript-Map(https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map)
