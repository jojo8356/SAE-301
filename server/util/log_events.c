/*
 * Copyright (c) AnimeTCG contributors.
 * SPDX-License-Identifier: MIT
 */
#include "log_events.h"

#define EVENT(message_, module_, level_, client_only_, deprecated_) \
    { (message_), (module_), (level_), (client_only_), (deprecated_) }
#define ARRAY_COUNT(array_) (sizeof(array_) / sizeof((array_)[0]))

static const animetcg_log_event_info_t EVENT_CATALOGUE[] = {
    EVENT("server started", "session", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("listen ready", "session", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("server stopped", "session", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("client connected", "session", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("client address", "session", ANIMETCG_LOG_LEVEL_DEBUG, false, false),
    EVENT("client admitted", "session", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("client disconnected", "session", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("client expired", "session", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("queue enqueue", "session", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("queue update", "session", ANIMETCG_LOG_LEVEL_DEBUG, false, false),
    EVENT("queue full", "session", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("queue timeout", "session", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("session welcomed", "session", ANIMETCG_LOG_LEVEL_INFO, true, false),
    EVENT("reconnect attempt", "session", ANIMETCG_LOG_LEVEL_WARN, true, false),
    EVENT("server busy", "session", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("resync requested", "session", ANIMETCG_LOG_LEVEL_WARN, true, false),

    EVENT("request sent", "net", ANIMETCG_LOG_LEVEL_DEBUG, true, false),
    EVENT("request received", "net", ANIMETCG_LOG_LEVEL_DEBUG, false, false),
    EVENT("response received", "net", ANIMETCG_LOG_LEVEL_DEBUG, true, false),
    EVENT("connection lost", "net", ANIMETCG_LOG_LEVEL_ERROR, true, false),
    EVENT("event broadcast", "net", ANIMETCG_LOG_LEVEL_DEBUG, false, false),
    EVENT("listen failed", "net", ANIMETCG_LOG_LEVEL_FATAL, false, false),

    EVENT("frame read", "protocol", ANIMETCG_LOG_LEVEL_TRACE, false, false),
    EVENT("frame rejected", "protocol", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("request accepted", "protocol", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("request refused", "protocol", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("protocol error", "protocol", ANIMETCG_LOG_LEVEL_ERROR, false, false),
    EVENT("version refused", "protocol", ANIMETCG_LOG_LEVEL_WARN, false, false),

    EVENT("calibration", "chain", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("block mined", "chain", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("mining deadline exceeded", "chain", ANIMETCG_LOG_LEVEL_ERROR, false, false),
    EVENT("nonce budget exhausted", "chain", ANIMETCG_LOG_LEVEL_ERROR, false, false),
    EVENT("chain loaded", "chain", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("chain verified", "chain", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("verification failed", "chain", ANIMETCG_LOG_LEVEL_ERROR, false, false),
    EVENT("difficulty changed", "chain", ANIMETCG_LOG_LEVEL_INFO, false, false),

    EVENT("block committed", "persistence", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("query done", "persistence", ANIMETCG_LOG_LEVEL_DEBUG, false, false),
    EVENT("db slow", "persistence", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("db write failed", "persistence", ANIMETCG_LOG_LEVEL_ERROR, false, false),
    EVENT("db unavailable", "persistence", ANIMETCG_LOG_LEVEL_ERROR, false, false),
    EVENT("restore done", "persistence", ANIMETCG_LOG_LEVEL_INFO, false, false),

    EVENT("card created", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("card inactivated", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("card archived", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("value updated", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("recommendation added", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("recommendation repudiated", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("battle opened", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("vote cast", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("vote refused", "business", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("battle closed", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("legitimation opened", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("legitimation liked", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("legitimation taken", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("claim opened", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("claim transferred", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("claim refused", "business", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("claim closed", "business", ANIMETCG_LOG_LEVEL_INFO, false, true),
    EVENT("transfer scheduled", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("rule refused", "business", ANIMETCG_LOG_LEVEL_WARN, false, false),

    EVENT("log rotated", "util", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("log dropped", "util", ANIMETCG_LOG_LEVEL_WARN, false, false),

    EVENT("card authenticated", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("authentication revoked", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("authentication refused", "business", ANIMETCG_LOG_LEVEL_WARN, false, false),
    EVENT("share sent", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("share accepted", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("share refused", "business", ANIMETCG_LOG_LEVEL_INFO, false, false),
    EVENT("startup failed", "view", ANIMETCG_LOG_LEVEL_FATAL, true, false)
};

_Static_assert(ARRAY_COUNT(EVENT_CATALOGUE) == (size_t)ANIMETCG_LOG_EVENT_COUNT,
               "log event catalogue and enum are out of sync");

const animetcg_log_event_info_t *animetcg_log_event_get_info(
    animetcg_log_event_t event)
{
    if ((int)event < 0 || (size_t)event >= ARRAY_COUNT(EVENT_CATALOGUE)) {
        return NULL;
    }
    return &EVENT_CATALOGUE[(size_t)event];
}

size_t animetcg_log_event_count(void)
{
    return ARRAY_COUNT(EVENT_CATALOGUE);
}
