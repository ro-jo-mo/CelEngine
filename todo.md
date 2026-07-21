# To do

# PRIORITY

- Organise public / private headers (currently all public)
- Asynchronous asset loading
- Split up world & commands
- Move to Query<Get<a,b,c>,With<d,e,f>,Without<x,y,z>>
- System allocation can cheaply re run on run
    - More flexible, currently a system can only be added once all resources it needs exist
- Alter commands to stored type index instead of using polymorphism (also update component arrays to support this)
- Rework world to allow for arbitrary queries
    - World should block all parallel systems

# Later

- Enforce query data being accessed as references
- When several systems are run sequentially, this can be tasked to a single thread
- Might merge immediate submit data into vk context
- Update default textures for materials
- Optional components
- Generating entity sets for queries can be optimised
    - With<const X, const Y> is the same set of entities as With<X, Y>
    - The set of entities used for With<X, Y> can be used as a base for calculating With<X, Y>, Without<Z>
- Multithreading
    - With<X, Y, Z> can run at the same time as With<X, Y> Without<Z,>
- Improve resource manager so its less dumb i.e. probably remove the resource wrapper or alias unique pointer ...
- Clean up public headers
- Rendering
- Physics
- ...
- cmake --install cmake-build-debug --prefix install
- 
