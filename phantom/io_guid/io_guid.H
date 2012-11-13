#pragma once

#include <phantom/io.H>
#include <pd/lightning/guid.H>

#pragma GCC visibility push(default)

namespace phantom {

class io_guid_t : public io_t {
public:
	struct config_t : public io_t::config_t {
		int host_id;

		config_t() throw();
		~config_t() throw() {};
		void check(const in_t::ptr_t& p) const;
	};
	guid_generator_t guid_;

	io_guid_t(const string_t& name, const config_t& config);
	~io_guid_t() throw() {}
private:
	virtual void init();
	virtual void run();
	virtual void fini();

	virtual void stat(out_t& out, bool clear);
};

} // namespace phantom

