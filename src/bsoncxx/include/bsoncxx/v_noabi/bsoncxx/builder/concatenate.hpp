// Copyright 2009-present MongoDB, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <bsoncxx/builder/concatenate-fwd.hpp>

#include <bsoncxx/array/view_or_value.hpp>
#include <bsoncxx/document/view_or_value.hpp>

#include <bsoncxx/config/prelude.hpp>

namespace bsoncxx {
namespace v_noabi {
namespace builder {

///
/// Container to concatenate a document. Use it by constructing an instance with the
/// document to be concatenated, and pass it into a document stream builder.
///
struct concatenate_doc {
    document::view_or_value doc;

    ///
    /// Conversion operator that provides a view of the wrapped concatenate
    /// document.
    ///
    /// @return A view of the wrapped concatenate document.
    ///
    BSONCXX_INLINE operator document::view() const {
        return doc;
    }

    ///
    /// Accessor that provides a view of the wrapped concatenate
    /// document.
    ///
    /// @return A view of the wrapped concatenate document.
    ///
    BSONCXX_INLINE document::view view() const {
        return doc;
    }
};

///
/// Container to concatenate an array. Use this with the array stream builder in order
/// to pass an array into a new builder and append its values to the stream.
///
struct concatenate_array {
    array::view_or_value array;

    ///
    /// Conversion operator that provides a view of the wrapped concatenate
    /// array.
    ///
    /// @return A view of the wrapped concatenate array.
    ///
    BSONCXX_INLINE operator array::view() const {
        return array;
    }

    ///
    /// Accessor that provides a view of the wrapped concatenate
    /// array.
    ///
    /// @return A view of the wrapped concatenate array.
    ///
    BSONCXX_INLINE array::view view() const {
        return array;
    }
};

///
/// Helper method to concatenate a document.
///
/// Use this with the document stream builder to merge an existing document's fields with a new
/// document's.
///
/// @param doc The document to concatenate.
///
/// @return concatenate_doc A concatenating struct.
///
/// @see bsoncxx::v_noabi::builder::concatenate_doc
///
/// @note An overload accepting @ref v_noabi::array::view_or_value and returning a @ref
/// concatenate_array is also declared in this scope.
///
BSONCXX_INLINE concatenate_doc concatenate(document::view_or_value doc) {
    return {std::move(doc)};
}

// Why is Doxygen unable to parse this overload correctly???
// @cond DOXYGEN_DISABLE "warning: no matching file member found for ..."
BSONCXX_INLINE concatenate_array concatenate(array::view_or_value array) {
    return {std::move(array)};
}
// @endcond

}  // namespace builder
}  // namespace v_noabi
}  // namespace bsoncxx

namespace bsoncxx {
namespace builder {

using ::bsoncxx::v_noabi::builder::concatenate;

}  // namespace builder
}  // namespace bsoncxx

#include <bsoncxx/config/postlude.hpp>
