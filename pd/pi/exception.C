// This file is part of the pd::pi library.
// Copyright (C) 2012, Eugene Mamchits <mamchits@yandex-team.ru>.
// Copyright (C) 2012, YANDEX LLC.
// This library may be distributed under the terms of the GNU LGPL 2.1.
// See ‘http://www.gnu.org/licenses/lgpl-2.1.html’.
#include "pi.H"

namespace pd {

void pi_t::exception_t::log() const {
	str_t msg = err.msg;

	switch(err.origin) {
		case err_t::_parse:
			log_error(
				"pi_t::parse err: %.*s, line = %lu, pos = %lu, abspos = %lu",
				(int)msg.size(), msg.ptr(),
				err.parse_lineno(), err.parse_pos(), err.parse_abspos()
			);
			break;

		case err_t::_verify:
			log_error(
				"pi_t::verify err: %.*s, lev = %lu, obj = %lu, req = %lu, bound = %lu",
				(int)msg.size(), msg.ptr(), err.verify_lev(),
				err.verify_obj(), err.verify_req(), err.verify_bound()
			);
			break;

		case err_t::_path:
			log_error(
				"pi_t::path err: %.*s, lev = %lu",
				(int)msg.size(), msg.ptr(), err.path_lev()
			);
			break;

		default:
			log_error(
				"pi_t::origin(%u) err: %.*s, aux0 = %lu, aux1 = %lu, aux2 = %lu, aux3 = %lu, aux4 = %lu, aux5 = %lu",
				err.origin, (int)msg.size(), msg.ptr(), err.aux[0], err.aux[1], err.aux[2],
				err.aux[3], err.aux[4], err.aux[5]
			);
	}
}

str_t pi_t::exception_t::msg() const { return err.msg; }

pi_t::exception_t::~exception_t() throw() { }

} // namespace pd
