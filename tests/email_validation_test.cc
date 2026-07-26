/* Copyright (c) 2026 lefred (Frédéric Descamps) */

#include "../email_validation.h"

#include <cassert>
#include <string>

int main()
{
  const char *valid[]=
  {
    "alice@example.com",
    "first.last+tag@sub.example.com",
    "\"quoted local\"@example.com",
    "postmaster@[127.0.0.1]",
    "root@[IPv6:2001:db8::1]"
  };
  for (const char *value : valid)
    assert(email::validate(value, std::char_traits<char>::length(value)));

  const char *invalid[]=
  {
    "", "plain-address", "@example.com", "alice@", "a@@example.com",
    ".alice@example.com", "alice..smith@example.com",
    "alice@example..com", "alice@-example.com", "alice@example-.com",
    "alice example@example.com", "alice@exa_mple.com",
    "alice@[999.1.1.1]", "álîçé@example.com"
  };
  for (const char *value : invalid)
    assert(!email::validate(value, std::char_traits<char>::length(value)));

  std::string normalized;
  assert(email::normalize("Case.Sensitive@EXAMPLE.COM", 26, &normalized));
  assert(normalized == "Case.Sensitive@example.com");
}
