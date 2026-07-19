Communication between the Engine executable and the Game DLL crosses the ABI (Application Binary Interface).<br> 
Crossing the ABI comes with certain risks, such as the same types (classes) possibly being compiled with different memory layouts,<br>and thus being incompatible when data is passed from one side to another.<br>
To mitigate this, the engine uses a boundary pattern; the Abstract Interface Pattern.<br>
For the engine to call functions on objects which were created in game memory, it must use an abstract interface.<br>
IGame and INode are abstract interfaces created for this purpose.<br>
Interface functions must **only pass stable datatypes** (simple POD type, not complex ones e.g. std::vector).<br>
