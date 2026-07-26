# mariadb-plugin-type-email

![mariabd-plugin-type-email](logo/email_type.png)

This server plugin adds a native `EMAIL` column type with validation at every
storage and cast boundary.

```sql
CREATE TABLE contacts (
  id BIGINT UNSIGNED PRIMARY KEY,
  address EMAIL NOT NULL
);

INSERT INTO contacts VALUES (1, 'Case.Sensitive@EXAMPLE.COM');

SELECT EMAIL_LOCAL_PART(address), EMAIL_DOMAIN(address),
       EMAIL_NORMALIZE(address)
FROM contacts;
```

The plugin also registers:

- `EMAIL_IS_VALID(value)` — `1` or `0`; returns `NULL` for SQL `NULL`.
- `EMAIL_LOCAL_PART(value)` — the portion before `@`, or `NULL` if invalid.
- `EMAIL_DOMAIN(value)` — the portion after `@`, or `NULL` if invalid.
- `EMAIL_NORMALIZE(value)` — preserves the local-part and lowercases the
  case-insensitive domain, or returns `NULL` if invalid.

## Validation policy

`EMAIL` accepts ASCII RFC-style addr-spec values:

- dot-atom and quoted local-parts, up to 64 octets;
- DNS domains with labels up to 63 octets;
- IPv4 (`[192.0.2.1]`) and IPv6 (`[IPv6:2001:db8::1]`) literals;
- total length up to 254 octets.

Display names, comments, whitespace folding, multiple recipients, bare local
addresses, and SMTPUTF8 addresses are rejected. This deliberately defines a
stable mailbox data type instead of trying to parse complete mail headers.
Domain existence and deliverability require DNS/SMTP checks and are outside
database validation.

## Build

This plugin uses MariaDB's internal data-type API and must be compiled as part
of a MariaDB server source tree (tested with MariaDB 13.1):

```sh
ln -s /path/to/mariadb-plugin-type-email \
  /path/to/MariaDB-server/plugin/type_email
cmake -S /path/to/MariaDB-server -B build
cmake --build build --target mariadbd
```

## Instalation

Load the plugin (it's possible to modify the `CMakeLists.txt` to load it a start and make it mandatory.

```sql
install soname 'type_email';
```

Verify that the data type and the functions are registered:

```sql
SELECT plugin_name, plugin_type, plugin_library, plugin_description,
plugin_author FROM information_schema.PLUGINS WHERE plugin_library LIKE 'type_email.so';
+------------------+-------------+----------------+-----------------------------------+---------------+
| plugin_name      | plugin_type | plugin_library | plugin_description                | plugin_author |
+------------------+-------------+----------------+-----------------------------------+---------------+
| email            | DATA TYPE   | type_email.so  | Validated EMAIL data type         | lefred        |
| email_is_valid   | FUNCTION    | type_email.so  | Validate an email address         | lefred        |
| email_local_part | FUNCTION    | type_email.so  | Return the local part of an email | lefred        |
| email_domain     | FUNCTION    | type_email.so  | Return the domain of an email     | lefred        |
| email_normalize  | FUNCTION    | type_email.so  | Lowercase an email domain         | lefred        |
+------------------+-------------+----------------+-----------------------------------+---------------+
5 rows in set (0.002 sec)
```

Run the standalone validator tests with:

```sh
c++ -std=c++17 -Wall -Wextra -pedantic \
  email_validation.cc tests/email_validation_test.cc -o /tmp/email-test
/tmp/email-test
```
