#ifndef HTML_H
#define HTML_H

#include <memory>
#include <functional>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <cctype>
#include <algorithm>
#include <map>
#include <utility>

#define STATE_DATA 0
#define STATE_RAWTEXT 3
#define STATE_TAG_OPEN 6
#define STATE_END_TAG_OPEN 7
#define STATE_TAG_NAME 8
#define STATE_RAWTEXT_LESS_THAN_SIGN 12
#define STATE_RAWTEXT_END_TAG_OPEN 13
#define STATE_RAWTEXT_END_TAG_NAME 14
#define STATE_BEFORE_ATTRIBUTE_NAME 32
#define STATE_ATTRIBUTE_NAME 33
#define STATE_AFTER_ATTRIBUTE_NAME 34
#define STATE_BEFORE_ATTRIBUTE_VALUE 35
#define STATE_ATTRIBUTE_VALUE_DOUBLE 36
#define STATE_ATTRIBUTE_VALUE_SINGLE 37
#define STATE_ATTRIBUTE_VALUE_UNQUOTED 38
#define STATE_AFTER_ATTRIBUTE_VALUE_QUOTED 39
#define STATE_SELF_CLOSING 40
#define STATE_BOGUS_COMMENT 41
#define STATE_MARKUP_DEC_OPEN_STATE 42
#define STATE_COMMENT_START 43
#define STATE_COMMENT_START_DASH 44
#define STATE_COMMENT 45
#define STATE_COMMENT_END_DASH 50
#define STATE_COMMENT_END 51
#define STATE_BEFORE_DOCTYPE_NAME 54
#define STATE_DOCTYPE_NAME 55

#define SEL_STATE_ROUTE 0
#define SEL_STATE_TAG 1
#define SEL_STATE_CLASS 2
#define SEL_STATE_ID 3
#define SEL_STATE_OPERATOR 4
#define SEL_STATE_INDEX 5
#define SEL_STATE_ATTR 6
#define SEL_STATE_ATTR_OPERATOR 7
#define SEL_STATE_ATTR_VAL 8

#define IS_UPPERCASE_ALPHA(c) ('A' <= c && c <= 'Z')
#define IS_LOWERCASE_ALPHA(c) ('a' <= c && c <= 'z')
#define IS_ALPHA(c) (IS_UPPERCASE_ALPHA(c) || IS_LOWERCASE_ALPHA(c))
#define IS_DIGIT(c) ('0' <= c && c <= '9')
#define IS_SPACE(c) (c == 0x09 || c == 0x0A || c == 0x0C || c == 0x20 || c == 0x0D)

namespace html {

	class selector;
	class parser;
	class node;

	using node_ptr = std::shared_ptr<node>;

	enum class node_t {
		none,
		text,
		tag,
		comment,
		doctype
	};

	enum class tag_t {
		none,
		open,
		close,
	};

	enum class err_t {
		tag_not_closed
	};

	class node : public std::enable_shared_from_this<node> {
	public:
		node(node* parent = nullptr) : parent(parent) {}
		node(node&&);
		node& operator=(node&&);
		node_ptr at(size_t i) const {
			if(i < children.size()) {
				return children[i];
			}
			return std::make_shared<node>();
		}
		size_t size() const {
			return children.size();
		}
		bool empty() const {
			return children.empty();
		}
		std::vector<node_ptr>::iterator begin() {
			return children.begin();
		}
		std::vector<node_ptr>::iterator end() {
			return children.end();
		}
		node_ptr select(const selector, bool nested = true);
		std::string to_html(char indent = '	', bool child = true) const;
		std::string to_text(bool raw = false) const;
		node* get_parent() const {
			return parent;
		}
		std::vector<node_ptr> get_children() const {
			return children;
		}
		std::string get_attr(const std::string&) const;
		void set_attr(const std::string&, const std::string&);
		node_ptr append(const node_ptr&);
		void walk(std::function<bool(node&)>);
		node_ptr copy();
		node_t type_node = node_t::none;
		tag_t type_tag = tag_t::none;
		bool self_closing = false;
		std::string tag_name;
		std::string content;
		std::map<std::string, std::string> attributes;
	private:
		node* parent = nullptr;
		bool bogus_comment = false;
		std::vector<node_ptr> children;
		int index = 0;
		int node_count = 0;
		void copy(node*, node*);
		void walk(node&, std::function<bool(node&)>);
		void to_html(std::ostream&, bool, int, int&, char, bool&, bool&) const;
		void to_text(std::ostream&, bool&) const;
		friend class selector;
		friend class parser;
		friend class utils;
	};

	class selector {
	public:
		selector() = default;
		selector(const std::string);
		selector(const char* s) : selector(std::string(s)) {}
		operator bool() const {
			return !matchers.empty();
		}
		friend class node;
		friend class parser;
	private:
		struct condition {
			condition() = default;
			condition(const condition& d) = default;
			condition(condition&&);
			std::string tag_name;
			std::string id;
			std::string class_name;
			std::string index = "0";
			std::string attr;
			std::string attr_value;
			std::string attr_operator;
			bool operator()(const node&) const;
		};
		struct selector_matcher {
			selector_matcher() = default;
			selector_matcher(const selector_matcher&) = default;
			selector_matcher(selector_matcher&&);
			bool operator()(const node&) const;
		private:
			bool all_match = false;
			std::vector<std::vector<condition>> conditions;
			friend class selector;
		};
		std::vector<selector_matcher>::const_iterator begin() const {
			return matchers.begin();
		}
		std::vector<selector_matcher>::const_iterator end() const {
			return matchers.end();
		}
		std::vector<selector_matcher> matchers;
	};

	class parser {
	public:
		parser& set_callback(std::function<void(node&)> cb);
		parser& set_callback(const selector, std::function<void(node&)> cb);
		parser& set_callback(std::function<void(err_t, node&)> cb);
		void clear_callbacks();
		node_ptr parse(const std::string& html);
	private:
		void operator()(node&);
		void handle_node();
		int state = STATE_DATA;
		node* current_ptr = nullptr;
		node_ptr new_node;
		std::vector<std::pair<selector, std::function<void(node&)>>> callback_node;
		std::vector<std::function<void(err_t, node&)>> callback_err;
	};

	class utils {
	public:
		static std::shared_ptr<html::node> make_node(node_t, const std::string&, const std::map<std::string, std::string>& attributes = {});
	};

}

#endif