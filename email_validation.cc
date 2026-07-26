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


#include "email_validation.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cstring>

namespace email
{
namespace
{

bool is_atext(unsigned char c)
{
  return std::isalnum(c) || std::strchr("!#$%&'*+-/=?^_`{|}~", c);
}

bool valid_dot_atom(const char *s, size_t n)
{
  if (!n || s[0] == '.' || s[n - 1] == '.')
    return false;
  bool previous_dot= false;
  for (size_t i= 0; i < n; ++i)
  {
    const unsigned char c= static_cast<unsigned char>(s[i]);
    if (c == '.')
    {
      if (previous_dot)
        return false;
      previous_dot= true;
    }
    else
    {
      if (c > 0x7f || !is_atext(c))
        return false;
      previous_dot= false;
    }
  }
  return true;
}

bool valid_quoted(const char *s, size_t n)
{
  if (n < 2 || s[0] != '"' || s[n - 1] != '"')
    return false;
  for (size_t i= 1; i + 1 < n; ++i)
  {
    const unsigned char c= static_cast<unsigned char>(s[i]);
    if (c == '\\')
    {
      if (++i + 1 >= n)
        return false;
      const unsigned char escaped= static_cast<unsigned char>(s[i]);
      if (escaped < 32 || escaped > 126)
        return false;
    }
    else if (c == '"' || c == '\r' || c == '\n' || c < 32 || c > 126)
      return false;
  }
  return true;
}

bool valid_domain_name(const char *s, size_t n)
{
  if (!n || n > 253 || s[0] == '.' || s[n - 1] == '.')
    return false;
  size_t label_start= 0;
  for (size_t i= 0; i <= n; ++i)
  {
    if (i != n && s[i] != '.')
    {
      const unsigned char c= static_cast<unsigned char>(s[i]);
      if (c > 0x7f || !(std::isalnum(c) || c == '-'))
        return false;
      continue;
    }
    const size_t label_length= i - label_start;
    if (!label_length || label_length > 63 ||
        s[label_start] == '-' || s[i - 1] == '-')
      return false;
    label_start= i + 1;
  }
  return true;
}

bool valid_address_literal(const char *s, size_t n)
{
  if (n < 3 || s[0] != '[' || s[n - 1] != ']')
    return false;
  std::string literal(s + 1, n - 2);
  unsigned char buffer[sizeof(struct in6_addr)];
  if (literal.size() > 5 &&
      std::equal(literal.begin(), literal.begin() + 5, "IPv6:",
                 [](char a, char b) {
                   return std::tolower(static_cast<unsigned char>(a)) ==
                          std::tolower(static_cast<unsigned char>(b));
                 }))
    return inet_pton(AF_INET6, literal.c_str() + 5, buffer) == 1;
  return inet_pton(AF_INET, literal.c_str(), buffer) == 1;
}

} // namespace

bool validate(const char *value, size_t length, Parts *parts)
{
  if (!value || length < 3 || length > 254)
    return false;

  size_t at= length;
  bool quoted= false;
  bool escaped= false;
  for (size_t i= 0; i < length; ++i)
  {
    const unsigned char c= static_cast<unsigned char>(value[i]);
    if (!c || c > 0x7f)
      return false;
    if (escaped)
    {
      escaped= false;
      continue;
    }
    if (quoted && c == '\\')
    {
      escaped= true;
      continue;
    }
    if (c == '"')
      quoted= !quoted;
    else if (c == '@' && !quoted)
    {
      if (at != length)
        return false;
      at= i;
    }
  }
  if (quoted || escaped || at == length || at == 0 || at > 64 ||
      at + 1 == length)
    return false;

  const bool local_ok= value[0] == '"' ?
                         valid_quoted(value, at) :
                         valid_dot_atom(value, at);
  const char *domain= value + at + 1;
  const size_t domain_length= length - at - 1;
  if (!local_ok ||
      !(valid_domain_name(domain, domain_length) ||
        valid_address_literal(domain, domain_length)))
    return false;

  if (parts)
    parts->at= at;
  return true;
}

bool normalize(const char *value, size_t length, std::string *result)
{
  Parts parts;
  if (!result || !validate(value, length, &parts))
    return false;
  result->assign(value, length);
  std::transform(result->begin() + parts.at + 1, result->end(),
                 result->begin() + parts.at + 1,
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return true;
}

} // namespace email
