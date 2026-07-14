- After a b3cpp::World gets destroyed, all b3cpp::Body objects created from it will be unsafe to use. Do not keep references to a Body after its world is destroyed.

- An object of the b3cpp::World type will tear down its physics state when it goes out of scope, keep a reference or smart pointer to make the world stay active.

- After creating a b3cpp::Body it must be kept in memory, the lifetime of the internal physics body is directly coupled to the lifetime of the b3cpp::Body object.

- References to b3cpp::Shape objects are non-owning. The shapes themselves are owned by the Body they are attached to, meaning you may safely "forget" a shape after adding it to a body (and calling Shape::activate()).

- A b3cpp::Shape only lives as long as the Body it as attached to.