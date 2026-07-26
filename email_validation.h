#ifndef EMAIL_VALIDATION_INCLUDED
#define EMAIL_VALIDATION_INCLUDED
/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA */

#include <stddef.h>
#include <string>

namespace email
{

struct Parts
{
  size_t at;
};

/*
  Validate an RFC 5321/5322 addr-spec suitable for use as a mailbox key.
  ASCII dot-atom and quoted local-parts are supported. Domain names and
  IPv4/IPv6 address literals are accepted; comments, display names, folding
  whitespace and SMTPUTF8 are deliberately not part of the EMAIL type.
*/
bool validate(const char *value, size_t length, Parts *parts= nullptr);

/* Preserve the case-sensitive local-part and lowercase the domain. */
bool normalize(const char *value, size_t length, std::string *result);

} // namespace email

#endif
