#ifndef OPENALPHA_DATA_H_
#define OPENALPHA_DATA_H_

#include <arrow/python/pyarrow.h>
#include <arrow/table.h>
#include <string>
#include <unordered_map>

#include "common.h"
#include "logger.h"
#include "python.h"

namespace openalpha {

struct Table : public std::shared_ptr<arrow::Table> {
  template <typename T>
  void Assert(int icol) {
    if (icol >= num_columns()) {
      LOG_FATAL("DataRegistry: column index "
                << icol << " out of range " << num_columns() << " of '" << name
                << "'");
    }
    auto type = (*this)->column(icol)->type();
    if (!type) {
      LOG_FATAL("DataRegistry: empty data type of '"
                << name << "', expected "
                << boost::typeindex::type_id<T>().pretty_name());
    }
    bool res;
    if constexpr (std::is_same<T, double>::value) {
      res = type->id() == arrow::Type::DOUBLE;
    } else if constexpr (std::is_same<T, float>::value) {
      res = type->id() == arrow::Type::FLOAT;
    } else if constexpr (std::is_same<T, int64_t>::value) {
      res = type->id() == arrow::Type::INT64;
    } else if constexpr (std::is_same<T, uint64_t>::value) {
      res = type->id() == arrow::Type::UINT64;
    } else if constexpr (std::is_same<T, int32_t>::value) {
      res = type->id() == arrow::Type::INT32;
    } else if constexpr (std::is_same<T, uint32_t>::value) {
      res = type->id() == arrow::Type::UINT32;
    } else if constexpr (std::is_same<T, int16_t>::value) {
      res = type->id() == arrow::Type::INT16;
    } else if constexpr (std::is_same<T, uint16_t>::value) {
      res = type->id() == arrow::Type::UINT16;
    } else if constexpr (std::is_same<T, int8_t>::value) {
      res = type->id() == arrow::Type::INT8;
    } else if constexpr (std::is_same<T, uint8_t>::value) {
      res = type->id() == arrow::Type::UINT8;
    } else if constexpr (std::is_same<T, bool>::value) {
      res = type->id() == arrow::Type::BOOL;
    } else if constexpr (std::is_same<T, std::string>::value) {
      res = type->id() == arrow::Type::STRING;
    } else {
      res = false;
    }
    if (!res) {
      LOG_FATAL("DataRegistry: invalid data type '"
                << type->name() << "' of '" << name << "', expected '"
                << boost::typeindex::type_id<T>().pretty_name() << "'");
    }
  }

  template <typename T>
  const T* Column(int icol) {
    // https://github.com/apache/arrow/blob/master/cpp/examples/arrow/row-wise-conversion-example.cc
    Assert<T>(icol);
    auto data = (*this)->column(icol)->data();
    if (data->num_chunks() > 1 || data->null_count() > 0) {
      // can not return null value columns as raw because those null values are
      // not initialized. can not use chunk->IsNull(i) to check
      LOG_FATAL("DataRegistry: can not get #"
                << icol << " column of '" << name
                << "' as a raw pointer, because it has "
                << (data->num_chunks() > 1 ? "more than 1 chunks"
                                           : "has null value"));
    }
    auto chunk = data->chunk(0);
    if constexpr (std::is_same<T, double>::value) {
      return std::static_pointer_cast<arrow::DoubleArray>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, float>::value) {
      return std::static_pointer_cast<arrow::FloatArray>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, int64_t>::value) {
      return std::static_pointer_cast<arrow::Int64Array>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, uint64_t>::value) {
      return std::static_pointer_cast<arrow::UInt64Array>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, int32_t>::value) {
      return std::static_pointer_cast<arrow::Int32Array>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, uint32_t>::value) {
      return std::static_pointer_cast<arrow::UInt32Array>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, int16_t>::value) {
      return std::static_pointer_cast<arrow::Int16Array>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, uint16_t>::value) {
      return std::static_pointer_cast<arrow::UInt16Array>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, int8_t>::value) {
      return std::static_pointer_cast<arrow::Int8Array>(chunk)->raw_values();
    } else if constexpr (std::is_same<T, uint8_t>::value) {
      return std::static_pointer_cast<arrow::UInt8Array>(chunk)->raw_values();
    } else {
      return nullptr;
      assert(false);
    }
  }

  template <typename T>
  T Value(int irow, int icol) {
    auto col = (*this)->column(icol)->data();
    if (irow >= num_rows()) {
      LOG_FATAL("DataRegistry: row index " << irow << " out of range "
                                           << num_rows() << " of '" << name
                                           << "'");
    }
    auto chunk = col->chunk(0);
    auto n = 0;
    while (irow >= chunk->length()) {
      irow -= chunk->length();
      chunk = col->chunk(++n);
    }
    if constexpr (std::is_same<T, double>::value) {
      if (chunk->IsNull(irow)) return kNaN;
      return std::static_pointer_cast<arrow::DoubleArray>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, float>::value) {
      if (chunk->IsNull(irow)) return kNaN;
      return std::static_pointer_cast<arrow::FloatArray>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, int64_t>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::Int64Array>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, uint64_t>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::UInt64Array>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, int32_t>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::Int32Array>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, uint32_t>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::UInt32Array>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, int16_t>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::Int16Array>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, uint16_t>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::UInt16Array>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, int8_t>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::Int8Array>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, uint8_t>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::UInt8Array>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, bool>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::BooleanArray>(chunk)->Value(irow);
    } else if constexpr (std::is_same<T, std::string>::value) {
      if (chunk->IsNull(irow)) return T{};
      return std::static_pointer_cast<arrow::StringArray>(chunk)->GetString(
          irow);
    } else {
      return T{};
      assert(false);
    }
  }

  template <typename T, typename Visitor>
  void Visit(int irow, int icol, int row_offset, Visitor visitor) {
    auto col = (*this)->column(icol)->data();
    int num_rows = this->num_rows();
    if (irow >= num_rows) {
      LOG_FATAL("DataRegistry: row index " << irow << " out of range "
                                           << num_rows << " of '" << name
                                           << "'");
    }
    auto chunk = col->chunk(0);
    auto n = 0;
    auto irow_begin = row_offset < 0 ? std::max(0, irow + row_offset) : irow;
    auto irow_end =
        row_offset < 0 ? irow + 1 : std::min(irow + row_offset + 1, num_rows);
    for (auto i = irow_begin; i != irow_end; ++i) {
      while (i >= chunk->length()) {
        i -= chunk->length();
        irow_end -= chunk->length();
        irow -= chunk->length();
        chunk = col->chunk(++n);
      }
      T v{};
      if constexpr (std::is_same<T, double>::value) {
        if (chunk->IsNull(i))
          v = kNaN;
        else
          v = std::static_pointer_cast<arrow::DoubleArray>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, float>::value) {
        if (chunk->IsNull(i))
          v = kNaN;
        else
          v = std::static_pointer_cast<arrow::FloatArray>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, int64_t>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::Int64Array>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, uint64_t>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::UInt64Array>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, int32_t>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::Int32Array>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, uint32_t>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::UInt32Array>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, int16_t>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::Int16Array>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, uint16_t>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::UInt16Array>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, int8_t>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::Int8Array>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, uint8_t>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::UInt8Array>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, bool>::value) {
        if (!chunk->IsNull(i))
          v = std::static_pointer_cast<arrow::BooleanArray>(chunk)->Value(i);
      } else if constexpr (std::is_same<T, std::string>::value) {
        if (!chunk->IsNull(i)) {
          v = std::static_pointer_cast<arrow::StringArray>(chunk)->GetString(i);
        }
      } else {
        assert(false);
      }
      if (visitor(v, i - irow)) break;
    }
  }

  template <typename T>
  const T* Data() {
    if (num_columns() != 1) {
      LOG_FATAL(
          "DataRegistry: Data function only works for one column table, not "
          "applicable to '"
          << name << "'");
    }
    return Column<T>(0);
  }
  // parquet has extra one index column
  int num_columns() { return (*this)->num_columns() - 1; }
  int num_rows() { return (*this)->num_rows(); }

  std::string name;
};

class DataRegistry : public Singleton<DataRegistry> {
 public:
  typedef std::unordered_map<std::string, Table> ArrayMap;
  typedef std::unordered_map<std::string, bp::object> PyArrayMap;
  void Initialize();
  bool Has(const std::string& name);
  Table GetData(const std::string& name, bool retain = true);
  bp::object GetDataPy(std::string name, bool retain = true);

 private:
  ArrayMap array_map_;
  PyArrayMap py_array_map_;
};

}  // namespace openalpha

#endif  // OPENALPHA_DATA_H_
