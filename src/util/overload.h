#ifndef __OVERLOAD_H__
#define __OVERLOAD_H__

namespace mk
{
namespace util
{
template <typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;

    template <typename... Us>
    Overload(Us &&... us) : Ts(std::forward<Us>(us))...
    {}
};

template <typename... Ts>
Overload(Ts &&...)
    -> Overload<std::remove_const_t<std::remove_reference_t<Ts>>...>;
}  // namespace util
}  // namespace mk

#endif