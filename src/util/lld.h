#ifndef _____LLD_H_____
#define _____LLD_H_____

#include "lld/Common/Driver.h"

#include <vector>

namespace mk
{
namespace lld
{
namespace elf
{
struct ScopedLink
{
    bool operator()(const std::vector<std::string> & args) const
    {
        std::string out, err;
        llvm::raw_string_ostream out_stream(out), err_stream(err);
        std::vector<const char *> lld_args;
        lld_args.reserve(args.size());
        for (const auto & arg : args)
            lld_args.emplace_back(arg.data());
        return ::lld::elf::link(lld_args, out_stream, err_stream, false, false);
    }

    ~ScopedLink() { ::lld::CommonLinkerContext::destroy(); }
};
}  // namespace elf
}  // namespace lld
}  // namespace mk

#endif