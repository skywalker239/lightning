#include "pi.H"

#include <errno.h> // Really dont needed

namespace pd {

struct pi_t::parse_t {
	in_t::ptr_t &__ptr;
	mem_t &mem;
	enum_table_t const &enum_table;

	in_t::ptr_t ptr;

	struct ctx_t {
		ctx_t *next;
		enum type_t { _root = 0, _array, _map } type;
		enum state_t { _init = 0, _item, _key, _colon } state;
		_size_t count;

		pi_t *ref;

		inline ctx_t(type_t _type, ctx_t *_next) throw() :
			next(_next), type(_type), state(_init), count(0), ref(NULL) { }

		inline ~ctx_t() throw() { }			

		void __noreturn error(parse_t &parse);
		void item(parse_t &parse);
		void end_of_array(parse_t &parse);
		void colon(parse_t &parse);
		void end_of_map(parse_t &parse);
		void end_of_root(parse_t &parse);
	};

	ctx_t *ctx;
	_size_t size;
	place_t place;

	root_t *res_root;

	inline parse_t(
		in_t::ptr_t &_ptr, mem_t &_mem, enum_table_t const &_enum_table
	) throw() :
		__ptr(_ptr), mem(_mem), enum_table(_enum_table),
		ptr(__ptr), ctx(NULL), size(0), place(NULL), res_root(NULL) { }

	void check_space();

	count_t string_prepare(bool &has_esc);
	void string_put(in_t::ptr_t &ptr, char *dst);

	union number_t { int64_t i; double f; };
	number_t parse_number(type_t &type);
	unsigned int parse_enum();

	void do_parse();

	inline root_t *operator()() {
		ctx_t _ctx(ctx_t::_root, NULL);
		ctx = &_ctx;

		do_parse();

		assert(ctx == &_ctx);
		assert(place._pi == (pi_t *)res_root + size);

		__ptr = ptr;

		return res_root;
	}

	inline void __noreturn error(char const *msg) {
		in_t::ptr_t _ptr = __ptr;
		size_t limit = ptr - __ptr;

		size_t lineno = 1;
		while(_ptr.scan("\n", 1, limit)) { ++_ptr; ++lineno; --limit; }

		err_t err(err_t::_parse, str_t(msg, strlen(msg)));
		err.parse_lineno() = lineno;
		err.parse_pos() = ptr - _ptr + 1;
		err.parse_abspos() = ptr - __ptr;

		throw exception_t(err);
	}
};

void __noreturn pi_t::parse_t::ctx_t::error(parse_t &parse) {
	switch(type) {
		case _root:
			switch(state) {
				case _init:
					parse.error("root object expected");
				case _item:
					parse.error("semicolon expected");
				default:;
			}
			break;
		case _array:
			switch(state) {
				case _init:
				case _item:
					parse.error("object or end of array expected");
				default:;
			}
			break;
		case _map:
			switch(state) {
				case _init:
				case _item:
					parse.error("key or end of map expected");
				case _key:
					parse.error("colon expected");
				case _colon:
					parse.error("object expected");
				default:;
			}
			break;
	}

	abort();
}

void pi_t::parse_t::ctx_t::item(parse_t &parse) {
	switch(type) {
		case _root:
			if(state == _init) {
				state = _item; return;
			}
			break;
		case _array:
			if(state == _init || state == _item) {
				state = _item; return;
			}
			break;
		case _map:
			if(state == _colon) {
				state = _item; return;
			}
			else if(state == _init || state == _item) {
				state = _key; return;
			}
			break;
	}

	error(parse);	
}

void pi_t::parse_t::ctx_t::end_of_array(parse_t &parse) {
	if(type == _array)
		return;

	error(parse);
}

void pi_t::parse_t::ctx_t::colon(parse_t &parse) {
	if(type == _map && state == _key) {
		state = _colon;
		return;
	}

	error(parse);
}

void pi_t::parse_t::ctx_t::end_of_map(parse_t &parse) {
	if(type == _map && state != _colon)
		return;

	error(parse);
}

void pi_t::parse_t::ctx_t::end_of_root(parse_t &parse) {
	if(type == _root && state == _item)
		return;

	error(parse);
}

void pi_t::parse_t::check_space() {
	switch(*ptr) {
		case '0' ... '9': case '-':
		case 'a' ... 'z': case 'A' ... 'Z': case '_':
		case '"':
			error("whitespace between scalars required");
	}
}

static inline bool parse_oct(char c, char &res) {
	switch(c) {
		case '0'...'7': res = res * 8 + (c - '0');
		return true;
	}
	return false;
}

pi_t::count_t pi_t::parse_t::string_prepare(bool &_has_esc) {
	count_t str_len = 0;
	bool has_esc = false;
	char c;

	while(true) {
		switch(*ptr) {
			case '\\': {
				has_esc = true;
				switch(c = *++ptr) {
					case 'r': case 'n': case 't': case 'v':
					case 'b': case 'f': case 'a': case 'e':
					case '"': case '\\': break;
					case '0' ... '3': {
						c -= '0';
						if(!parse_oct(*++ptr, c) || !parse_oct(*++ptr, c))
							error("not an octal digit");
					}
					break;

					default:
						error("illegal escape character");
				}
			}
			break;

			case '"':
				++ptr;
				check_space();
				_has_esc = has_esc;
				return str_len;

			case '\0' ... (' ' - 1): case '\177':
				error("control character in the string");

		}
		++ptr;
		++str_len;
	}
}

void pi_t::parse_t::string_put(in_t::ptr_t &ptr, char *dst) {
	char c;

	while(true) {
		switch(c = *ptr) {
			case '\\': {
				switch(c = *++ptr) {
					case 'r': c = '\r'; break;
					case 'n': c = '\n'; break;
					case 't': c = '\t'; break;
					case 'v': c = '\v'; break;
					case 'b': c = '\b'; break;
					case 'f': c = '\f'; break;
					case 'a': c = '\a'; break;
					case 'e': c = '\e'; break;
					case '"': case '\\': break;
					case '0' ... '3': {
						c -= '0';
						if(!parse_oct(*++ptr, c) || !parse_oct(*++ptr, c))
							abort();
					}
					break;

					default:
						abort();
				}
			}
			break;

			case '"':
				return;

			case '\0' ... (' ' - 1): case '\177':
				abort();
		}
		++ptr;
		*(dst++) = c;
	}
}

pi_t::parse_t::number_t pi_t::parse_t::parse_number(type_t &type) {
	number_t val;
	val.i = 0;
	int neg = 0;

	in_t::ptr_t _ptr = ptr;
	char c = *ptr;

	if(c == '-') {
		neg = 1; c = *++ptr;

		if(c < '0' || c > '9')
			error("digit expected");
	}

	uint64_t _val = c - '0';

	while(true) {
		c = *++ptr;

		if(c >= '0' && c <= '9') {
			uint64_t tmp = _val * 10 + (c - '0');
			if(tmp / 10 != _val) {
				ptr = _ptr;
				error("integer overflow");
			}

			_val = tmp;
		}
		else if(c == '.' || c == 'e' || c =='E') {
			while(true) {
				switch(*++ptr) {
					case '.': case 'e': case 'E': case '+': case '-': case '0' ... '9':
						continue;
				}
				check_space();
				break;
			}

			size_t size = ptr - _ptr;
			char buf[size + 1];
			{ out_t out(buf, size + 1); out(_ptr, size)('\0'); }

			char *e = buf;
			int _errno = errno;

			errno = 0;

			// FIXME: Use sane parsing function here.
			val.f = strtod(buf, &e);
			if(errno || e != &buf[size]) {
				ptr = _ptr;
				error("floating point number parsing error");
			}

			errno = _errno;

			type = _float;
			return val;
		}
		else {
			check_space();
			break;
		}
	}

	if(neg) {
		if(_val > ((uint64_t)1 << 63)) {
			ptr = _ptr;
			error("integer overflow");
		}

		val.i = -_val;
		type = val.i >= _int29_min ? _int29 : _uint64;
	}
	else {
		type = _val <= _int29_max ? _int29 : _uint64;
		val.i = _val;
	}

	return val;
}

unsigned int pi_t::parse_t::parse_enum() {
	in_t::ptr_t _ptr = ptr;

	while(true) {
		switch(*++ptr) {
			case 'a' ... 'z': case 'A' ... 'Z':
			case '_': case '0' ... '9': continue;
		}
		check_space();
		break;
	}

	pd::string_t string(_ptr, ptr - _ptr);

	unsigned int res = enum_table.lookup(string.str());

	if(res == ~0U) {
		ptr = _ptr;
		error("unknown enum value");
	}

	return res;
}

static inline bool is_space(char c) {
	switch(c) {
		case ' ': case'\t': case '\n': case '\r': return true;
	}

	return false;
}

static inline char skip_space(in_t::ptr_t &ptr) {
	char c;
	while(is_space(c = *ptr)) ++ptr;
	return c;
}

void pi_t::parse_t::do_parse() {
	switch(skip_space(ptr)) {
		case '[': {
			ctx->item(*this);
			++ptr;
			ctx_t _ctx(ctx_t::_array, ctx);
			ctx = &_ctx;

			do_parse();

			assert(ctx == &_ctx);

			assert(!_ctx.count || (((array_t *)_ctx.ref) - 1)->count== _ctx.count);
			ctx = _ctx.next;
			return;
		}

		case '{': {
			ctx->item(*this);
			++ptr;
			ctx_t _ctx(ctx_t::_map, ctx);
			ctx = &_ctx;

			do_parse();

			assert(ctx == &_ctx);

			if(_ctx.count) {
				map_t *map = ((map_t *)_ctx.ref) - 1;
				assert(map->count == _ctx.count / 2);
				map->index_make();
			}
			ctx = _ctx.next;
			return;
		}

		case ':': {
			ctx->colon(*this);
			++ptr;

			do_parse();
			return;
		}

		case ']': {
			ctx->end_of_array(*this);
			++ptr;
			ctx_t *_ctx = ctx;

			if(_ctx->count)
				size += array_t::_size(_ctx->count);

			ctx = _ctx->next;
			++ctx->count;

			do_parse();

			assert(ctx == _ctx->next);

			pi_t *ref = --ctx->ref;
			ctx = _ctx;

			if(ctx->count) {
				count_t count = ctx->count;
				place.__array->count = count;
				ctx->ref = &place.__array->items[count];

				ref->setup(_array, place._pi);
				place._pi += place.__array->_size();
			}
			else
				ref->setup(_array);

			return;
		}

		case '}': {
			ctx->end_of_map(*this);
			++ptr;
			ctx_t *_ctx = ctx;

			assert(!(_ctx->count % 2));

			if(_ctx->count)
				size += map_t::_size(_ctx->count / 2);

			ctx = _ctx->next;
			++ctx->count;

			do_parse();

			assert(ctx == _ctx->next);

			pi_t *ref = --ctx->ref;
			ctx = _ctx;

			if(ctx->count) {
				count_t count = ctx->count / 2;
				place.__map->count = count;
				ctx->ref = &place.__map->items[count].key;

				ref->setup(_map, place._pi);
				place._pi += place.__map->_size();
			}
			else
				ref->setup(_map);

			return;
		}

		case ';': {
			ctx->end_of_root(*this);
			++ptr;

			size += root_t::_size();

			res_root = root_t::__new(size, mem);
			ctx->ref = &res_root->value + 1;
			place.__root = res_root;

			place._pi += place.__root->_size();

			return;
		}

		case '"': {
			ctx->item(*this);
			++ptr;
			in_t::ptr_t _ptr = ptr;
			bool has_esc = false;
			count_t str_len = string_prepare(has_esc);
			if(str_len)
				size += string_t::_size(str_len);

			++ctx->count;

			do_parse();

			pi_t *ref = --ctx->ref;
			if(str_len) {
				place.__string->count = str_len;
				_size_t size = place.__string->_size();
				place._pi[size - 1].value = 0U;

				if(!has_esc) {
					out_t out(place.__string->items, str_len);
					out(_ptr, str_len);
				}
				else
					string_put(_ptr, place.__string->items);

				ref->setup(_string, place._pi);
				place._pi += size;
			}
			else
				ref->setup(_string);

			return;
		}

		case '0' ... '9': case '-': {
			ctx->item(*this);

			type_t type;
			number_t val = parse_number(type);

			if(type != _int29)
				size += _number_size;

			++ctx->count;

			do_parse();

			pi_t *ref = --ctx->ref;
			switch(type) {
				case _float: {
					*place.__float = val.f;

					ref->setup(_float, place._pi);
					place._pi += _number_size;
				}
				break;
				case _uint64: {
					*place.__uint64 = val.i;

					ref->setup(_uint64, place._pi);
					place._pi += _number_size;
				}
				break;
				case _int29: {
					ref->setup(_int29, val.i);
				}
				break;
				default:
					abort();
			}
			return;
		}

		case 'a' ... 'z': case 'A' ... 'Z': case '_': {
			ctx->item(*this);
			unsigned int val = parse_enum();
			++ctx->count;

			do_parse();

			pi_t *ref = --ctx->ref;
			ref->setup(_enum, val);

			return;
		}

		default:
			ctx->error(*this);
	}
}

pi_t::root_t *pi_t::parse_text(
	in_t::ptr_t &_ptr, mem_t &mem, enum_table_t const &enum_table
) {
	return parse_t(_ptr, mem, enum_table)();
}

pi_t::root_t *pi_t::parse_app(
	in_t::ptr_t &_ptr, mem_t &mem, enum_table_t const &enum_table
) {
	in_t::ptr_t ptr = _ptr;
	_size_t _size = 0;
	for(int i = 0; i < 32; i += 8)
		_size |= (*(ptr++) << i);

	size_t size = _size * sizeof(pi_t);
	char *buf = (char *)mem.alloc(size);

	try {
		{ out_t out(buf, size); out(_ptr, size); }

		verify(buf, size, enum_table);

		_ptr += size;

		return (root_t *)buf;
	}
	catch(...) {
		mem.free(buf);
		throw;
	}
}
	
} // namespace pd
