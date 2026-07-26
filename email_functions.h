#ifndef EMAIL_FUNCTIONS_INCLUDED
#define EMAIL_FUNCTIONS_INCLUDED

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


#include <mysql/plugin_function.h>

extern Plugin_function plugin_descriptor_email_is_valid;
extern Plugin_function plugin_descriptor_email_local_part;
extern Plugin_function plugin_descriptor_email_domain;
extern Plugin_function plugin_descriptor_email_normalize;

#endif
