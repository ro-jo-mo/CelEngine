# To do

# PRIORITY

- Update schedulers
    - Single
    - Group scheduling
    - Fix chains
- Organise public / private headers (currently all public)

# Later

- Generating entity sets for queries can be optimised
    - With<const X, const Y> is the same set of entities as With<X, Y>
    - The set of entities used for With<X, Y> can be used as a base for calculating With<X, Y>, Without<Z>
- Improve resource manager so its less dumb i.e. probably remove the resource wrapper or alias unique pointer ...
- Update fixed assumptions in types.h
- Switch to snake case
- Queue system registry instead of instantly registering on add sys
- Clean up public headers
- Rendering
- Physics
- ...
- cmake --install cmake-build-debug --prefix install
- 
