#pragma once

namespace Cel {

// Used for std::visit
// e.g. std::visit(Overload {[](...){...},...};
template<class... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};

}