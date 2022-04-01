#include "json_builder.h"

using namespace json;

// BaseContext
BuilderBaseContext::BuilderBaseContext(Builder& builder) : builder_(builder) {}

// InsideArrayActions
InsideArrayActions InsideArrayActions::Value(Node::Value value) {
    builder_.Value(value);
    return *this;
}

InsideArrayActions InsideArrayActions::StartArray() { return builder_.StartArray(); }

AfterStartDictContext InsideArrayActions::StartDict() { return builder_.StartDict(); }

Builder& InsideArrayActions::EndArray() { return builder_.EndArray(); }

// AfterStartDictContext
AfterKeyContext AfterStartDictContext::Key(std::string key) { return builder_.Key(key); }

Builder& AfterStartDictContext::EndDict() { return builder_.EndDict(); }

// AfterValueInDictContext
AfterKeyContext AfterValueInDictContext::Key(std::string value) { return builder_.Key(value); }

Builder& AfterValueInDictContext::EndDict() { return builder_.EndDict(); }

// AfterKeyContext
AfterValueInDictContext AfterKeyContext::Value(Node::Value value) {
    builder_.Value(value);
    return after_value_in_dict_context_;
}

InsideArrayActions AfterKeyContext::StartArray() { return builder_.StartArray(); }

AfterStartDictContext AfterKeyContext::StartDict() { return builder_.StartDict(); }
