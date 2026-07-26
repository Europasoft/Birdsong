## Engine-game interop
### Game loading
On startup, the engine application loads the game from a DLL (Dynamic Link Library, AKA a Shared Object).<br>
Loading is done explicitly, after the engine application has already been initialized.<br>
The engine must be able to find this DLL file easily. For that reason, the working directory (folder path where the engine is started) **must** be exactly where the DLL file is located. 

### Execution flow
<pre>
[Engine]                        │       │   [Game]
                                │  ABI  │
Engine starts                   │       │
Loads the game DLL              │       │
Retrieves factory function      │       │
Calls factory function ─────────┤───────├───▶ Creates Game object
                           ◄────┤───────├──── Passes IGame* pointer back
                                │       │
Calls IGame::onLoadCall ────────┤───────├───▶ Game::onLoad runs
                                │       │
Passes IEngine* pointer ────────┤───────├───▶
Some function runs ◄────────────┤───────├──── Calls some function on IEngine*
                                │       │
Calls some function on IGame* ──┤───────├───▶ Some function runs
                                │       │
Calls IGame::onTickCall ────────┤───────├───▶ Game::tick runs
...                             │       │
Calls IGame::onUnloadCall ──────┤───────├───▶ Game::onUnload runs 
                                │       │     Destroys Game object
</pre>

### ABI safety
Communication between the Engine executable and the Game DLL crosses the ABI (Application Binary Interface) **boundary**.<br> 
Crossing the boundary comes with certain risks, such as the same types (classes) possibly being compiled differently.<br>Such types may be incompatible if data is passed from one side to another, the exact same declaration can produce two different memory layouts.<br>
To mitigate this, the engine uses a boundary pattern: the Abstract Interface Pattern.<br>
For the engine to call functions on objects which were **created in game memory**, it must use an **abstract interface**.<br>
The same applies in the opposite direction, when the game calls an engine object.<br>
`IGame`, `INode` and `IEngine` are abstract interfaces created for this purpose.<br>
Interface functions must **only pass stable datatypes** (simple POD, not complex ones e.g. std::vector).<br>

Safety is guranteed only if
1. All functions of the interface are **pure virtual**.
2. They use the correct calling convention (`DLL_CALL`/`__cdecl`).
3. They only accept and return safe types.
4. The interface has no data members.
<br>

Objects created on one side of the boundary should never be deleted on the other side.<br>
**Protected destructors** are used to prevent deletion of the object through a pointer to the interface.<br>
The derived class (where the actual function implementations live) can have a public destructor.<br>

### Available interfaces
| Interface | Call direction | Owner | Implementation class |
| --- | --- | --- | --- |
| `IGame` | Engine >> Game | Game | `Game` |
| `INode` | Engine >> Game | Game | `Node` |
| `IEngine` | Game >> Engine | Engine | `IEngineImpl` |

The "implementation class" for an abstract interface is a class derived from the interface, where the pure virtual functions are actually defined.<br>
These function definitions should be marked with `DLL_CALL` and `final override`.<br>

### Code separation
Because an implementation class contains function logic that is only safe to execute on the owner's side, these files are separated to avoid `#include` mistakes:<br>

| Directory (folder) | Included by | Info |
| --- | --- | --- |
| `src/core/include/shared/` | Game + Engine | These are files which both the engine and game may include. |
| `src/core/engine/interop/` | Engine | These files contain logic that will run directly in the engine application. Do not include these in game code. |
| `src/core/include/game/` | Game | These files contain logic that will run inside the game DLL. Do not include any headers here in engine code. The source files here are compiled ONLY into the game DLL. They are conceptually part of the engine, but they do not contribute to the final application when the engine is built. |

### Node lifetime
A physical (or just visual) object is called a "Node". Nodes are usually created by the game, they are exposed to the engine only through the INode interface.<br>
As nodes are not movable (in memory), it is safe for the engine to keep a list of pointers to the nodes. They will remain valid up until the nodes are destroyed.<br>
When a Node is spawned, the Node itself notifies the engine. Likewise, the node notifies when it is about to be destroyed, so that the engine can safely stop using that Node's memory address.

<pre>
[Game]                        │       │   [Engine]
                              │  ABI  │
Game::spawnNode               │       │
Node constructor runs         │       │
Calls IEngine::registerNode ──┤───────├───▶ IEngineImpl::registerNode runs
                              │       │     Adds INode* to internal registry
Runs Node::onSpawn            │       │
                              │       │
Node::tick runs ◄─────────────┤───────├──── Calls INode::tickCall
...                           │       │                              
~Node destructor runs         │       │
IEngine::unregisterNode ──────┤───────├───▶ IEngineImpl::unregisterNode
                              │       │     Removes INode* from internal registry
</pre>



