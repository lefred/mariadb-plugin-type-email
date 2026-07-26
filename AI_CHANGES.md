# AI Changes

Changes made by Claude (model: `claude-sonnet-5`) via Claude Code.

## 2026-07-26

Follow-up to a code review of the plugin, requested by the user. Two issues
were fixed; a third candidate issue was investigated and left unchanged
after comparison with MariaDB's own upstream `type_xmltype` plugin (which
this plugin's structure is modeled on).

- **Removed dead code**: `Field_email::report_wrong_value()` was declared
  (`sql_type_email.h`) and defined (`sql_type_email.cc`) but never called
  from anywhere, including `Field_email::store()`. Confirmed the same method
  is equally unused in MariaDB's upstream `type_xmltype` plugin, so this is
  inherited dead code rather than an intentional-but-unwired feature.
  Removed both the declaration and the definition.

- **Removed redundant double validation in `EMAIL_NORMALIZE()`**:
  `Item_func_email_string::val_str()` (`email_functions.cc`) called
  `email::validate()` up front to obtain `Parts`, then for the `NORMALIZED`
  component called `email::normalize()`, which internally calls
  `email::validate()` again. Every `EMAIL_NORMALIZE()` call validated its
  input twice. Restructured so the `NORMALIZED` branch is handled before
  the explicit `validate()` call and relies solely on the validation done
  inside `email::normalize()`; the `LOCAL_PART`/`DOMAIN` branches still call
  `email::validate()` directly since they need the resulting `Parts`.

- **Investigated, left unchanged**: `Field_email::store()` writes a
  syntactically-valid placeholder (`"invalid@invalid"`) into the record
  buffer after already raising `ER_WRONG_VALUE` for a `NOT NULL` column.
  This looked like pointless work after a hard error, but it's a deliberate
  defense-in-depth pattern copied verbatim from MariaDB's own
  `type_xmltype` plugin (`Field_xmltype::store()`), which does the same
  thing with `"<invalid_xml_replaced />"`. Left as-is to stay consistent
  with the upstream convention.

Verified with the standalone validator test after each change:

```sh
c++ -std=c++17 -Wall -Wextra -pedantic \
  email_validation.cc tests/email_validation_test.cc -o /tmp/email-test
/tmp/email-test
```
