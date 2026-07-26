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
#include "sql_lex.h"
#include "sql_type_email.h"
#include "email_validation.h"


Type_handler_email type_handler_email;
Type_collection_email type_collection_email;

const Type_collection *Type_handler_email::type_collection() const
{
  return &type_collection_email;
}

const Type_handler *Type_collection_email::aggregate_for_comparison(
                       const Type_handler *a, const Type_handler *b) const
{
  if (a->type_collection() == this)
    swap_variables(const Type_handler *, a, b);
  if (a == &type_handler_email      || a == &type_handler_hex_hybrid ||
      a == &type_handler_tiny_blob   || a == &type_handler_blob       ||
      a == &type_handler_medium_blob || a == &type_handler_long_blob  ||
      a == &type_handler_varchar     || a == &type_handler_string     ||
      a == &type_handler_null)
    return b;
  return NULL;
}

const Type_handler *Type_collection_email::aggregate_for_result(
                       const Type_handler *a, const Type_handler *b) const
{
  return aggregate_for_comparison(a,b);
}

const Type_handler *Type_collection_email::aggregate_for_min_max(
                       const Type_handler *a, const Type_handler *b) const
{
  return aggregate_for_comparison(a,b);
}

const Type_handler *Type_collection_email::aggregate_for_num_op(
                      const Type_handler *a, const Type_handler *b) const
{
  return NULL;
}


constexpr LEX_CSTRING Type_handler_email::name_on_client;

const Type_handler *Type_handler_email::type_handler_for_comparison() const
{
  return &type_handler_email;
}


Field *Type_handler_email::make_conversion_table_field(
      MEM_ROOT *root, TABLE *table, uint metadata, const Field *target) const
{
  /* Copied from Type_handler_blob_common. */
  uint pack_length= metadata & 0x00ff;
  if (pack_length != 4)
    return NULL; // Broken binary log?

  return new(root)
    Field_email(NULL, (uchar *) "", 1, Field::NONE, &empty_clex_str,
                  table->s, target->charset());
}


Item *Type_handler_email::create_typecast_item(THD *thd, Item *item,
        const Type_cast_attributes &attr) const
{
  CHARSET_INFO *real_cs= attr.charset() ?
                  attr.charset() : thd->variables.collation_connection;

  if (real_cs == &my_charset_bin)
  {
    my_error(ER_ILLEGAL_PARAMETER_DATA_TYPE_FOR_OPERATION, MYF(0),
             name().ptr(), "CHARACTER SET binary");
    return NULL;
  }

  return new (thd->mem_root) Item_email_typecast(thd, item, real_cs);
}


bool Type_handler_email:: Column_definition_prepare_stage1(THD *thd,
  MEM_ROOT *mem_root, Column_definition *def, column_definition_type_t type,
  const Column_derived_attributes *derived_attr) const
{
  if (Type_handler_long_blob::
      Column_definition_prepare_stage1(thd, mem_root, def, type, derived_attr))
    return true;
  if (def->charset == &my_charset_bin)
  {
    my_error(ER_ILLEGAL_PARAMETER_DATA_TYPE_FOR_OPERATION, MYF(0),
             name().ptr(), "CHARACTER SET binary");
    return true;
  }
  return false;
}


Field *Type_handler_email::make_table_field(MEM_ROOT *root,
         const LEX_CSTRING *name, const Record_addr &addr,
         const Type_all_attributes &attr, TABLE_SHARE *share) const
{
  return new (root) Field_email(addr.ptr(), addr.null_ptr(), addr.null_bit(),
                                 Field::NONE, name, share, attr.collation);
}


Field *Type_handler_email::make_table_field_from_def(TABLE_SHARE *share,
         MEM_ROOT *root, const LEX_CSTRING *name, const Record_addr &rec,
         const Bit_addr &bit, const Column_definition_attributes *attr,
         uint32 flags) const
{
  return new (root) Field_email(rec.ptr(), rec.null_ptr(), rec.null_bit(),
            attr->unireg_check, name, share, attr->charset);
}


Item *
Type_handler_email::make_constructor_item(THD *thd, List<Item> *args) const
{
  if (!args || args->elements != 1)
    return NULL;
  Item_args tmp(thd, *args);
  return new (thd->mem_root)
    Item_email_typecast(thd, tmp.arguments()[0],
                          thd->variables.collation_connection);
}


bool Type_handler_email::
  Item_hybrid_func_fix_attributes(THD *thd, const LEX_CSTRING &func_name,
         Type_handler_hybrid_field_type *handler, Type_all_attributes *func,
         Item **items, uint nitems) const
{
  if (func->aggregate_attributes_string(func_name, items, nitems))
    return true;

  handler->set_handler(&type_handler_email);
  return false;
}


const Type_handler *Type_handler_email::
  type_handler_for_tmp_table(const Item *item) const
{
  return &type_handler_email;
}



/*****************************************************************/
void Field_email::sql_type(String &res) const
{
  res.set_ascii(STRING_WITH_LEN("email"));
}


static bool check_email(const char *value, size_t length)
{
  return email::validate(value, length);
}


int Field_email::store(const char *from, size_t length, CHARSET_INFO *cs)
{
  if (!check_email(from, length))
    goto err;


  return Field_blob::store(from, length, cs);

err:
  my_error(ER_WRONG_VALUE, MYF(0),
           "EMAIL", ErrConvString(from, length, cs).ptr());

  if (maybe_null())
    set_null();
  else
    Field_blob::store(STRING_WITH_LEN("invalid@invalid"), cs);
  return -1;
}


/*
  We allow any string input into the EMAIL,
  as it can fit into LONG BLOB without any loss.
  But in any case values themselves can be invalid EMAIL-s.

  TODO: when the replication start sending UDT informatio,
  we should only return CONV_TYPE_PRECISE for the EMAIL.
*/
enum_conv_type
Field_email::rpl_conv_type_from(const Conv_source &source,
                                  const Relay_log_info *rli,
                                  const Conv_param &param) const
{
  const Type_handler *th= source.type_handler();
  if (th == &type_handler_tiny_blob ||
      th == &type_handler_medium_blob ||
      th == &type_handler_long_blob ||
      th == &type_handler_blob ||
      th == &type_handler_blob_compressed ||
      th == &type_handler_string ||
      th == &type_handler_var_string ||
      th == &type_handler_varchar ||
      th == &type_handler_varchar_compressed)
  {
    return CONV_TYPE_PRECISE;
  }

  return CONV_TYPE_IMPOSSIBLE;
}


class Item_email_typecast_func_handler: public Item_handled_func::Handler_str
{
public:
  const Type_handler *
    return_type_handler(const Item_handled_func *item) const override
  { return &type_handler_email; }

  const Type_handler *
    type_handler_for_create_select(const Item_handled_func *item) const override
  { return &type_handler_email; }

  bool fix_length_and_dec(Item_handled_func *item) const override
  {
    return false;
  }
  String *val_str(Item_handled_func *item, String *to) const override
  {
    DBUG_ASSERT(dynamic_cast<const Item_email_typecast*>(item));
    return static_cast<Item_email_typecast*>(item)->val_str_generic(to);
  }
};


static Item_email_typecast_func_handler item_email_typecast_func_handler;

bool Item_email_typecast::fix_length_and_dec(THD *thd)
{
  Item_char_typecast::fix_length_and_dec_str();
  set_func_handler(&item_email_typecast_func_handler);

  if (cast_charset()->mbminlen > 1)
  {
    my_error(ER_NOT_SUPPORTED_YET, MYF(0),
             "CAST(AS EMAIL CHARACTER SET ucs2/utf16/utf32)");
    return true;
  }

  return false;
}


String *Item_email_typecast::val_str(String *to)
{
  String *res= Item_char_typecast::val_str(to);
  if (!res)
    return NULL;

  if (!check_email(res->ptr(), res->length()))
  {
    null_value= TRUE;
    return NULL;
  }

  return res;
}


void Item_email_typecast::print(String *str, enum_query_type query_type)
{
  str->append(STRING_WITH_LEN("cast("));
  args[0]->print(str, query_type);
  str->append(STRING_WITH_LEN(" as email"));
  print_charset(str);
  str->append(')');
}
