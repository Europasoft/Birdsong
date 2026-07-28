
## Transform to model matrix
A transform represents a Node's (3D object) position, rotation, and scale.<br>
Positions are relative to the Node's "home sector", and the coordinates for the home Sector is also included in the transform.<br>
After the transform is set, either by the game or by the engine, it must be pushed to the global instance buffer for rendering.<br>
On the engine side, transforms are stored in `EngineNodeData` objects, alongside the corresponding `INode*` which points to the game-side Node object. These data pairs exist in the `Sector` instance to which the Node belongs, depending on where it exists in 3D space.<br>
In `Scene::updateInstanceData` each transform is converted into a matrix, and saved to the **instance buffer**.<br>
When the draw call is dispatched, the shader is supplied with a pointer into this buffer (device address), and the offset (instance ID) which is used to find the correct data for the specific Node.<br>
Finally, the shader applies this data to the Node's mesh geometry, placing it visually in the right spot relative to the camera.<br>

## Physics updates
The physics engine may move and rotate Nodes, therefore, the engine-side transform (`EngineNodeData::engineTransform`) is often updated by the engine directly.<br>
The game may also change a Node's transform, in which case a "teleported" flag is set, signaling to the engine that it should overwrite its own transform with the game transform.<br>
The engine periodically uses the INode interface to call `Node::getTransform` which fetches the updated game transform.<br>
> Note that the game may only modify transforms during events fired by the engine (such as `Node::tick`) in order to prevent race conditions between threads.

## Per-frame sequence
1. Tick game
2. Pull transform updates from game
3. Sector system update (move nodes between sectors when needed)
4. Apply transform updates to physics engine (if any changed)
5. Simulate physics for one step
6. Update instance data for rendering
7. Render frame
8. Push updated transforms to the game (for next frame)
