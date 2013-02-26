// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <vector>

#include <phantom/pd.H>
#include <phantom/io.H>
#include <phantom/module.H>

#include <pd/base/ref.H>
#include <pd/base/time.H>
#include <pd/base/log.H>
#include <pd/base/assert.H>
#include <pd/bq/bq_job.H>
#include <pd/bq/bq_cond.H>
#include <pd/bq/bq_util.H>
#include <pd/intrusive/list.H>
#include <pd/intrusive/lru_cache.H>
#include <pd/intrusive/hash_map.H>

namespace phantom {

MODULE(test_pd_intrusive);

using namespace pd::intrusive;

class io_pd_intrusive_test_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        inline void check(const in_t::ptr_t& ptr) const {
            io_t::config_t::check(ptr);
        }
    };

    io_pd_intrusive_test_t(const string_t& name, const config_t& config)
        : io_t(name, config) {}

    virtual void init() {}

    virtual void fini() {}

    virtual void run() {
        log_info("Running tests");

        test_simple_list();
        test_lru_cache();
        stress_test_lru_cache();
        test_hash_map();

        log_info("All tests finished");
        log_info("Sending SIGQUIT");
        kill(getpid(), SIGQUIT);
    }

    virtual void stat(out_t& /*out*/, bool /*clear*/) {}
private:
    struct foo_t : public list_hook_t<foo_t> {
        explicit foo_t(int x) : x(x) {}

        int x;
    };

    void test_simple_list() {
        simple_list_t<foo_t> list;

        foo_t a(1), b(2), c(3);

        assert(!a.is_linked());

        list.checkrep();

        assert(list.empty());
        assert(!list.ptr()); // list empty

        list.push_front(&a);
        list.checkrep();

        assert(!list.empty());
        auto ptr = list.ptr();
        assert(ptr->x == 1);
        assert(ptr);

        ptr++;
        assert(!ptr);

        list.push_front(&b);
        list.push_front(&c);
        list.checkrep();

        ptr = list.ptr();
        assert((*ptr).x == 3);
        ++ptr;
        assert(ptr->x == 2);
        ++ptr;
        assert(ptr->x == 1);
        ++ptr;
        assert(!ptr);

        assert(list.front()->x == 3);
        list.pop_front();
        list.checkrep();

        assert(list.front()->x == 2);
        list.pop_front();
        list.checkrep();

        assert(list.front()->x == 1);
        list.pop_front();
        list.checkrep();

        assert(list.empty());

        list.push_front(&a);
        list.push_front(&b);
        list.push_front(&c);

        ptr = ++(list.ptr());

        ptr->unlink();

        list.checkrep();

        ptr = list.ptr();
        assert(ptr->x == 3);
        ++ptr;
        assert(ptr->x == 1);
        ++ptr;

        assert(!ptr);
    }

    void test_list() {
        list_t<foo_t> list;

        assert(list.empty());
        foo_t a(1), b(2), c(3);

        list.push_back(&a);
        assert(!list.empty());
        assert(list.front()->x == 1);

        list.push_back(&b);
        assert(!list.empty());
        assert(list.front()->x == 1);

        list.push_back(&c);
        assert(!list.empty());
        assert(list.front()->x == 1);

        assert(list.pop_front() == &a);
        assert(list.pop_front() == &b);
        assert(list.pop_front() == &c);
        assert(list.empty());
    }

    struct bar_t : public lru_cache_t<bar_t>::hook_t {
        explicit bar_t(int x) : x(x) {}

        int x;
    };

    void test_lru_cache() {
        lru_cache_t<bar_t> cache(2);

        bar_t a(1), b(2), c(3);

        assert(cache.push(&a) == NULL);
        assert(cache.push(&b) == NULL);
        assert(cache.push(&c) == &a);

        assert(cache.push(&b) == NULL);
        assert(cache.push(&a) == &c);
    }

    void stress_test_lru_cache() {
        std::vector<bar_t> v;

        v.emplace_back(1);
        v.emplace_back(1);
        v.emplace_back(1);
        v.emplace_back(1);
        v.emplace_back(1);

        lru_cache_t<bar_t> cache(3);

        for (int i = 0; i < 10000; ++i) {
            cache.push(&v[rand() % v.size()]);

            bar_t& e = v[rand() & v.size()];

            if(e.is_linked()) {
                cache.pop(&e);
            }
        }
    }

    struct zog_t : public hash_map_hook_t<zog_t> {
        typedef int key_t;

        zog_t(int key, int value) : key(key), value(value) {}

        int key;
        int value;
    };

    struct zog_key_fn_t {
        int operator() (const zog_t& z) const {
            return z.key;
        }
    };

    typedef hash_map_t<zog_t, zog_key_fn_t> zog_map_t;

    void test_hash_map() {
        zog_map_t map(2);

        zog_t a(1, 2), b(3, 4), c(5, 6), d(7, 8);

        assert(map.lookup(2) == NULL);
        map.checkrep();

        map.insert(&a);
        map.checkrep();

        map.insert(&b);
        map.checkrep();

        map.insert(&c);
        map.checkrep();

        map.insert(&d);
        map.checkrep();

        assert(&a == map.lookup(1));
        assert(&b == map.lookup(3));
        assert(&c == map.lookup(5));
        assert(&d == map.lookup(7));

        assert(NULL == map.lookup(4));

        a.unlink();
        map.checkrep();
        assert(NULL == map.lookup(1));
    }
};

namespace io_pd_intrusive_test {
config_binding_sname(io_pd_intrusive_test_t);
config_binding_parent(io_pd_intrusive_test_t, io_t, 1);
config_binding_ctor(io_t, io_pd_intrusive_test_t);
}

} // namespace phantom
