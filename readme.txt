When I learned about hash_map in Visual Studio 2012, I was delighted.
I started using hash_map (later, unordered_map) instead of map almost always unless I care about the order.
I thought that was no brainer.


But one day my boss convinced us that vector performs better if N is smaller.
Because vector is cheap to construct and it's also quick to lookup (thanks to L1, L2... whatever Ln caches.)


I get it, but I still prefered map or unorder_map, just because they are semantically clear and find syntax is natural.
And what number is "small" really?


So... I decided to run various test and here is the result!
http://htmlpreview.github.io/?https://github.com/masakotoda/pick-a-container/blob/master/pick.a.container.rpt/compare.html


Some of the results seem quite odd. Maybe I have errors in my code and/or my computer got slow for some reasons. But it's kinda fun to look at a lot of graphs :-)



Container:
 * vector
 * map
 * unordered_map
 * boost::flat_map

Key type:
 * integer key
 * 10 chars (short string)
 * 32 chars (long string)

N (# of elements):
 10, 100, 1000, 10,000

# of instance of container:
 10

Size of value:
 4, 16, 64, 256 bytes

Construction
 * "All at once": Construct container 1 with N elements, container 2 with N elements, ... , container 10 with N elements.
 * "Sporadic": Add an element to container 1, container 2, ... , container 10. Repeat N.

Lookup
 * "Continuous": Lookup 100,000 times in container 1, 100,000 times in container 2, ..., 100,000 times in container 10.
 * "Sporadic": Lookup once in container 1, in container 2, ... container 10, repeat 100,000 times

Other assumption:
 * Guaranteed to be found.
 * To simulate somewhat realistic application, allocate extra memory here and there, access to it here and there.
 * The unit is microseconds.
 * The spec of test PC: i7-6700HQ 2.6GHz, 16GB.

Wish list
 * Concurrency
 * QMap w/QString
 * CMap w/CString
 * Java-HashMap, Java-Map
 * C#-Dictionary, C#-SortedDictionary
 * JavaScript-Map(https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map)
