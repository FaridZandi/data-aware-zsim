//
// Created by farid on 5/1/18.
//

#ifndef DATA_AWARE_ZSIM_BDILRUREPLPOLICY_H
#define DATA_AWARE_ZSIM_BDILRUREPLPOLICY_H

#include <queue>
#include "repl_policies.h"


template<bool sharersAware>
class BDILRUReplPolicy : public ReplPolicy {
protected:
    uint64_t timestamp; // incremented on each access
    uint64_t *array;
    uint32_t numLines;

public:
    explicit BDILRUReplPolicy(uint32_t _numLines) : timestamp(1), numLines(_numLines) {
        array = gm_calloc<uint64_t>(numLines);
    }


    ~BDILRUReplPolicy() {
        gm_free(array);
    }

    void update(uint32_t id, const MemReq *req) {
        array[id] = timestamp++;
    }

    void replaced(uint32_t id) {
        array[id] = 0;
    }

    candsPriorityQueue buildCandsPriorityQueue(uint32_t begin, uint32_t end) override {
        candsPriorityQueue scores;

        for (uint32_t i = begin; i < end; ++i) {
            scores.push(std::pair<uint64_t, uint32_t>(score(i), i));
        }

        return scores;
    }

    template<typename C>
    inline uint32_t rank(const MemReq *req, C cands) {
        uint32_t bestCand = -1;
        uint64_t bestScore = (uint64_t) -1L;
        for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
            uint32_t s = score(*ci);
            bestCand = (s < bestScore) ? *ci : bestCand;
            bestScore = MIN(s, bestScore);
        }
        return bestCand;
    }

    DECL_RANK_BINDINGS;

private:
    inline uint64_t score(uint32_t id) { //higher is least evictable
        //array[id] < timestamp always, so this prioritizes by:
        // (1) valid (if not valid, it's 0)
        // (2) sharers, and
        // (3) timestamp
        return (sharersAware ? cc->numSharers(id) : 0) * timestamp + array[id] * cc->isValid(id);
    }
};


#endif //DATA_AWARE_ZSIM_BDILRUREPLPOLICY_H