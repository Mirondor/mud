// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef SPAM_HPP_INCLUDED
#define SPAM_HPP_INCLUDED

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"

namespace antispam {

enum { kOfftopMode };
bool check(CharData *ch, int mode);

} // antispam


#endif // SPAM_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
