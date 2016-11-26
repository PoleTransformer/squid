/*
 * Copyright (C) 1996-2016 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "base/RegexPattern.h"
#include <utility>

RegexPattern::RegexPattern(const std::regex_constants::syntax_option_type &aFlags, const char *aPattern) :
        flags(aFlags),
        pattern(xstrdup(aPattern)),
        regex(pattern, flags)
{}

RegexPattern &
RegexPattern::operator =(RegexPattern &&o)
{
    flags = std::move(o.flags);
    regex = std::move(o.regex);
    pattern = o.pattern;
    o.pattern = nullptr;
    return *this;
}

