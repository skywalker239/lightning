// This file is part of the pd::pi library.
// Copyright (C) 2012, Eugene Mamchits <mamchits@yandex-team.ru>.
// Copyright (C) 2012, YANDEX LLC.
// This library may be distributed under the terms of the GNU LGPL 2.1.
// See ‘http://www.gnu.org/licenses/lgpl-2.1.html’.
#pragma once

#include <initializer_list>

#include <pd/pi/pi.H>

namespace pd {

class pi_t::pro_t {
public:
	class array_t;
	class item_t;

	static item_t const *null_array;

	class map_t;
	class pair_t;

	static pair_t const *null_map;

	struct enum_t {
		uint32_t val;
		inline enum_t(uint32_t _val) throw() : val(_val) { }
		inline enum_t(enum_t const &__enum) = default;
		inline ~enum_t() throw() { }
	};

	struct int_t {
		int32_t val;
		inline int_t(int32_t _val) throw() : val(_val) { }
		inline int_t(int_t const &__enum) = default;
		inline ~int_t() throw() { }
	};

	struct uint_t {
		uint64_t val;
		inline uint_t(uint64_t _val) throw() : val(_val) { }
		inline uint_t(uint_t const &__enum) = default;
		inline ~uint_t() throw() { }
	};

private:
	type_t type;
	unsigned short variant;
	
	union {
		pi_t const *pi;
		int32_t int_val;
		uint64_t uint_val;
		double float_val;
		str_t const *str;
		array_t const *array;
		item_t const *items;
		map_t const *map;
		pair_t const *pairs;
	};

	static inline type_t __int_type(int64_t _val) {
		return (_val <= _int29_max && _val >= _int29_min) ? _int29 : _uint64 ;
	}

	static inline type_t __uint_type(uint64_t _val) {
		return _val <= _int29_max ? _int29 : _uint64;
	}

public:
	inline pro_t() throw() :
		type(_enum), variant(1), uint_val(_null) { }

	inline pro_t(pi_t const &_pi) throw() :
		type(_pi.type()), variant(0), pi(&_pi) { }

	inline pro_t(pi_t const &&) = delete;

	inline pro_t(enum_t __enum) throw() :
		type(_enum), variant(1), uint_val(__enum.val) { }

	inline pro_t(uint_t __uint) throw() :
		type(__uint_type(__uint.val)), variant(1), uint_val(__uint.val) { }

	inline pro_t(int_t __int) throw() :
		type(__int_type(__int.val)), variant(1), int_val(__int.val) { }

	inline pro_t(str_t const &_str) throw() :
		type(_string), variant(1), str(&_str) { }

//	inline pro_t(str_t const &&) = delete;

	inline pro_t(array_t const &__array) throw() :
		type(_array), variant(1), array(&__array) { }

//	inline pro_t(array_t const &&) = delete;

	inline pro_t(item_t const *_items) throw() :
		type(_array), variant(2), items(_items) { }

	inline pro_t(map_t const &__map) throw() :
		type(_map), variant(1), map(&__map) { }

//	inline pro_t(map_t const &&) = delete;

	inline pro_t(pair_t const *_pairs) throw() :
		type(_map), variant(2), pairs(_pairs) { }

	inline operator bool() const throw() {
		return !(variant == 0 && pi->value == 0);
	}

	_size_t size() const throw();
	void put(pi_t *&ref, place_t &place) const throw();
};

struct pi_t::pro_t::array_t {
	count_t count;
	pro_t const *items;

    inline array_t() :
        count(0), items(NULL) { }

	inline array_t(count_t _count, pro_t const *_items) throw() :
		count(_count), items(_items) { }

	inline array_t(const std::initializer_list<pro_t>& _items) throw() :
		count(_items.size()), items(_items.begin()) { }
};

struct pi_t::pro_t::item_t {
	pro_t value;
	item_t const *next;

	inline item_t(pro_t const &_value, item_t const *_next) throw() :
		value(_value), next(_next) { }

	inline ~item_t() throw() { }
};

struct pi_t::pro_t::map_t {
	count_t count;
	struct item_t {
		pro_t key;
		pro_t value;
	};

	item_t const *items;

	inline map_t(count_t _count, item_t const *_items = NULL) throw() :
		count(_count), items(_items) { }

	inline map_t(const std::initializer_list<item_t>& _items) throw() :
		count(_items.size()), items(_items.begin()) { }
};

struct pi_t::pro_t::pair_t {
	pro_t key;
	pro_t value;
	pair_t const *next;

	inline pair_t(
		pro_t const &_key, pro_t const &_value, pair_t const *_next = NULL
	) throw() :
		key(_key), value(_value), next(_next) { }

	inline ~pair_t() throw() { }
};

namespace pi_build {
typedef pi_t::pro_t::array_t arr_t;
typedef pi_t::pro_t::map_t map_t;
typedef pi_t::pro_t::uint_t uint_t;
typedef pi_t::pro_t::int_t int_t;
} // namespace pi_build

} // namaspace pd
