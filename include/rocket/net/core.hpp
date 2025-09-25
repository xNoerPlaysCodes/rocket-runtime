#ifndef ROCKETGE__NETWORKING_CORE_HPP
#define ROCKETGE__NETWORKING_CORE_HPP

namespace rocket::net {
    void init();

    /// @brief Check the connection status by pinging some popular websites
    bool check_connection_status();

    enum class status_t : int {
        ok = 200,
        not_found = 404,
        server_error = 500,
        bad_request = 400,
        unauthorized = 401,
    };

    // I'm not very eager to continue rocket::net
    // maybe someone else can pull this off...
}

#endif // ROCKETGE__NETWORKING_CORE_HPP
