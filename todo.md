# To do

# PRIORITY

- Update schedulers
    - Single
    - Group scheduling
    - Fix chains
- Introduce custom schedules for Ai tick and such
- Organise public / private headers (currently all public)
- Asynchronous asset loading
- Split up world & commands
- Move to Query<Get<a,b,c>,With<d,e,f>,Without<x,y,z>>
- Update Time resource to be less silly

# Later

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
- Update fixed assumptions in types.h
- Switch to snake case
- Queue system registry instead of instantly registering on add sys (why???)
- Clean up public headers
- Rendering
- Physics
- ...
- cmake --install cmake-build-debug --prefix install
- 
