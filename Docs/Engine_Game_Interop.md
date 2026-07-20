## Engine-game interop
### Game loading
On startup, the engine application loads the game from a DLL (Dynamic Link Library, AKA a Shared Object).<br>
Loading is done explicitly, after the engine application has already been initialized.<br>
The engine must be able to find this DLL file easily. For that reason, the working directory (folder path where the engine is started) **must** be exactly where the DLL file is located. 

### Execution flow
```text
[Engine]                       │       │   [Game]
                               │  ABI  │
Engine starts                  │       │
Loads the game DLL             │       │
Retrieves factory function     │       │
Calls factory function ────────┤───────┤───▶ Creates Game object
                           ◀───├───────├──── Passes IGame* pointer back
                               │       │
Calls OnLoad ──────────────────┤───────┤───▶ OnLoad runs
                               │       │
Passes IEngine* pointer ───────┤───────┤───▶
Some function runs ◀───────────├───────├──── Calls some function on IEngine
                               │       │
Calls some function on IGame ──┤───────┤───▶ Some function runs
                               │       │
Calls tick ────────────────────┤───────┤───▶ Tick runs
...                            │       │
Calls onUnload ────────────────┤───────┤───▶ onUnload runs 
                               │       │     Destroys Game object
```

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
| `IEngine` | Game >> Engine | Engine | `EngineInterface` |

The "implementation class" for an abstract interface is a class derived from the interface, where the pure virtual functions are actually defined.<br>
These function definitions should be marked with `DLL_CALL` and `final override`.<br>

### Code separation
Because an implementation class contains function logic that is only safe to execute on the owner's side, these files are separated to avoid `#include` mistakes:<br>

| Directory (folder) | Included by | Info |
| --- | --- | --- |
| src/core/include/shared/ | Game + Engine | These are shared files that both the engine and game may include. |
| src/core/include/game/ | Game only | These files contain logic that will run inside the game DLL. Do not include these in engine code. |
| src/core/engine/interop/ | Engine only | These files contain logic that will run directly in the engine application. Do not include these in game code. |


