#ifndef CPP_PCP_CLIENT_SRC_CONNECTOR_SESSION_ASSOCIATION_HPP_
#define CPP_PCP_CLIENT_SRC_CONNECTOR_SESSION_ASSOCIATION_HPP_

#include <cpp-pcp-client/util/thread.hpp>
#include <cpp-pcp-client/export.h>

#include <string>
#include <atomic>

namespace PCPClient {
namespace v1 {

// Encapsulates concurrency things for managing the state of the
// Associate Session process; consumer code may want to hold a lock
// over the instance mutex, depending on the usage

struct CPP_PCP_CLIENT_EXPORT SessionAssociation {
    std::atomic<bool> success;
    std::atomic<bool> in_progress;
    std::atomic<bool> got_messaging_failure;
    std::string request_id;
    std::string error;
    Util::mutex mtx;
    Util::condition_variable cond_var;
    uint32_t association_timeout_s;

    SessionAssociation(uint32_t assoc_timeout_s);

    // Does not lock mtx
    void reset();
};

}  // namespace v1
}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_CONNECTOR_SESSION_ASSOCIATION_HPP_
