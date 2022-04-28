#ifndef _____LLD_H_____
#define _____LLD_H_____

#include "lld/Common/Driver.h"

namespace mk
{
namespace lld
{
namespace elf
{
struct ScopedLink
{
    template <typename... Ts>
    bool operator()(Ts &&... ts) const
    {
        return ::lld::elf::link(std::forward<Ts>(ts)...);
    }

    ~ScopedLink() { ::lld::CommonLinkerContext::destroy(); }
};
}  // namespace elf
}  // namespace lld
}  // namespace mk

#endif