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

#define MYSQL_SERVER
#include "mariadb.h"
#include "sql_class.h"
#include "item_func.h"
#include "email_functions.h"
#include "email_validation.h"

class Item_func_email_is_valid : public Item_bool_func
{
public:
  Item_func_email_is_valid(THD *thd, Item *arg): Item_bool_func(thd, arg) {}
  bool val_bool() override
  {
    StringBuffer<256> buffer;
    String *value= args[0]->val_str(&buffer);
    if (!value)
    {
      null_value= true;
      return false;
    }
    null_value= false;
    return email::validate(value->ptr(), value->length());
  }
  LEX_CSTRING func_name_cstring() const override
  {
    return "email_is_valid"_LEX_CSTRING;
  }
  Item *shallow_copy(THD *thd) const override
  {
    return get_item_copy<Item_func_email_is_valid>(thd, this);
  }
};

enum Email_component { LOCAL_PART, DOMAIN, NORMALIZED };

class Item_func_email_string : public Item_str_func
{
  Email_component component;
  LEX_CSTRING function_name;
public:
  Item_func_email_string(THD *thd, Item *arg, Email_component component_arg,
                         LEX_CSTRING name_arg)
    : Item_str_func(thd, arg), component(component_arg), function_name(name_arg)
  {}

  bool fix_length_and_dec(THD *thd) override
  {
    collation.set(args[0]->collation);
    max_length= component == LOCAL_PART ? 64 :
                component == DOMAIN ? 253 : 254;
    set_maybe_null();
    return false;
  }

  String *val_str(String *result) override
  {
    StringBuffer<256> buffer;
    String *value= args[0]->val_str(&buffer);
    if (!value)
    {
      null_value= true;
      return nullptr;
    }

    if (component == NORMALIZED)
    {
      std::string normalized;
      if (!email::normalize(value->ptr(), value->length(), &normalized) ||
          result->copy(normalized.data(), normalized.size(), value->charset()))
      {
        null_value= true;
        return nullptr;
      }
      null_value= false;
      return result;
    }

    email::Parts parts;
    if (!email::validate(value->ptr(), value->length(), &parts))
    {
      null_value= true;
      return nullptr;
    }

    null_value= false;
    const size_t offset= component == LOCAL_PART ? 0 : parts.at + 1;
    const size_t length= component == LOCAL_PART ?
                           parts.at : value->length() - offset;
    if (result->copy(value->ptr() + offset, length, value->charset()))
    {
      null_value= true;
      return nullptr;
    }
    return result;
  }

  LEX_CSTRING func_name_cstring() const override { return function_name; }
  Item *shallow_copy(THD *thd) const override
  {
    return get_item_copy<Item_func_email_string>(thd, this);
  }
};

template <class Item_type>
class Create_email_func_arg1 : public Create_func_arg1
{
public:
  Item *create_1_arg(THD *thd, Item *arg) override
  {
    return new (thd->mem_root) Item_type(thd, arg);
  }
};

class Item_func_email_local_part : public Item_func_email_string
{
public:
  Item_func_email_local_part(THD *thd, Item *arg)
    : Item_func_email_string(thd, arg, LOCAL_PART,
                             "email_local_part"_LEX_CSTRING) {}
};

class Item_func_email_domain : public Item_func_email_string
{
public:
  Item_func_email_domain(THD *thd, Item *arg)
    : Item_func_email_string(thd, arg, DOMAIN,
                             "email_domain"_LEX_CSTRING) {}
};

class Item_func_email_normalize : public Item_func_email_string
{
public:
  Item_func_email_normalize(THD *thd, Item *arg)
    : Item_func_email_string(thd, arg, NORMALIZED,
                             "email_normalize"_LEX_CSTRING) {}
};

static Create_email_func_arg1<Item_func_email_is_valid> create_email_is_valid;
static Create_email_func_arg1<Item_func_email_local_part>
  create_email_local_part;
static Create_email_func_arg1<Item_func_email_domain> create_email_domain;
static Create_email_func_arg1<Item_func_email_normalize> create_email_normalize;

Plugin_function plugin_descriptor_email_is_valid(&create_email_is_valid);
Plugin_function plugin_descriptor_email_local_part(&create_email_local_part);
Plugin_function plugin_descriptor_email_domain(&create_email_domain);
Plugin_function plugin_descriptor_email_normalize(&create_email_normalize);
