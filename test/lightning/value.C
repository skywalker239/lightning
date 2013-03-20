// Copyright (C) 2012, Pervushin Alexey <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:

#include <pd/lightning/value.H>
#include <pd/base/out_fd.H>
#include <stdio.h>

using namespace pd;

static char outbuf[1024];
static out_fd_t out(outbuf, sizeof(outbuf), 1);

static void test_helper(const value_t& value) {
    out.print((int)value.valid()).lf();
    out.print(value.value_id()).lf();
    const pi_t::root_t *root = (const pi_t::root_t *)(value.value().ptr());
    out.print(root->value, "10")(';').lf();
}

static inline void __test(string_t const &string) {
    out.print(string, "e").lf();

    ref_t<pi_ext_t> parsed;

    try {
        in_t::ptr_t ptr = string;
        parsed = pi_ext_t::parse(ptr, &pi_t::parse_text);;
        out.print(parsed->pi(), "10")(';').lf();
    }
    catch(exception_t const &ex) {
        ex.log();
    }

    const str_t value_str((char const *)&(parsed->root()), parsed->root().size * sizeof(pi_t::_size_t));

    value_t v(13, pd::string(value_str));

    test_helper(v);
    out(CSTR("---------")).lf().flush_all();
}

#define test(x) __test(STRING(x))

extern "C" int main() {
    test("{ \"abcdef\": \"123456\" \"qwerty\": [ \"\" \"a\" \"q\" ] \"asddf\" : { \"m\": \"none\" \"l\": { } }};");
    return 0;
}

