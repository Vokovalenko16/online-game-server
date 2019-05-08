//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci-sqlite3.h"
#include "../../rowid.h"
#include "../../blob.h"
// std
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <sstream>
#include <string>

#ifdef _MSC_VER
#pragma warning(disable:4355 4996)
#define snprintf _snprintf
#endif

using namespace soci;
using namespace soci::details;

void sqlite3_standard_use_type_backend::bind_by_pos(int& position, void* data,
    exchange_type type, bool /*readOnly*/)
{
    if (statement_.boundByName_)
    {
        throw soci_error(
         "Binding for use elements must be either by position or by name.");
    }

    data_ = data;
    type_ = type;
    position_ = position++;

    statement_.boundByPos_ = true;
}

void sqlite3_standard_use_type_backend::bind_by_name(std::string const& name,
    void* data, exchange_type type, bool /*readOnly*/)
{
    if (statement_.boundByPos_)
    {
        throw soci_error(
         "Binding for use elements must be either by position or by name.");
    }

    data_ = data;
    type_ = type;
    name_ = ":" + name;

    statement_.reset_if_needed();
    position_ = sqlite3_bind_parameter_index(statement_.stmt_, name_.c_str());

    if (0 == position_)
    {
        std::ostringstream ss;
        ss << "Cannot bind to (by name) " << name_;
        throw soci_error(ss.str());
    }
    statement_.boundByName_ = true;
}

void sqlite3_standard_use_type_backend::pre_use(indicator const * ind)
{
    statement_.useData_.resize(1);
    int const pos = position_ - 1;

    if (statement_.useData_[0].size() < static_cast<std::size_t>(position_))
    {
        statement_.useData_[0].resize(position_);
    }

	sqlite3_column& column = statement_.useData_[0][pos];

    if (ind != NULL && *ind == i_null)
    {
		column.type_ = sqlite3_c_null;
		column.data_ = "";
		column.blobBuf_ = 0;
		column.blobSize_ = 0;
    }
    else
    {
        // allocate and fill the buffer with text-formatted client data
        switch (type_)
        {
        case x_char:
            {
				column.type_ = sqlite3_c_text;
				column.data_.assign(static_cast<const char*>(data_), 1);
            }
            break;
        case x_stdstring:
            {
				column.type_ = sqlite3_c_text;
				column.data_ = *static_cast<std::string *>(data_);
            }
            break;
        case x_short:
            {
				column.type_ = sqlite3_c_int;
				column.numeric.i = *static_cast<short*>(data_);
            }
            break;
        case x_integer:
            {
				column.type_ = sqlite3_c_int;
				column.numeric.i = *static_cast<int*>(data_);
			}
            break;
        case x_unsigned_long:
            {
				column.type_ = sqlite3_c_int;
				column.numeric.i = *static_cast<unsigned long*>(data_);
			}
            break;
        case x_long_long:
            {
				column.type_ = sqlite3_c_int64;
				column.numeric.ll = *static_cast<long long*>(data_);
			}
            break;
        case x_unsigned_long_long:
            {
				column.type_ = sqlite3_c_int64;
				column.numeric.ll = *static_cast<unsigned long long*>(data_);
			}
            break;
        case x_double:
            {
				column.type_ = sqlite3_c_double;
				column.numeric.d = *static_cast<double*>(data_);
			}
            break;
        case x_stdtm:
            {
				const int bufSize = 20;
				char temp[bufSize];

                std::tm *t = static_cast<std::tm *>(data_);
				snprintf(temp, bufSize, "%d-%02d-%02d %02d:%02d:%02d",
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                    t->tm_hour, t->tm_min, t->tm_sec);

				column.type_ = sqlite3_c_text;
				column.data_.assign(temp);
			}
            break;
        case x_rowid:
            {
                // RowID is internally identical to unsigned long

                rowid *rid = static_cast<rowid *>(data_);
                sqlite3_rowid_backend *rbe = static_cast<sqlite3_rowid_backend *>(rid->get_backend());

				column.type_ = sqlite3_c_int;
				column.numeric.i = rbe->value_;
			}
            break;
        case x_blob:
            {
                blob *b = static_cast<blob *>(data_);
                sqlite3_blob_backend *bbe =
                    static_cast<sqlite3_blob_backend *>(b->get_backend());

                std::size_t len = bbe->get_len();
                buf_ = new char[len];
                bbe->read(0, buf_, len);

				column.type_ = sqlite3_c_blob;
                column.blobBuf_ = buf_;
                column.blobSize_ = len;
            }
            break;
        default:
            throw soci_error("Use element used with non-supported type.");
        }
    }
}

void sqlite3_standard_use_type_backend::post_use(
    bool /* gotData */, indicator * /* ind */)
{
    // TODO: Is it possible to have the bound element being overwritten
    // by the database?
    // If not, then nothing to do here, please remove this comment.
    // If yes, then use the value of the readOnly parameter:
    // - true:  the given object should not be modified and the backend
    //          should detect if the modification was performed on the
    //          isolated buffer and throw an exception if the buffer was modified
    //          (this indicates logic error, because the user used const object
    //          and executed a query that attempted to modified it)
    // - false: the modification should be propagated to the given object.
    // ...

    // clean up the working buffer, it might be allocated anew in
    // the next run of preUse
    clean_up();
}

void sqlite3_standard_use_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
