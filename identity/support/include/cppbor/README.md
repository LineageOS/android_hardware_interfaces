CppBor: A Modern C++ CBOR Parser and Generator
==============================================

CppBor provides a natural and easy-to-use syntax for constructing and
parsing CBOR messages.  It does not (yet) support all features of
CBOR, nor (yet) support validation against CDDL schemata, though both
are planned.  CBOR features that aren't supported include:

* Indefinite length values
* Semantic tagging
* Floating point

CppBor requires C++-17.

## CBOR representation

CppBor represents CBOR data items as instances of the `Item` class or,
more precisely, as instances of subclasses of `Item`, since `Item` is a
pure interface.  The subclasses of `Item` correspond almost one-to-one
with CBOR major types, and are named to match the CDDL names to which
they correspond.  They are:

* `Uint` corresponds to major type 0, and can hold unsigned integers
  up through (2^64 - 1).
* `Nint` corresponds to major type 1.  It can only hold values from -1
  to -(2^63 - 1), since it's internal representation is an int64_t.
  This can be fixed, but it seems unlikely that applications will need
  the omitted range from -(2^63) to (2^64 - 1), since it's
  inconvenient to represent them in many programming languages.
* `Int` is an abstract base of `Uint` and `Nint` that facilitates
  working with all signed integers representable with int64_t.
* `Bstr` corresponds to major type 2, a byte string.
* `Tstr` corresponds to major type 3, a text string.
* `Array` corresponds to major type 4, an Array.  It holds a
  variable-length array of `Item`s.
* `Map` corresponds to major type 5, a Map.  It holds a
  variable-length array of pairs of `Item`s.
* `Simple` corresponds to major type 7.  It's an abstract class since
  items require more specific type.
* `Bool` is the only currently-implemented subclass of `Simple`.

Note that major type 6, semantic tag, is not yet implemented.

In practice, users of CppBor will rarely use most of these classes
when generating CBOR encodings.  This is because CppBor provides
straightforward conversions from the obvious normal C++ types.
Specifically, the following conversions are provided in appropriate
contexts:

* Signed and unsigned integers convert to `Uint` or `Nint`, as
  appropriate.
* `std::string`, `std::string_view`, `const char*` and
  `std::pair<char iterator, char iterator>` convert to `Tstr`.
* `std::vector<uint8_t>`, `std::pair<uint8_t iterator, uint8_t
  iterator>` and `std::pair<uint8_t*, size_t>` convert to `Bstr`.
* `bool` converts to `Bool`.

## CBOR generation

### Complete tree generation

The set of `encode` methods in `Item` provide the interface for
producing encoded CBOR.  The basic process for "complete tree"
generation (as opposed to "incremental" generation, which is discussed
below) is to construct an `Item` which models the data to be encoded,
and then call one of the `encode` methods, whichever is convenient for
the encoding destination.  A trivial example:

```
cppbor::Uint val(0);
std::vector<uint8_t> encoding = val.encode();
```

    It's relatively rare that single values are encoded as above.  More often, the
    "root" data item will be an `Array` or `Map` which contains a more complex structure.For example
    :

``` using cppbor::Map;
using cppbor::Array;

std::vector<uint8_t> vec =  // ...
    Map val("key1", Array(Map("key_a", 99 "key_b", vec), "foo"), "key2", true);
std::vector<uint8_t> encoding = val.encode();
```

This creates a map with two entries, with `Tstr` keys "Outer1" and
"Outer2", respectively.  The "Outer1" entry has as its value an
`Array` containing a `Map` and a `Tstr`.  The "Outer2" entry has a
`Bool` value.

This example demonstrates how automatic conversion of C++ types to
CppBor `Item` subclass instances is done.  Where the caller provides a
C++ or C string, a `Tstr` entry is added.  Where the caller provides
an integer literal or variable, a `Uint` or `Nint` is added, depending
on whether the value is positive or negative.

As an alternative, a more fluent-style API is provided for building up
structures.  For example:

```
using cppbor::Map;
using cppbor::Array;

std::vector<uint8_t> vec =  // ...
    Map val();
val.add("key1", Array().add(Map().add("key_a", 99).add("key_b", vec)).add("foo")).add("key2", true);
std::vector<uint8_t> encoding = val.encode();
```

    An advantage of this interface over the constructor -
    based creation approach above is that it need not be done all at once
        .The `add` methods return a reference to the object added to to allow calls to be chained,
    but chaining is not necessary; calls can be made
sequentially, as the data to add is available.

#### `encode` methods

There are several variations of `Item::encode`, all of which
accomplish the same task but output the encoded data in different
ways, and with somewhat different performance characteristics.  The
provided options are:

* `bool encode(uint8\_t** pos, const uint8\_t* end)` encodes into the
  buffer referenced by the range [`*pos`, end).  `*pos` is moved.  If
  the encoding runs out of buffer space before finishing, the method
  returns false.  This is the most efficient way to encode, into an
  already-allocated buffer.
* `void encode(EncodeCallback encodeCallback)` calls `encodeCallback`
  for each encoded byte.  It's the responsibility of the implementor
  of the callback to behave safely in the event that the output buffer
  (if applicable) is exhausted.  This is less efficient than the prior
  method because it imposes an additional function call for each byte.
* `template </*...*/> void encode(OutputIterator i)`
  encodes into the provided iterator.  SFINAE ensures that the
  template doesn't match for non-iterators.  The implementation
  actually uses the callback-based method, plus has whatever overhead
  the iterator adds.
* `std::vector<uint8_t> encode()` creates a new std::vector, reserves
  sufficient capacity to hold the encoding, and inserts the encoded
  bytes with a std::pushback_iterator and the previous method.
* `std::string toString()` does the same as the previous method, but
  returns a string instead of a vector.

### Incremental generation

Incremental generation requires deeper understanding of CBOR, because
the library can't do as much to ensure that the output is valid.  The
basic tool for intcremental generation is the `encodeHeader`
function.  There are two variations, one which writes into a buffer,
and one which uses a callback.  Both simply write out the bytes of a
header.  To construct the same map as in the above examples,
incrementally, one might write:

```
using namespace cppbor;  // For example brevity

std::vector encoding;
auto iter = std::back_inserter(result);
encodeHeader(MAP, 2 /* # of map entries */, iter);
std::string s = "key1";
encodeHeader(TSTR, s.size(), iter);
std::copy(s.begin(), s.end(), iter);
encodeHeader(ARRAY, 2 /* # of array entries */, iter);
Map().add("key_a", 99).add("key_b", vec).encode(iter)
s = "foo";
encodeHeader(TSTR, foo.size(), iter);
std::copy(s.begin(), s.end(), iter);
s = "key2";
encodeHeader(TSTR, foo.size(), iter);
std::copy(s.begin(), s.end(), iter);
encodeHeader(SIMPLE, TRUE, iter);
```

As the above example demonstrates, the styles can be mixed -- Note the
creation and encoding of the inner Map using the fluent style.

## Parsing

CppBor also supports parsing of encoded CBOR data, with the same
feature set as encoding.  There are two basic approaches to parsing,
"full" and "stream"

### Full parsing

Full parsing means completely parsing a (possibly-compound) data
item from a byte buffer.  The `parse` functions that do not take a
`ParseClient` pointer do this.  They return a `ParseResult` which is a
tuple of three values:

* std::unique_ptr<Item> that points to the parsed item, or is nullptr
  if there was a parse error.
* const uint8_t* that points to the byte after the end of the decoded
  item, or to the first unparseable byte in the event of an error.
* std::string that is empty on success or contains an error message if
  a parse error occurred.

Assuming a successful parse, you can then use `Item::type()` to
discover the type of the parsed item (e.g. MAP), and then use the
appropriate `Item::as*()` method (e.g. `Item::asMap()`) to get a
pointer to an interface which allows you to retrieve specific values.

### Stream parsing

Stream parsing is more complex, but more flexible.  To use
StreamParsing, you must create your own subclass of `ParseClient` and
call one of the `parse` functions that accepts it.  See the
`ParseClient` methods docstrings for details.

One unusual feature of stream parsing is that the `ParseClient`
callback methods not only provide the parsed Item, but also pointers
to the portion of the buffer that encode that Item.  This is useful
if, for example, you want to find an element inside of a structure,
and then copy the encoding of that sub-structure, without bothering to
parse the rest.

The full parser is implemented with the stream parser.
