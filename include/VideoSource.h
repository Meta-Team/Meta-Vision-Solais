//
// Created by liuzikai on 5/25/21.
//

#ifndef META_VISION_SOLAIS_VIDEOSOURCE_H
#define META_VISION_SOLAIS_VIDEOSOURCE_H

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include "Parameters.pb.h"

namespace meta {

class VideoSource {
public:

    virtual ~VideoSource() {}

    /**
     * Start streaming and fetch the first frame.
     * @param params
     * @return
     */
    virtual bool open(const package::ParamSet &params) = 0;

    /**
     * Whether the streaming source is opened
     * @return
     */
    virtual bool isOpened() const = 0;

    /**
     * Close the streaming source
     */
    virtual void close() = 0;

    /**
     * Get current frame ID. -1 indicates the end of the stream.
     * @return
     */
    virtual int getFrameID() const = 0;

    /**
     * Get current frame. No need for data copying due to double buffering.
     * @return
     */
    virtual const cv::Mat &getFrame() const = 0;

    /**
     * Fetch next frame. Called after current frame is accepted (no copy needed due to double buffering).
     */
    virtual void fetchNextFrame() {};

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
    virtual unsigned int fetchAndClearFrameCounter() {
        unsigned int ret = cumulativeFrameCounter - lastFetchFrameCounter;  // only read cumulativeFrameCounter

        // Even if cumulativeFrameCounter overflows, the subtraction should still be correct?

        lastFetchFrameCounter += ret;  // avoid reading cumulativeFrameCounter as it may have changed
        return ret;
    }

protected:

    static constexpr int FRAME_ID_MAX = 0x7FFF;

    // To be (only) incremented by the thread and read by fetchAndClearFrameCounter(). Always increasing. Avoid atomic.
    unsigned int cumulativeFrameCounter = 0;

    // Use by fetchAndClearFrameCounter() to calculate difference between two fetches
    unsigned int lastFetchFrameCounter = 0;

};

}

#endif //META_VISION_SOLAIS_VIDEOSOURCE_H
