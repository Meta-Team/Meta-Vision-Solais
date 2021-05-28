//
// Created by liuzikai on 5/28/21.
//

#ifndef META_VISION_SOLAIS_FRAMECOUNTERBASE_H
#define META_VISION_SOLAIS_FRAMECOUNTERBASE_H

namespace meta {

class FrameCounterBase {
public:

    /**
     * Get current processed frame count and clear the counter.
     * @note Here we want to avoid atomic to possibly improve the performance. We make an assumption that an increment
     *       doesn't produce outputs other than the original value or it plus one. cumulativeFrameCounter is to be only
     *       incremented in the thread and only read in this function. lastFetchFrameCounter is only used by this
     *       function. If the assumption holds, the result shall be correct.
     * @note There should be no concurrent calls to this function from different thread. Callbacks from io_context
     *       should be fine (all from the thread that runs the io_context). Otherwise, lastFetchFrameCounter may have
     *       concurrent access, falsify the reasoning above.
     * @return
     */
    unsigned int fetchAndClearFrameCounter() {
        unsigned int ret = cumulativeFrameCounter - lastFetchFrameCounter;  // only read cumulativeFrameCounter

        // Even if cumulativeFrameCounter overflows, the subtraction should still be correct?

        lastFetchFrameCounter += ret;  // avoid reading cumulativeFrameCounter as it may have changed
        return ret;
    }

protected:

    // To be (only) incremented by the thread and read by fetchAndClearFrameCounter(). Always increasing. Avoid atomic.
    unsigned int cumulativeFrameCounter = 0;

    // Use by fetchAndClearFrameCounter() to calculate difference between two fetches
    unsigned int lastFetchFrameCounter = 0;

};

}

#endif //META_VISION_SOLAIS_FRAMECOUNTERBASE_H
