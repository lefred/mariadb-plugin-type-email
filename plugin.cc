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


/* MariaDB EMAIL data type and companion functions. */
#define MYSQL_SERVER
#include "mariadb.h"
#include "sql_class.h"
#include "sql_type_email.h"
#include "email_functions.h"
#include <mysql/plugin_data_type.h>

static struct st_mariadb_data_type plugin_descriptor_email=
{
  MariaDB_DATA_TYPE_INTERFACE_VERSION,
  &type_handler_email
};

#define EMAIL_PLUGIN_ENTRY(TYPE, DESCRIPTOR, NAME, DESCRIPTION) \
{                                                               \
  TYPE, DESCRIPTOR, NAME, "lefred", DESCRIPTION,      \
  PLUGIN_LICENSE_GPL, 0, 0, 0x0100, NULL, NULL, "0.2.0",        \
  MariaDB_PLUGIN_MATURITY_BETA                                   \
}

maria_declare_plugin(type_email)
  EMAIL_PLUGIN_ENTRY(MariaDB_DATA_TYPE_PLUGIN, &plugin_descriptor_email,
                     "email", "Validated EMAIL data type"),
  EMAIL_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_email_is_valid,
                     "email_is_valid", "Validate an email address"),
  EMAIL_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_email_local_part,
                     "email_local_part", "Return the local part of an email"),
  EMAIL_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_email_domain,
                     "email_domain", "Return the domain of an email"),
  EMAIL_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_email_normalize,
                     "email_normalize", "Lowercase an email domain")
maria_declare_plugin_end;
