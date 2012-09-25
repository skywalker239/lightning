#pragma once

#include <phantom/io_zookeeper/io_zookeeper.H>

#include <pd/base/string.H>

#include <pd/bq/bq_cond.H>

namespace pd {

class var_base_t {
public:
    var_base_t(const string_t& key, phantom::io_zookeeper_t* io_zookeeper);

    ~var_base_t();

    static const int kDeletedVersion = -1;
    static const int kAnyVersion = -2;
    static const int kInvalidVersion = -3;

    //! Update the local copy from io_zookeeper.
    //  Returns the current version of the variable.
    //  If the variable has not yet been fetched by io_zookeeper,
    //  blocks until it becomes available.
    //
    //  Version kDeletedVersion means that the corresponding key
    //  has been deleted from Zookeeper.
    int update();

    //! Blocks until the version of the variable becomes different from
    //  old_version.
    int wait(int old_version);

    //! Resets the variable to a new value. Blocks until ZK call completes.
    //  Does not reset the local copy (i.e. call set() and then update).
    void set(const string_t& value);

    bool exists() const;

    int version() const;

    const string_t& value_string() const;

    const string_t& key() const;
protected:
    int version_;
    string_t value_string_;

    //! Called by update() to parse the underlying value string into
    //  data the descendant class is interested in.
    virtual void update_impl() {}
private:
    var_base_t(const var_base_t&) = delete;
    var_base_t& operator=(const var_base_t&) = delete;

    void update_internal();

    string_t key_;

    phantom::io_zookeeper_t* io_zookeeper_;
    phantom::io_zookeeper_t::stat_t* stat_;
};

}  // namespace pd
