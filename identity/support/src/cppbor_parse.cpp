/*
 * Copyright (c) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cppbor_parse.h"

#include <sstream>
#include <stack>

#define LOG_TAG "CppBor"
#include <android-base/logging.h>

namespace cppbor {

namespace {

std::string insufficientLengthString(size_t bytesNeeded, size_t bytesAvail,
                                     const std::string& type) {
    std::stringstream errStream;
    errStream << "Need " << bytesNeeded << " byte(s) for " << type << ", have " << bytesAvail
              << ".";
    return errStream.str();
}

template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
std::tuple<bool, uint64_t, const uint8_t*> parseLength(const uint8_t* pos, const uint8_t* end,
                                                       ParseClient* parseClient) {
    if (pos + sizeof(T) > end) {
        parseClient->error(pos - 1, insufficientLengthString(sizeof(T), end - pos, "length field"));
        return {false, 0, pos};
    }

    const uint8_t* intEnd = pos + sizeof(T);
    T result = 0;
    do {
        result = static_cast<T>((result << 8) | *pos++);
    } while (pos < intEnd);
    return {true, result, pos};
}

std::tuple<const uint8_t*, ParseClient*> parseRecursively(const uint8_t* begin, const uint8_t* end,
                                                          ParseClient* parseClient);

std::tuple<const uint8_t*, ParseClient*> handleUint(uint64_t value, const uint8_t* hdrBegin,
                                                    const uint8_t* hdrEnd,
                                                    ParseClient* parseClient) {
    std::unique_ptr<Item> item = std::make_unique<Uint>(value);
    return {hdrEnd,
            parseClient->item(item, hdrBegin, hdrEnd /* valueBegin */, hdrEnd /* itemEnd */)};
}

std::tuple<const uint8_t*, ParseClient*> handleNint(uint64_t value, const uint8_t* hdrBegin,
                                                    const uint8_t* hdrEnd,
                                                    ParseClient* parseClient) {
    if (value > std::numeric_limits<int64_t>::max()) {
        parseClient->error(hdrBegin, "NINT values that don't fit in int64_t are not supported.");
        return {hdrBegin, nullptr /* end parsing */};
    }
    std::unique_ptr<Item> item = std::make_unique<Nint>(-1 - static_cast<uint64_t>(value));
    return {hdrEnd,
            parseClient->item(item, hdrBegin, hdrEnd /* valueBegin */, hdrEnd /* itemEnd */)};
}

std::tuple<const uint8_t*, ParseClient*> handleBool(uint64_t value, const uint8_t* hdrBegin,
                                                    const uint8_t* hdrEnd,
                                                    ParseClient* parseClient) {
    std::unique_ptr<Item> item = std::make_unique<Bool>(value == TRUE);
    return {hdrEnd,
            parseClient->item(item, hdrBegin, hdrEnd /* valueBegin */, hdrEnd /* itemEnd */)};
}

std::tuple<const uint8_t*, ParseClient*> handleNull(const uint8_t* hdrBegin, const uint8_t* hdrEnd,
                                                    ParseClient* parseClient) {
    std::unique_ptr<Item> item = std::make_unique<Null>();
    return {hdrEnd,
            parseClient->item(item, hdrBegin, hdrEnd /* valueBegin */, hdrEnd /* itemEnd */)};
}

template <typename T>
std::tuple<const uint8_t*, ParseClient*> handleString(uint64_t length, const uint8_t* hdrBegin,
                                                      const uint8_t* valueBegin, const uint8_t* end,
                                                      const std::string& errLabel,
                                                      ParseClient* parseClient) {
    if (end - valueBegin < static_cast<ssize_t>(length)) {
        parseClient->error(hdrBegin, insufficientLengthString(length, end - valueBegin, errLabel));
        return {hdrBegin, nullptr /* end parsing */};
    }

    std::unique_ptr<Item> item = std::make_unique<T>(valueBegin, valueBegin + length);
    return {valueBegin + length,
            parseClient->item(item, hdrBegin, valueBegin, valueBegin + length)};
}

class IncompleteItem {
  public:
    virtual ~IncompleteItem() {}
    virtual void add(std::unique_ptr<Item> item) = 0;
};

class IncompleteArray : public Array, public IncompleteItem {
  public:
    IncompleteArray(size_t size) : mSize(size) {}

    // We return the "complete" size, rather than the actual size.
    size_t size() const override { return mSize; }

    void add(std::unique_ptr<Item> item) override {
        mEntries.reserve(mSize);
        mEntries.push_back(std::move(item));
    }

  private:
    size_t mSize;
};

class IncompleteMap : public Map, public IncompleteItem {
  public:
    IncompleteMap(size_t size) : mSize(size) {}

    // We return the "complete" size, rather than the actual size.
    size_t size() const override { return mSize; }

    void add(std::unique_ptr<Item> item) override {
        mEntries.reserve(mSize * 2);
        mEntries.push_back(std::move(item));
    }

  private:
    size_t mSize;
};

class IncompleteSemantic : public Semantic, public IncompleteItem {
  public:
    IncompleteSemantic(uint64_t value) : Semantic(value) {}

    // We return the "complete" size, rather than the actual size.
    size_t size() const override { return 1; }

    void add(std::unique_ptr<Item> item) override {
        mEntries.reserve(1);
        mEntries.push_back(std::move(item));
    }
};

std::tuple<const uint8_t*, ParseClient*> handleEntries(size_t entryCount, const uint8_t* hdrBegin,
                                                       const uint8_t* pos, const uint8_t* end,
                                                       const std::string& typeName,
                                                       ParseClient* parseClient) {
    while (entryCount > 0) {
        --entryCount;
        if (pos == end) {
            parseClient->error(hdrBegin, "Not enough entries for " + typeName + ".");
            return {hdrBegin, nullptr /* end parsing */};
        }
        std::tie(pos, parseClient) = parseRecursively(pos, end, parseClient);
        if (!parseClient) return {hdrBegin, nullptr};
    }
    return {pos, parseClient};
}

std::tuple<const uint8_t*, ParseClient*> handleCompound(
        std::unique_ptr<Item> item, uint64_t entryCount, const uint8_t* hdrBegin,
        const uint8_t* valueBegin, const uint8_t* end, const std::string& typeName,
        ParseClient* parseClient) {
    parseClient =
            parseClient->item(item, hdrBegin, valueBegin, valueBegin /* don't know the end yet */);
    if (!parseClient) return {hdrBegin, nullptr};

    const uint8_t* pos;
    std::tie(pos, parseClient) =
            handleEntries(entryCount, hdrBegin, valueBegin, end, typeName, parseClient);
    if (!parseClient) return {hdrBegin, nullptr};

    return {pos, parseClient->itemEnd(item, hdrBegin, valueBegin, pos)};
}

std::tuple<const uint8_t*, ParseClient*> parseRecursively(const uint8_t* begin, const uint8_t* end,
                                                          ParseClient* parseClient) {
    const uint8_t* pos = begin;

    MajorType type = static_cast<MajorType>(*pos & 0xE0);
    uint8_t tagInt = *pos & 0x1F;
    ++pos;

    bool success = true;
    uint64_t addlData;
    if (tagInt < ONE_BYTE_LENGTH || tagInt > EIGHT_BYTE_LENGTH) {
        addlData = tagInt;
    } else {
        switch (tagInt) {
            case ONE_BYTE_LENGTH:
                std::tie(success, addlData, pos) = parseLength<uint8_t>(pos, end, parseClient);
                break;

            case TWO_BYTE_LENGTH:
                std::tie(success, addlData, pos) = parseLength<uint16_t>(pos, end, parseClient);
                break;

            case FOUR_BYTE_LENGTH:
                std::tie(success, addlData, pos) = parseLength<uint32_t>(pos, end, parseClient);
                break;

            case EIGHT_BYTE_LENGTH:
                std::tie(success, addlData, pos) = parseLength<uint64_t>(pos, end, parseClient);
                break;

            default:
                CHECK(false);  //  It's impossible to get here
                break;
        }
    }

    if (!success) return {begin, nullptr};

    switch (type) {
        case UINT:
            return handleUint(addlData, begin, pos, parseClient);

        case NINT:
            return handleNint(addlData, begin, pos, parseClient);

        case BSTR:
            return handleString<Bstr>(addlData, begin, pos, end, "byte string", parseClient);

        case TSTR:
            return handleString<Tstr>(addlData, begin, pos, end, "text string", parseClient);

        case ARRAY:
            return handleCompound(std::make_unique<IncompleteArray>(addlData), addlData, begin, pos,
                                  end, "array", parseClient);

        case MAP:
            return handleCompound(std::make_unique<IncompleteMap>(addlData), addlData * 2, begin,
                                  pos, end, "map", parseClient);

        case SEMANTIC:
            return handleCompound(std::make_unique<IncompleteSemantic>(addlData), 1, begin, pos,
                                  end, "semantic", parseClient);

        case SIMPLE:
            switch (addlData) {
                case TRUE:
                case FALSE:
                    return handleBool(addlData, begin, pos, parseClient);
                case NULL_V:
                    return handleNull(begin, pos, parseClient);
            }
    }
    CHECK(false);  // Impossible to get here.
    return {};
}

class FullParseClient : public ParseClient {
  public:
    virtual ParseClient* item(std::unique_ptr<Item>& item, const uint8_t*, const uint8_t*,
                              const uint8_t* end) override {
        if (mParentStack.empty() && !item->isCompound()) {
            // This is the first and only item.
            mTheItem = std::move(item);
            mPosition = end;
            return nullptr;  //  We're done.
        }

        if (item->isCompound()) {
            // Starting a new compound data item, i.e. a new parent.  Save it on the parent stack.
            // It's safe to save a raw pointer because the unique_ptr is guaranteed to stay in
            // existence until the corresponding itemEnd() call.
            assert(dynamic_cast<CompoundItem*>(item.get()));
            mParentStack.push(static_cast<CompoundItem*>(item.get()));
            return this;
        } else {
            appendToLastParent(std::move(item));
            return this;
        }
    }

    virtual ParseClient* itemEnd(std::unique_ptr<Item>& item, const uint8_t*, const uint8_t*,
                                 const uint8_t* end) override {
        CHECK(item->isCompound() && item.get() == mParentStack.top());
        mParentStack.pop();

        if (mParentStack.empty()) {
            mTheItem = std::move(item);
            mPosition = end;
            return nullptr;  // We're done
        } else {
            appendToLastParent(std::move(item));
            return this;
        }
    }

    virtual void error(const uint8_t* position, const std::string& errorMessage) override {
        mPosition = position;
        mErrorMessage = errorMessage;
    }

    std::tuple<std::unique_ptr<Item> /* result */, const uint8_t* /* newPos */,
               std::string /* errMsg */>
    parseResult() {
        std::unique_ptr<Item> p = std::move(mTheItem);
        return {std::move(p), mPosition, std::move(mErrorMessage)};
    }

  private:
    void appendToLastParent(std::unique_ptr<Item> item) {
        auto parent = mParentStack.top();
        assert(dynamic_cast<IncompleteItem*>(parent));
        if (parent->type() == ARRAY) {
            static_cast<IncompleteArray*>(parent)->add(std::move(item));
        } else if (parent->type() == MAP) {
            static_cast<IncompleteMap*>(parent)->add(std::move(item));
        } else if (parent->type() == SEMANTIC) {
            static_cast<IncompleteSemantic*>(parent)->add(std::move(item));
        } else {
            CHECK(false);  // Impossible to get here.
        }
    }

    std::unique_ptr<Item> mTheItem;
    std::stack<CompoundItem*> mParentStack;
    const uint8_t* mPosition = nullptr;
    std::string mErrorMessage;
};

}  // anonymous namespace

void parse(const uint8_t* begin, const uint8_t* end, ParseClient* parseClient) {
    parseRecursively(begin, end, parseClient);
}

std::tuple<std::unique_ptr<Item> /* result */, const uint8_t* /* newPos */,
           std::string /* errMsg */>
parse(const uint8_t* begin, const uint8_t* end) {
    FullParseClient parseClient;
    parse(begin, end, &parseClient);
    return parseClient.parseResult();
}

}  // namespace cppbor
