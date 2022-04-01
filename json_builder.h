#pragma once

#include <cassert>
#include <memory>

#include "json.h"

using namespace std::string_literals;

namespace json {

class Builder;
class AfterStartDictContext;
class AfterStartArrayContext;
class AfterKeyContext;

class BuilderBaseContext {
public:
    BuilderBaseContext(Builder& builder);
    
protected:
    Builder& builder_;
};

class InsideArrayActions : public BuilderBaseContext {
public:
    InsideArrayActions Value(Node::Value value);
    
    InsideArrayActions StartArray();
    
    AfterStartDictContext StartDict();
    
    Builder& EndArray();
};

class AfterStartDictContext final : public BuilderBaseContext {
public:
    AfterKeyContext Key(std::string key);
    
    Builder& EndDict();
};

// After value that followed after Key(...)
class AfterValueInDictContext final : public BuilderBaseContext {
public:
    AfterKeyContext Key(std::string value);
    
    Builder& EndDict();
};

class AfterKeyContext final : public BuilderBaseContext {
public:
    AfterKeyContext(Builder& builder) : BuilderBaseContext(builder) {}
    
    AfterValueInDictContext Value(Node::Value value);
    
    InsideArrayActions StartArray();
    
    AfterStartDictContext StartDict();
    
private:
    AfterValueInDictContext after_value_in_dict_context_{builder_};
};


class Builder {
public:
    Builder& Value(Node::Value value) {
        if (!root_.IsNull()) {
            throw std::logic_error("value has to be null when Value method is used"s);
        }
        // Get node using value
        std::unique_ptr<Node> ptr;
        visit([&ptr](auto&& val) { ptr = std::make_unique<Node>(val); }, value);
        
        // For empty
        if (nodes_stack_.empty()) {
            root_ = *ptr.release();
            return *this;
        }
        
        // For Array
        if (nodes_stack_.back()->IsArray()) {
            nodes_stack_.back()->AsArray().emplace_back(*ptr.release());
            return *this;
        }
        
        // For Key
        if (nodes_stack_.back()->IsString()) {
            std::string key = nodes_stack_.back().release()->AsString();
            nodes_stack_.pop_back();
            nodes_stack_.back()->AsDict().emplace(key, *ptr.release());
            
            return *this;
        }
        
        // if root not
        throw std::logic_error("value couldn't be inserted"s);
    }
    
    // Array
    InsideArrayActions StartArray() {
        // not null for when it is the first element
        if (!root_.IsNull()) {
            throw std::logic_error("probably object has already been constructed"s);
        }
        
        // nodes_stack_ can be empty so Dict will be root or new dict can be an element in array or
        // value to key in Dict
        if (!(nodes_stack_.empty() || nodes_stack_.back()->IsArray() ||
              nodes_stack_.back()->IsString())) {
            throw std::logic_error("nodes stack is not empty"s);
        }
        
        auto ptr = std::make_unique<Node>();
        *ptr = json::Array{};
        nodes_stack_.push_back(std::move(ptr));
        
        return after_start_array_context_;
    }
    
    Builder& EndArray() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("end dict problem"s);
        }
        
        if (nodes_stack_.back()->IsArray()) {
            auto array_being_closed = std::move(nodes_stack_.back());
            nodes_stack_.pop_back();
            
            // if array is root element
            if (nodes_stack_.empty()) {
                root_ = *array_being_closed.release();
                
                return *this;
            }
            
            // if array is inside another array
            if (nodes_stack_.back()->IsArray()) {
                nodes_stack_.back()->AsArray().push_back(*array_being_closed.release());
                
                return *this;
            }
            
            // if array is value in another array
            if (nodes_stack_.back()->IsString()) {
                std::string key = nodes_stack_.back().release()->AsString();
                nodes_stack_.pop_back();
                nodes_stack_.back()->AsDict().emplace(key, *array_being_closed.release());
                
                return *this;
            }
        }
        
        throw std::logic_error("couldn't close array"s);
    }
    
    // Dict
    AfterStartDictContext& StartDict() {
        // not null for when it is the first element
        if (!root_.IsNull()) {
            throw std::logic_error("probably object has already been constructed"s);
        }
        
        // nodes_stack_ can be empty so Dict will be root or new dict can be an element in array or
        // value to key in Dict
        if (!(nodes_stack_.empty() || nodes_stack_.back()->IsArray() ||
              nodes_stack_.back()->IsString())) {
            throw std::logic_error("nodes stack is not empty"s);
        }
        
        auto ptr = std::make_unique<Node>();
        *ptr = json::Dict{};
        nodes_stack_.push_back(std::move(ptr));
        
        return dict_item_context_;
    }
    
    Builder& EndDict() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("end dict problem"s);
        }
        
        if (nodes_stack_.back()->IsDict()) {
            auto dict_being_closed = std::move(nodes_stack_.back());
            nodes_stack_.pop_back();
            
            if (nodes_stack_.empty()) {
                root_ = *dict_being_closed.release();
                
                return *this;
            }
            
            // dict element of array
            if (nodes_stack_.back()->IsArray()) {
                nodes_stack_.back()->AsArray().push_back(*dict_being_closed.release());
                
                return *this;
            }
            
            // dict is value in another dict
            if (nodes_stack_.back()->IsString()) {
                std::string key = nodes_stack_.back().release()->AsString();
                nodes_stack_.pop_back();
                nodes_stack_.back()->AsDict().emplace(key, *dict_being_closed.release());
                
                return *this;
            }
        }
        
        throw std::logic_error("couldn't close dict"s);
    }
    
    AfterKeyContext& Key(std::string key) {
        if (nodes_stack_.empty() || !root_.IsNull()) {
            throw std::logic_error("couldn't use Key"s);
        }
        
        if (nodes_stack_.back()->IsDict()) {
            auto ptr = std::make_unique<Node>();
            *ptr = std::move(key);
            nodes_stack_.push_back(std::move(ptr));
            
            return key_item_context_;
        }
        
        throw std::logic_error("couldn't insert key"s);
    }
    
    Node Build() const {
        if (root_.IsNull()) {
            // if object hasn't been constructed
            throw std::logic_error("root is null"s);
        }
        
        if (!nodes_stack_.empty()) {
            // if object is unfinished
            throw std::logic_error("stack is not empty"s);
        }
        
        return root_;
    }
    
private:
    Node root_ = nullptr;
    std::vector<std::unique_ptr<Node>> nodes_stack_;
    
    AfterStartDictContext dict_item_context_{*this};
    AfterKeyContext key_item_context_{*this};
    InsideArrayActions after_start_array_context_{*this};
};

}  // namespace json
